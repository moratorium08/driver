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
char *buf_td;
char *buf_rd; 

typedef struct ring_buffer_entry {
    uint64_t addr;
    uint64_t flag;
} ring_buffer_entry;

char *read_a_packet() {
  int rx_head_prev = read_reg_wrapper(RDH_OFFSET);
  int rx_head;

  int rdt_tail_prev = read_reg_wrapper(RDT_OFFSET);
  if (rdt_tail_prev == ring_buffer_length - 1) {
      write_reg_wrapper(RDT_OFFSET, 0);
  } else {
      write_reg_wrapper(RDT_OFFSET, rdt_tail_prev + 1);
  }
  while(1) {
      rx_head = read_reg_wrapper(RDH_OFFSET);
      if (rx_head != rx_head_prev) break;
  }
  return buf_rd + 2048 * rdt_tail_prev;
}

void write_a_packet(char *buf) {
  int tx_head_prev = read_reg_wrapper(TDH_OFFSET) % ring_buffer_length;
  int tx_tail_prev = read_reg_wrapper(TDT_OFFSET);
  for (int j = 0, i = 2048 * tx_head_prev; i < 2048 * (tx_head_prev + 1);
          i++, j++) {
      buf_td[i] = buf[j];
  }
  if (tx_tail_prev == ring_buffer_length - 1) {
      write_reg_wrapper(TDT_OFFSET, 0);
  } else {
      write_reg_wrapper(TDT_OFFSET, tx_tail_prev + 1);
  }
}

void print_val(char x) {
    if (x < 10) {
        printf("%c", x + '0');
    } else {
        printf("%c", x + 'a' - 10);
    }
}

void print_hex(unsigned char c) {
    print_val(c / 0x10);
    print_val(c % 0x10);
}

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


  // setup buffer of rd and td
  buf_td = ring_base_addr_td + 16 * ring_buffer_length;
  buf_rd = ring_base_addr_rd + 16 * ring_buffer_length;

  enable_transmit();
  print_log("enable interrupts");
  enable_interrupts(0xFFFFFFFF);


  // wait until NIC send packet physically 
  char buf[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDCEFGHIJKLMNOPQRSTUVWXYZABCDEFG";
  for (int i = 0; i < 26; i++) {
      printf("%d\n", i);
      buf[0] = 'A' + i;
      write_a_packet(buf);
  }

  print_log("waiting read buffer");
  for (int i = 0; i < 1000; i++) {
      unsigned char *bufr = read_a_packet();
      for (int i = 0; i < 64; i++) {
          print_hex(bufr[i]);
      }
      printf("\n\n");
  }
  sleep(2);
  return 0;
}
