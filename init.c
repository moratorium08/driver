#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "init.h"
#include "util.h"
#define DANGER
#include "reg.h"
#undef DANGER

void init_pci() {
  int uiofd, configfd, csrfd;
  uint16_t buf16;

  uiofd = open("/dev/uio0", O_RDONLY);
  if (uiofd < 0) {
    perror("uio open:");
    panic("");
  }
  print_log("open /dev/uio0");

  configfd = open("/sys/class/uio/uio0/device/config", O_RDWR);
  if (configfd < 0) {
    perror("config open:");
    panic("");
  }
  print_log("[init_pci] open /sys/class/uio/uio0/device/config");

  // PCICMD see chipset c220 spec p365
  // Bus Master Enable, Memory Space Enable, I/O Space Enable
  pread(configfd, &buf16, 2, 0x04);
  buf16 |= 0b111;
  pwrite(configfd, &buf16, 2, 0x04);
  print_log("config PCIe CTRL register");

  csrfd = open("/sys/class/uio/uio0/device/resource0", O_RDWR);
  if (csrfd < 0) {
    perror("open");
    panic("");
  }
  print_log("[init_pci] open resource0");

  // Internal registers and memories region is 128KB
  // see spec Table 13-1 PCI Base Address Registers (p271)
  void *base_addr =
      mmap(NULL, 1 << 17, PROT_READ | PROT_WRITE, MAP_SHARED, csrfd, 0);
  if (base_addr == MAP_FAILED) {
    perror("mmap");
    panic("");
  }
  set_regspace(base_addr);
  close(csrfd);
  print_log("[init_pci] map Internal registers and memories(resource0)");
  print_log("[init_pci] regspace = %p", base_addr);
}

void init_dev(void) {
  uint16_t buf16;

  // Disable Interrupts;
  // see spec 14.4 Interrupts Duraing Initialization
  // cf. spec 13.3.32 Interrupt Mask Clear Register
  set_flags(IMC_OFFSET, 0xFFFFFFFF);

  print_log("[init_dev] Disable interrupts");
  usleep(10000);
  print_log("wait 10ms");

  // Global reset
  // see spec chapter 14.5(p490)
  // CHECK: are these really Global reset?
  set_flags(CTRL_OFFSET, CTRL_LRST | CTRL_RST | CTRL_PHY_RST);
  print_log("[init_dev] Global rest(Link reset & Device reset & PHY reset)");

  // Disable Interrupts again;
  // see spec 14.4 Interrupts Duraing Initialization
  set_flags(IMC_OFFSET, 0xFFFFFFFF);
  print_log("[init_dev] Disable interrupt(again)");

  // General Configuration
  // see spec 14.5 Global Reset and General Configuration
  // there seems to be device specific setting (ignore them here)
  // No use of XOFF flow control write 0 to FCAL, FCAH, FCT
  write_reg(FCAL_OFFSET, 0);
  write_reg(FCAH_OFFSET, 0);
  write_reg(FCT_OFFSET, 0);
  print_log("[init_dev] Configure XOFF Flow control(no use)");

  // Setup PHY and Link
  // see spec 14.8 Link Setup Mechanisms and Control/Status Bit Summary

  // PHY Initialization
  // see spec 14.8.1 PHY Initialization
  // see spec 13.3.8 MDI Control Register
  buf16 = read_mdic(MDIC_PCTRL_POFFSET);
  buf16 |= MDIC_PCTRL_PHYRST;
  write_mdic(MDIC_PCTRL_POFFSET, buf16);
  print_log("[init_dev] PHY RESET from MDI");

  // MAC/PHY Link Setup
  // see spec 14.8.2 MAC/PHY Link Setup
  clear_flags(CTRL_OFFSET, CTRL_FRCSPD | CTRL_FRCDPLX);
  set_flags(CTRL_OFFSET, CTRL_SLU | CTRL_RFCE);
  print_log("[init_dev] MAC/PHY Link Setup");

  // Initialize all statistical counters
  // see spec 13.9 Statistics Registers
  // dee spec 14.10 Initialization of Statistics
  usleep(1000);
  for (int offset = 0x04000; offset <= 0x04124; offset += 0x04) {
    read_reg(offset);
  }
  print_log("[init_dev] reset statistical registers");
}

void init_receive(void) {
  // see spec 14.6 Receive Initialization

  // MTA
  // see spec 13.4.1 Multicast Table Array
  for (int i = 0; i < MTA_LEN; i++) {
    write_reg(MTA_OFFSET + i * 4, 0);
  }
  print_log("[Receive] Reset Multicast Table Array");

  // RCTL
  // see spec 13.3.34 Receive Control Register
  // set receive buffer size to 2KB & disable VFE
  clear_flags(RCTL_OFFSET, RCTL_BSIZE_MASK | RCTL_BSEX | RCTL_VFE);
  set_flags(RCTL_OFFSET, RCTL_BSIZE_2K | RCTL_UPE | RCTL_MPE |RCTL_LPE);
  print_log("[Receive] program RCTL");
}

void init_transmit(void) {
  // TXDCTL values GRAN=1 WTHRESH=1(TXDCTL[21:16]) othes=0
  write_reg(TXDCTL_OFFSET, TXDCTL_GRAN | (1 << 16));
  print_log("[Transmit] program TXDCTL");

  // TCTL CT=0x0F(TCTL[11:4]) COLD=0x03F(TCTL[21:12]) PSP=1b EN =1b others = 0
  write_reg(TCTL_OFFSET, (0x0F << 4) | (0x03F << 12) | TCTL_PSP);
  print_log("[Transmit] program TCTL");

  // Program TARC if use multi Txqueues
  // TODO:

  // Program TIPG
  // TODO:
}

void disable_interrupts(uint32_t flags) { set_flags(IMC_OFFSET, flags); }
void enable_interrupts(uint32_t flags) { set_flags(IMS_OFFSET, flags); }
