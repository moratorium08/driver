#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
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

typedef enum {
    ETYPE_IPv4,
    ETYPE_ARP,
    ETYPE_RARP,
    ETYPE_UNKNOWN
} EthernetType;

typedef unsigned long long mac_addr;

typedef enum {
    ARPHTYPE_ETHERNET,
    ARPHTYPE_UNKNOWN
} ARPHType;

typedef enum {
    ARPPTYPE_IPv4,
    ARPPTYPE_UNKNOWN
} ARPPType;

typedef enum {
    ARPOPERATION_REQUEST = 1,
    ARPOPERATION_REPLY = 2,
    ARPOPERATION_UNKNOWN = 3
} ARPOperation;

typedef struct {
    ARPHType hardware_type;
    ARPPType protocol_type;
    ARPOperation operation;

    // 多分ここは場合によって大きさが異なる？
    union {
        struct {
            int sender_ip_addr;
            int recver_ip_addr;
        };
    };
    union {
        struct {
            mac_addr sender_mac_addr;
            mac_addr recver_mac_addr;
        };
    };
    
} ARP;

typedef struct {
    mac_addr recver_mac;
    mac_addr sender_mac;
    EthernetType type;
    union {
        ARP *arp;
        char *data;
    };
} Ethernet;


ARP * parse_arp(unsigned char *raw) {
    ARP *arp = (ARP *)malloc(sizeof(ARP));
    memset(arp, 0, sizeof(ARP));

    int tmp;
    tmp = raw[0] * 0x100 + raw[1];
    switch (tmp) {
        case 1:
            arp->hardware_type = ARPHTYPE_ETHERNET;
            break;
        default:
            arp->hardware_type = ARPHTYPE_UNKNOWN;
    }

    tmp = raw[2] * 0x100 + raw[3];
    if (tmp >= 0x800) {
        arp->protocol_type = ARPPTYPE_IPv4;
    } else {
        arp->protocol_type = ARPPTYPE_UNKNOWN;
    }

    tmp = raw[6] * 0x100 + raw[7];
    switch (tmp) {
        case 1:
            arp->operation = ARPOPERATION_REQUEST;
            break;
        case 2:
            arp->operation = ARPOPERATION_REPLY;
            break;
        default:
            arp->operation = ARPOPERATION_UNKNOWN;
    }

    int hardware_size = raw[4];
    int protocol_size = raw[5];

    unsigned long long addr;
    switch (arp->hardware_type) {
        case ARPHTYPE_ETHERNET:
            addr = 0;
            for (int i = 0; i < 6; i++) {
                addr = addr * 0x100 + raw[8 + i];
            }
            arp->sender_mac_addr = addr;

            addr = 0;
            for (int i = 0; i < 6; i++) {
                addr = addr * 0x100 + raw[8 + i + hardware_size + protocol_size];
            }
            arp->recver_mac_addr = addr;
            break;
        default:
            // nop
            break;
    }

    switch (arp->protocol_type) {
        case ARPPTYPE_IPv4:
            addr = 0;
            for (int i = 0; i < 4; i++) {
                addr = addr * 0x100 + raw[8 + i + hardware_size];
            }
            arp->sender_ip_addr= addr;

            addr = 0;
            for (int i = 0; i < 4; i++) {
                addr = addr * 0x100 + raw[8 + i + protocol_size +
                    2 * hardware_size];
            }
            arp->recver_ip_addr = addr;
            break;
        default:
            //nop
            break;
    }

    return arp;
}

Ethernet * parse_packet(unsigned char *raw) {
    Ethernet *e = (Ethernet *)malloc(sizeof(Ethernet));
    memset(e, 0, sizeof(Ethernet));
    mac_addr recv = 0;
    for (int i = 0; i < 6; i++) {
        recv = recv * 0x100 + (unsigned int)raw[i];
    }

    mac_addr sender = 0;
    for (int i = 0; i < 6; i++) {
        sender = sender * 0x100 + (unsigned int)raw[6 + i];
    }
    int ttype = (unsigned int)raw[12] * 0x100 + (unsigned int)raw[13];
    EthernetType t;
    switch (ttype) {
        case 0x0800:
            t = ETYPE_IPv4;
            break;
        case 0x0806:
            t = ETYPE_ARP;
            break;
        case 0x8036:
            t = ETYPE_RARP;
            break;
        default:
            t = ETYPE_UNKNOWN;
    }
    e->recver_mac = recv;
    e->sender_mac = sender;
    e->type = t;
    switch (e->type) {
        case ETYPE_ARP:
            e->arp = parse_arp(raw + 14);
            break;
        default:
            e->data = raw + 14;
    }
    return e;
}

void print_arp_packet(ARP *arp) {
    printf("Hardware Type: ");
    switch (arp->hardware_type) {
        case ARPHTYPE_ETHERNET:
            printf("Ethernet\n");
            break;
        default:
            printf("Unknown\n");
    }

    printf("Protocol Type: ");
    switch (arp->protocol_type) {
        case ARPPTYPE_IPv4:
            printf("IPv4\n");
            break;
        default:
            printf("Unknown\n");
    }

    printf("Operation: ");
    switch (arp->operation) {
        case ARPOPERATION_REQUEST:
            printf("Request\n");
            break;
        case ARPOPERATION_REPLY:
            printf("Reply\n");
            break;
        default:
            printf("Unknown\n");
    }

    // TBD
    printf("Sender Hardware Addr: %llx\n", arp->sender_mac_addr);
    printf("Sender Protocol Addr: %x\n", arp->sender_ip_addr);
    
    printf("Receiver Hardware Addr: %llx\n", arp->recver_mac_addr);
    printf("Receiver Protocol Addr: %x\n", arp->recver_ip_addr);
}

void print_packet(Ethernet *e) {
    printf("RECEIVER: %llx", e->recver_mac);
    printf("\tSENDER: %llx\n", e->sender_mac);
    switch (e->type) {
        case ETYPE_IPv4:
            printf("Type: IPv4\n");
            break;
        case ETYPE_ARP:
            printf("Type: ARP\n");
            print_arp_packet(e->arp);
            break;
        case ETYPE_RARP:
            printf("Type: RARP\n");
            break;
        default:
            printf("Type: Unknown\n");
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

void dump_data(unsigned char *bufr) {
      for (int i = 0; i < 64; i++) {
          print_hex(bufr[i]);
          printf(" ");
          if (i % 8 == 7) {
              printf("\n");
          }
      }
      printf("\n\n");
}

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
      Ethernet *e = parse_packet(bufr);
      if (e->type == ETYPE_ARP) {
          print_packet(e);
          dump_data(bufr);
      }
  }
  sleep(2);
  return 0;
}
