#pragma once
#include "mem.h"
uint64_t get_mac_addr(void);
void enable_receive(void);
void enable_transmit(void);
void disable_receive(void);
void disable_transmit(void);

/* Receive Descriptor
** Legacy Rx Descriptor (RCTL.DTYPE=00b,RFCTL.EXSTEN = 0b)
** see spec p23 3.2.4 Receive Descriptor Format
** 命名規則は基本的にはspec依存
** 例外
** buffer address -> addr
** packet checksum -> csm
** vlan tag -> vtag
*/
typedef struct rdesc *Rdesc;
struct rdesc {
  phys_addr addr;
  uint16_t length;
  uint16_t csm;
  uint8_t status;
  uint8_t errors;
  uint16_t vtag;
}__attribute__((packed));

/* Transmit Descriptor
** Legacy Transmit Descriptor (デスクリプタのdext ＝ 1bで設定)
** こちらの命名規則も基本的にはspec準拠
** その結果ステータスがstaとなっていたりする。(Rxではstatus)
** reserved はstaにまとめた。
**
*/

typedef struct tdesc *Tdesc;
struct tdesc {
  phys_addr addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t sta;
  uint8_t css;
  uint16_t special;
}__attribute__((packed));


