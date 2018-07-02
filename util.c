#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define DANGER
#include "reg.h"

#include "util.h"

#define DUMPREG(X) {X ## _OFFSET, #X}
struct {
  uint32_t offset;
  const char *name;
} dump_regs[] = {
  DUMPREG(CTRL),
  DUMPREG(STATUS),
  DUMPREG(RCTL),
  DUMPREG(RDBAL),
  DUMPREG(RDBAH),
  DUMPREG(RDLEN),
  DUMPREG(RDH),
  DUMPREG(RDT),
  DUMPREG(RDTR),
  DUMPREG(RXDCTL),
  DUMPREG(RAL),
  DUMPREG(RAH),
  DUMPREG(TCTL),
  DUMPREG(TDBAL),
  DUMPREG(TDBAH),
  DUMPREG(TDLEN),
  DUMPREG(TDH),
  DUMPREG(TDT),
  DUMPREG(TXDCTL),
  DUMPREG(TIDV),
};
#undef DUMPREG

void dump_registers() {
  for(size_t i=0; i<sizeof(dump_regs)/sizeof(dump_regs[0]); i++) {
    uint32_t val;
    val = read_reg(dump_regs[i].offset);
    printf("%10s(%08x): %08x\n", dump_regs[i].name, dump_regs[i].offset, val);
  }
}


void panic(const char *str) {
  fprintf(stderr, "[ERROR]:%s\n", str);
  exit(-1);
}

void print_log(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  fprintf(stderr, "[LOG]:");
  vfprintf(stderr, format, arg);
  fprintf(stderr, "\n");
}
