#include <stdio.h>
#include <stdint.h>
#include "mem.h"
#define DANGER
#include "reg.h"
#include "desc.h"
#include "checker.h"

uint64_t get_mac_addr(void) {
  static uint64_t mac_addr = 0;
  if (mac_addr == 0) {
    uint32_t buf32;
    buf32 = read_reg_wrapper(RAL_OFFSET);
    mac_addr |= buf32;
    buf32 = read_reg_wrapper(RAH_OFFSET);
    mac_addr |= ((uint64_t)buf32 & 0xFFFFll) << 32;
  }
  return mac_addr;
}

void enable_receive(void) {
  check_before_rqueue_enable(CHECK_LEVEL);
  set_flags((uint32_t)RCTL_OFFSET, (uint32_t)RCTL_EN);
}
void enable_transmit(void) {
  check_before_tqueue_enable(CHECK_LEVEL);
  set_flags((uint32_t)TCTL_OFFSET, (uint32_t)TCTL_EN);
}
void disable_receive(void) { clear_flags((uint32_t)RCTL_OFFSET, (uint32_t)RCTL_EN); }
void disable_transmit(void) { clear_flags((uint32_t)TCTL_OFFSET, (uint32_t)TCTL_EN); }
