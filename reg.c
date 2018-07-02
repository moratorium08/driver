#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define DANGER
#include "reg.h"
#include "checker.h"
#include "util.h"

static void *regspace;


void set_regspace(void *addr) { regspace = addr; }

uint32_t read_reg_wrapper(uint32_t off){
  switch(off){
    case RDBAL_OFFSET:
    case RDBAH_OFFSET:
    case RDLEN_OFFSET:
    case RDH_OFFSET:
    case RDT_OFFSET:
    case RAL_OFFSET:
    case RAH_OFFSET:
    case TDBAL_OFFSET:
    case TDBAH_OFFSET:
    case TDLEN_OFFSET:
    case TDH_OFFSET:
    case TDT_OFFSET:break;
    default:panic("this register is not allowed to access");
  }
   return read_reg(off);
}
void write_reg_wrapper(uint32_t off, uint32_t val){
  switch(off){
    case RDBAL_OFFSET:
    case RDBAH_OFFSET:
        check_before_set_rdba(CHECK_LEVEL);
        break;
    case RDLEN_OFFSET:
        check_before_set_rdlen(CHECK_LEVEL);
        break;
    case RDH_OFFSET:
        check_set_rqueue_head(CHECK_LEVEL, val);
        break;
    case RDT_OFFSET:
        check_set_rqueue_tail(CHECK_LEVEL, val);
        break;
    case TDBAL_OFFSET:
    case TDBAH_OFFSET:
        check_before_set_tdba(CHECK_LEVEL);
        break;
    case TDLEN_OFFSET:
        check_before_set_tdlen(CHECK_LEVEL);
        break;
    case TDH_OFFSET:
        check_set_tqueue_head(CHECK_LEVEL, val);
        break;
    case TDT_OFFSET:
        check_set_tqueue_tail(CHECK_LEVEL, val);break;
    default:panic("this register is not allowed to access");
  }
  write_reg(off,val);
}



void write_reg(uint32_t off, uint32_t val) {
  ((uint32_t *)regspace)[off / 4] = val;
}
uint32_t read_reg(uint32_t off) {
  return ((uint32_t *)regspace)[off / 4];
}

void set_flags(uint32_t off, uint32_t flags) {
  uint32_t buf32;
  buf32 = read_reg(off);
  buf32 |= flags;
  write_reg(off, buf32);
}
void clear_flags(uint32_t off, uint32_t flags) {
  uint32_t buf32;
  buf32 = read_reg(off);
  buf32 &= ~flags;
  write_reg(off, buf32);
}

void write_mdic(uint32_t page, uint16_t val) {
  uint32_t buf = 0;
  buf |= val;
  buf |= (page & ((1 << 17) - 1)) << 16;  // set Page addr
  buf |= 1 << 21;
  buf |= 1 << 26;  // OP = Write
  buf = 0;
  write_reg(MDIC_OFFSET, buf);
  // Wait until ready
  while (1) {
    buf = read_reg(MDIC_OFFSET);
    if (buf & (1 << 28)) break;
  }
}



uint16_t read_mdic(uint32_t page) {
  uint32_t buf = 0;
  buf |= (page & ((1 << 17) - 1)) << 16;  // set Page addr
  buf |= 1 << 21;
  buf |= 1 << 27;  // OP = Read
  write_reg(MDIC_OFFSET, buf);
  // Wait until ready
  buf = 0;
  while (1) {
    buf = read_reg(MDIC_OFFSET);
    if (buf & (1 << 28)) break;
  }
  return (uint16_t)(buf & 0xFFFF);

}
