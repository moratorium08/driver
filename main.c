#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "desc.h"
#include "init.h"
#include "mem.h"
#include "reg.h"
#include "util.h"

int ring_buffer_length = 8;

typedef struct ring_buffer_entry {
    uint64_t addr;
    uint64_t flag;
} ring_buffer_entry;
    
int main(int argc, char const *argv[]) {
  init_pci();
  init_dev();
  init_mem(1024 * 1024 * 2);

  void *memory_base_virt = get_virt_ptr();
  size_t memory_base_phys = get_phys_ptr();

  // initialize receive
  print_log("initialize receive");
  init_receive();
  // TODO: set receive descriptor ring base address and length
  /*  ----------------- 
     [0]
     [1]
     ...
     [7] -> 
     -------------------
     [8] ->
     [8 + 
     ...
     [
   */
  uint64_t ring_base_addr_rd = memory_base_virt;
  uint64_t ring_base_paddr_rd = memory_base_phys;
  write_reg_wrapper(RDLEN_OFFSET, (uint32_t)ring_buffer_length * 16);
  write_reg_wrapper(RDBAH_OFFSET, (uint32_t)(ring_base_paddr_rd >> 32));
  write_reg_wrapper(RDBAL_OFFSET, (uint32_t)(ring_base_paddr_rd &0xffffffff));

  uint64_t ring_buf_base = ring_base_paddr_rd + 16 * ring_buffer_length;
  Rdesc addr = (Tdesc)ring_base_addr_rd;
  for (int i = 0; i < ring_buffer_length; i++) {
      addr[i].addr = ring_buf_base + i * 2048;
      addr[i].length = 64;
  }

  enable_receive();

  // initialize transmit
  print_log("initialize transmit");
  init_transmit();
  // TODO: set transmit descriptor ring base address and length

  // TODO: allocate memory to each transmit descriptors
  // this can be done on transmit
  uint64_t ring_base_addr_td = ring_base_addr_rd + (2048 + 16) * ring_buffer_length;
  uint64_t ring_base_paddr_td = ring_base_paddr_rd + (2048 + 16) * ring_buffer_length;
  write_reg_wrapper(TDLEN_OFFSET, (uint32_t)ring_buffer_length * 16);
  write_reg_wrapper(TDBAH_OFFSET, (uint32_t)(ring_base_paddr_td >> 32));
  write_reg_wrapper(TDBAL_OFFSET, (uint32_t)(ring_base_paddr_td &0xffffffff));

  ring_buf_base = ring_base_paddr_td + 16 * ring_buffer_length;
  Tdesc addr2 = (Tdesc)ring_base_addr_td;
  for (int i = 0; i < ring_buffer_length; i++) {
      addr2[i].addr = ring_buf_base + i * 2048;
      addr2[i].length = 64;
      addr2[i].cso = 0;
      addr2[i].css = 0;
      addr2[i].sta = 0;
      addr2[i].cmd = (1 << 0) | (1 << 1) | (0 << 2) | (1 << 3) | (0 << 4) |
          (0 << 5) | (0 << 6) | (0 << 7);
      addr2[i].special = 0;
  }
  print_log("%d", addr2[0].cmd);

  enable_transmit();
  print_log("enable interrupts");
  enable_interrupts(0xFFFFFFFF);
  usleep(1000);

  // wait until NIC send packet physically 
  int tx_head_prev = read_reg_wrapper(TDT_OFFSET);
  char *buf = ring_base_addr_td + 16 * ring_buffer_length;
  for (int i = 0; i < 2048 * 16; i++) {
      buf[i] = 'A' + (i % 26);
  }
  write_reg_wrapper(TDT_OFFSET, tx_head_prev + 1);
  int tx_head = read_reg_wrapper(TDH_OFFSET);


  enable_receive();
  int rdt_tail_prev = read_reg_wrapper(RDT_OFFSET);
  write_reg_wrapper(RDT_OFFSET, rdt_tail_prev + 1);

  int rx_head_prev = read_reg_wrapper(RDH_OFFSET);
  int rx_head;
  print_log("waiting read buffer");
  while(1) {
      rx_head = read_reg_wrapper(RDH_OFFSET);
      if (rx_head != rx_head_prev) break;
  }
  int *bufr = ring_base_addr_rd + 16 * ring_buffer_length;
  for (int i = 0; i < 2048 / 4; i++) {
      printf("%x", bufr[i]);
  }
  print_log("%d\n", rx_head);
  sleep(2);
  return 0;
}
