#pragma once
#include <stdint.h>




//===========General==========================
#define CTRL_OFFSET 0x0000
#define CTRL_LRST (1 << 3)
#define CTRL_SLU (1 << 6)
#define CTRL_FRCSPD (1 << 11)
#define CTRL_FRCDPLX (1 << 12)
#define CTRL_RST (1 << 26)
#define CTRL_RFCE (1 << 27)
#define CTRL_TFCE (1 << 28)
#define CTRL_PHY_RST (1 << 31)
#define STATUS_OFFSET 0x0008

//================MDIC=======================
// P means PAGE
#define MDIC_OFFSET 0x0020
#define MDIC_PCTRL_POFFSET 0x00
// Auto Negotiation Enable
#define MDIC_PCTRL_ANEN (1 << 12)
// Enable Loopback
#define MDIC_PCTRL_ENLB (1 << 14)
// PHY Reset
#define MDIC_PCTRL_PHYRST (1 << 15)

//================Flow Control================
#define FCAL_OFFSET 0x0028
#define FCAH_OFFSET 0x002C
#define FCT_OFFSET 0x0030
#define FCTTV_OFFSET 0x0170

//================Interrupt===================
#define IMS_OFFSET 0x00D0
#define IMS_RXT (1 << 7)
#define IMS_RXO (1 << 6)
#define IMS_RXDMT (1 << 4)
#define IMS_RXSEQ (1 << 3)
#define IMC_OFFSET 0x00D8

//================Receive=====================
#define RCTL_OFFSET 0x0100
#define RCTL_EN (1 << 1)
#define RCTL_SBP (1 << 2)  // Store Bad Packet
#define RCTL_UPE (1 << 3)
#define RCTL_MPE (1 << 4)
#define RCTL_LPE (1 << 5)
#define RCTL_DTYPE_MASK (0b11 << 10)
#define RCTL_DTYPE_LEGACY (0b00 << 10)
#define RCTL_DTYPE_PS (0b01 << 10)
#define RCTL_BSIZE_MASK (0b11 << 16)
#define RCTL_BSIZE_2K (0b00 << 16)
#define RCTL_BSIZE_1K (0b01 << 16)
#define RCTL_VFE (1 << 18)  // VLAN Filter Enable
#define RCTL_BSEX (1 << 25)

#define RDBAL_OFFSET 0x2800
#define RDBAH_OFFSET 0x2804
#define RDLEN_OFFSET 0x2808
#define RDH_OFFSET 0x2810
#define RDT_OFFSET 0x2818
#define RDTR_OFFSET 0x2820
#define RXDCTL_OFFSET 0x2828

//================Transmit===================
#define TCTL_OFFSET 0x0400
#define TCTL_EN (1 << 1)
#define TCTL_PSP (1 << 3)
#define TDBAL_OFFSET 0x3800
#define TDBAH_OFFSET 0x3804
#define TDLEN_OFFSET 0x3808
#define TDH_OFFSET 0x3810
#define TDT_OFFSET 0x3818
#define TXDCTL_OFFSET 0x3828
#define TXDCTL_GRAN (1 << 24)
#define TIDV_OFFSET 0x3820

//================Filter Registers============
#define MTA_OFFSET 0x5200
#define MTA_LEN    128
#define RAL_OFFSET 0x5400
#define RAH_OFFSET 0x5404


#ifdef DANGER
void set_regspace(void *addr);

uint32_t read_reg(uint32_t off);
void write_reg(uint32_t off, uint32_t val);

void set_flags(uint32_t off, uint32_t val);
void clear_flags(uint32_t off, uint32_t val);

uint16_t read_mdic(uint32_t poff);
void write_mdic(uint32_t poff, uint16_t val);
#endif

uint32_t read_reg_wrapper(uint32_t off);
void write_reg_wrapper(uint32_t off, uint32_t val);