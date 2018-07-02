#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "desc.h"
#include "init.h"
#include "mem.h"
#include "reg.h"
#include "util.h"

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

  // receive buffer size is set to 2KB in init_receive
  // TODO: allocate memory to each receivev descriptors

  enable_receive();

  // initialize transmit
  print_log("initialize transmit");
  init_transmit();
  // TODO: set transmit descriptor ring base address and length

  // TODO: allocate memory to each transmit descriptors
  // this can be done on transmit

  enable_transmit();
  print_log("enable interrupts");
  enable_interrupts(0xFFFFFFFF);
  usleep(1000);

  // TODO: program data communicaton

  // wait until NIC send packet physically 
  sleep(2);
  return 0;
}
