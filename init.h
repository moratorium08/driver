#pragma once
// pciを初期化
void init_pci(void);
void init_dev(void);
void init_receive(void);
void init_transmit(void);
void enable_interrupts(uint32_t);
void disable_interrupts(uint32_t);
