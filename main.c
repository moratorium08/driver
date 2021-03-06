#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "dhcp.h"
#include "desc.h"
#include "init.h"
#include "mem.h"
#include "reg.h"
#include "util.h"

#define IPADDR 0xc0a8000b
#define MACADDR 0xf8cab84be88bull

int ring_buffer_length = 150;
int delta_dt = 20;
int entry_size = 350;
char *buf_td;
char *buf_rd;
Tdesc ring_base_addr_rd_p;
Tdesc ring_base_addr_td_p;

typedef struct ring_buffer_entry { uint64_t addr;
    uint64_t flag;
} ring_buffer_entry;

// --- util ---
// big endians
byte read_byte(byte *raw) {
    return raw[0];
}

unsigned int read_2bytes(byte *raw) {
    return read_byte(raw) * 0x100 + read_byte(raw + 1);
}

unsigned int read_4bytes(byte *raw) {
    unsigned int x = 0;
    for (int i = 0; i < 4; i++) {
        x *= 0x100;
        x += read_byte(raw + i);
    }
    return x;
}
unsigned long long read_bytes(byte *raw, int n) {
    unsigned long long x = 0;
    for (int i = 0; i < n; i++) {
        x *= 0x100;
        x += read_byte(raw + i);
    }
    return x;
}
unsigned long long read_6bytes(byte *raw) {
    return read_bytes(raw, 6);
}
unsigned long long read_8bytes(byte *raw) {
    return read_bytes(raw, 8);
}
// ------------------

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
    unsigned int sender_port;
    unsigned int recver_port;
    unsigned int length;
    char *data;
} UDP;

typedef enum {
    IPV4_SERVICE_PRIORITY_0   = 1,
    IPV4_SERVICE_PRIORITY_1   = 1 << 1,
    IPV4_SERVICE_PRIORITY_2   = 1 << 2,
    IPV4_SERVICE_DELAY        = 1 << 3,
    IPV4_SERVICE_TRANSPORT    = 1 << 4,
    IPV4_SERVICE_RELIABILITY  = 1 << 5,
    IPV4_SERVICE_RESERVED_0   = 1 << 6,
    IPV4_SERVICE_RESERVED_1   = 1 << 7,
} IPV4_SERVICE;

typedef enum {
    IPV4_FLAG_RESERVED = 1,
    IPV4_FLAG_INHIVITED = 1 << 1,
    IPV4_FLAG_CONTINUATION = 1 << 2
} IPV4_FLAG;

typedef enum {
    IP_PROTOCOL_UDP,
    IP_PROTOCOL_UNKNOWN
} IP_PROTOCOL;

typedef struct {
    unsigned long long sender_addr;
    unsigned long long recver_addr;
    IP_PROTOCOL protocol;
    int length;
    union {
        UDP *udp;
        char *data;
    };
} IPv4;

typedef struct {
    mac_addr recver_mac;
    mac_addr sender_mac;
    EthernetType type;
    union {
        ARP *arp;
        IPv4 *ipv4;
        char *data;
    };
} Ethernet;

void save_big_endian(unsigned long long val, unsigned char *p, int size) {
    for (int i = 0;  i < size; i++) {
        p[size - i - 1] = val % 0x100;
        val = val / 0x100;
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

void dump_data_size(unsigned char *bufr, int s) {
      for (int i = 0; i < s; i++) {
          print_hex(bufr[i]);
          printf(" ");
          if (i % 24 == 23) {
              printf("\n");
          }
      }
      printf("\n\n");
}

void dump_data(unsigned char *bufr) {
    dump_data_size(bufr, entry_size);
}

int gen_udp_packet(UDP *udp, unsigned char *raw) {
    save_big_endian(udp->sender_port, raw, 2);
    save_big_endian(udp->recver_port, raw + 2, 2);
    save_big_endian(udp->length, raw + 4, 2);

    raw[6] = 0;
    raw[7] = 0;

    memcpy(raw + 8, udp->data, udp->length);
    return 0;
}

int ipv4_add(unsigned int x, unsigned int y) {
    int z = x + y;
    if (z > 0xffff) {
        return ipv4_add(z % 0x10000, z / 0x10000);
    } else {
        return z;
    }
}

int calc_ipv4_checksum(unsigned char *raw) {
    int tmp = 0;
    for (int i = 0; i < 10; i++) {
        unsigned int x = 0x100 * raw[2 * i] + raw[2 * i + 1];
        tmp = ipv4_add(tmp, x);
    }
    return tmp ^ 0xffff;
}


int gen_ipv4_packet(IPv4 *ipv4, unsigned char *raw) {
    raw[0] = 0x45;
    raw[1] = 0x00;
    save_big_endian(ipv4->length, raw + 2, 2);
    raw[4] = 0xde;
    raw[5] = 0xad;
    raw[6] = 0x00;
    raw[7] = 0x00;
    raw[8] = 0x40;
    switch (ipv4->protocol) {
        case IP_PROTOCOL_UDP:
            raw[9] = 17;
            if(gen_udp_packet(ipv4->udp, raw + 20)) return 2;
            break;
        default:
            return 1;
    }
    raw[10] = 0;
    raw[11] = 0;
    save_big_endian(ipv4->sender_addr, raw + 12, 4);
    save_big_endian(ipv4->recver_addr, raw + 16, 4);
    int checksum = calc_ipv4_checksum(raw);
    save_big_endian(checksum, raw + 10, 2);
    return 0;
}

// if success then returns 0
int gen_arp_packet(ARP *arp, unsigned char *raw) {
    int hardware_size, protocol_size;
    switch (arp->hardware_type){
        case ARPHTYPE_ETHERNET:
            hardware_size = 6;
            break;
        default:
            return 4;
    }

    switch (arp->protocol_type){
        case ARPPTYPE_IPv4:
            protocol_size = 4;
            break;
        default:
            return 5;
    }

    switch (arp->hardware_type) {
        case ARPHTYPE_ETHERNET:
            raw[0] = 0;
            raw[1] = 1;
            save_big_endian(arp->sender_mac_addr, raw + 8, hardware_size);
            save_big_endian(
                    arp->recver_mac_addr, raw + 8 + hardware_size + protocol_size,
                    hardware_size);
            break;
        default:
            return 1;
    }

    switch (arp->protocol_type) {
        case ARPHTYPE_ETHERNET:
            raw[2] = 0x8;
            raw[3] = 0x00;
            save_big_endian(arp->sender_ip_addr, raw + 8 + hardware_size, protocol_size);
            save_big_endian(
                    arp->recver_ip_addr, raw + 8 + 2 * hardware_size + protocol_size,
                    protocol_size);
            break;
        default:
            return 2;
    }

    switch (arp->operation) {
        case ARPOPERATION_REQUEST:
            raw[6] = 0;
            raw[7] = 1;
            break;
        case ARPOPERATION_REPLY:
            raw[6] = 0;
            raw[7] = 2;
            break;
        default:
            return 3;
    }
    save_big_endian(hardware_size, raw + 4, 1);
    save_big_endian(protocol_size, raw + 5, 1);

    return 0;
}

int gen_ethernet_packet(Ethernet *e, unsigned char *raw) {
    save_big_endian(e->recver_mac, raw, 6);
    save_big_endian(e->sender_mac, raw + 6, 6);
    switch (e->type) {
        case ETYPE_IPv4:
            save_big_endian(0x800, raw + 12, 2);
            break;
        case ETYPE_ARP:
            save_big_endian(0x806, raw + 12, 2);
            break;
        case ETYPE_RARP:
            save_big_endian(0x8036, raw + 12, 2);
            break;
        default:
            return 1;
    }
    int ret;
    switch(e->type) {
        case ETYPE_ARP:
            if ((ret = gen_arp_packet(e->arp, raw + 14)) != 0) {
                print_log("[error] ID: %d", ret);
            }
            break;
        case ETYPE_IPv4:
            if ((ret = gen_ipv4_packet(e->ipv4, raw + 14)) != 0) {
                print_log("[error] ID: %d", ret);
            }
            break;
        default:
            return 2;
    }
    return 0;
}

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

void free_arp(ARP *arp) {
}



UDP *parse_udp(unsigned char *raw) {
    UDP *udp = (UDP *)malloc(sizeof(UDP));
    memset(udp,  0, sizeof(UDP));
    unsigned int addr = 0;
    for (int i = 0; i < 2; i++) {
        addr = addr * 0x100 + raw[0 + i];
    }
    udp->sender_port = addr;

    addr = 0;
    for (int i = 0; i < 2; i++) {
        addr = addr * 0x100 + raw[2 + i];
    }
    udp->recver_port = addr;
    addr = 0;
    for (int i = 0; i < 2; i++) {
        addr = addr * 0x100 + raw[4 + i];
    }
    udp->length = addr;
    udp->data = raw + 8;
    return udp;
}

void free_udp(UDP *udp) {
    free(udp);
}


IPv4 *parse_ipv4(unsigned char *raw) {
    IPv4 *ipv4 = (IPv4*)malloc(sizeof(IPv4));
    memset(ipv4, 0, sizeof(IPv4));

    unsigned int checksum = raw[10] * 0x100 + raw[11];
    raw[10] = 0;
    raw[11] = 0;
    printf("checksum: %x, %x\n", checksum, calc_ipv4_checksum(raw));

    unsigned long long addr = 0;
    for (int i = 0; i < 4; i++) {
        addr = addr * 0x100 + raw[12 + i];
    }
    ipv4->sender_addr = addr;

    addr = 0;
    for (int i = 0; i < 4; i++) {
        addr = addr * 0x100 + raw[16 + i];
    }
    ipv4->recver_addr = addr;

    addr = 0;
    for (int i = 0; i < 1; i++) {
        addr = addr * 0x100 + raw[9 + i];
    }


    if (addr == 17) {
        ipv4->protocol = IP_PROTOCOL_UDP;
        ipv4->udp = parse_udp(raw + 20);
    } else {
        ipv4->protocol = IP_PROTOCOL_UNKNOWN;
        ipv4->data = raw + 20;
    }

    addr = 0;
    for (int i = 0; i < 2; i++) {
        addr = addr * 0x100 + raw[2 + i];
    }
    ipv4->length = addr;

    return ipv4;
}

int gen_dhcp_packet(DHCP *d, byte * buf) {
    printf("%p, %p\n", d, buf);
    save_big_endian(d->op, buf, 1);
    save_big_endian(d->htype, buf + 1, 1);
    save_big_endian(d->hlen, buf + 2, 1);
    save_big_endian(d->ops, buf + 3, 1);
    save_big_endian(d->xid, buf + 4, 4);
    save_big_endian(d->secs, buf + 8, 2);
    save_big_endian(d->flags, buf + 10, 2);
    save_big_endian(d->ciaddr, buf + 12, 4);
    save_big_endian(d->yiaddr, buf + 16, 4);
    save_big_endian(d->siaddr, buf + 20, 4);
    save_big_endian(d->giaddr, buf + 24, 4);
    save_big_endian(d->chaddr, buf + 28, 6);

    byte *options = buf + 28 + 16 + 64 + 128;
    save_big_endian(0x63825363, options, 4);

    save_big_endian(0x3501, options + 4, 2);
    save_big_endian(d->type,  options + 6, 1);

    save_big_endian(0x0104, options + 7, 2);
    save_big_endian(0xffffff00, options + 9, 4);
    
    save_big_endian(0x0304, options + 13, 2);
    save_big_endian(IPADDR, options + 15, 4);

    save_big_endian(0x3304, options + 19, 2);
    save_big_endian(8000, options + 21, 4);

    save_big_endian(0x0604, options + 25, 2);
    save_big_endian(IPADDR, options + 27, 4);
    
    save_big_endian(0x0f04, options + 31, 2);
    save_big_endian(0x686f6765, options + 33, 4);

    save_big_endian(0x3604, options + 37, 2);
    save_big_endian(IPADDR, options + 39, 4);

    save_big_endian(0xff,  options + 43, 1);
    return 28 + 16 + 64 + 128 + 64;
}


DHCP *parse_dhcp(byte *raw) {
    DHCP *d =  (DHCP *)malloc(sizeof(DHCP));
    memset(d, 0, sizeof(DHCP));

    printf("raw: %p\n", raw);

    d->op = read_byte(raw);
    d->htype = read_byte(raw + 1);
    d->hlen = read_byte(raw + 2);
    d->ops = read_byte(raw + 3);
    d->xid = read_4bytes(raw + 4);
    d->secs = read_2bytes(raw + 8);
    d->flags = read_2bytes(raw + 10);
    d->ciaddr = read_4bytes(raw + 12);
    d->yiaddr = read_4bytes(raw + 16);
    d->siaddr = read_4bytes(raw + 20);
    d->giaddr = read_4bytes(raw + 24);
    d->chaddr = read_6bytes(raw + 28);

    byte *options = raw + 28 + 16 + 64 + 128;
    printf("Debug: magic is %x\n", read_4bytes(options));
    options += 4;
    int cnt = 0;
    int idx = 0;
    while (1) {
        if (cnt >= 64) {
            break;
        }
        byte type = read_byte(options);
        if (type == 0xff) {
            break;
        }
        if (type == 53) {
            byte dhcp_type = read_byte(options + 2);
            d->type = dhcp_type;
            options += 3;
            cnt += 3;
        } else {
            byte len = read_byte(options + 1);
            byte *buf = (byte *)malloc(len);
            memcpy(buf, options + 2, len);

            d->options[idx].type = type;
            d->options[idx].len = len;
            d->options[idx].data = buf;
            d->option_num++;
            idx++;

            options += len + 2;
            cnt += len + 2;
        }
    }
    return d;
}

void print_dhcp(DHCP *d) {
    printf("[DHCP]\nType: ");
    switch (d->type) {
        case DHCPDiscover:
            printf("Discover");
            break;
        case DHCPOffer:
            printf("Offer");
            break;
        case DHCPRequest:
            printf("Request");
            break;
        case DHCPAck:
            printf("Ack");
            break;
        default:
            printf("Unknown");
    }

    printf("\n");
    printf("[OP]%d, [htype]%d, [hlen]%d,[hops] %d\n", d->op, d->htype, d->hlen,
            d->ops);
    printf("[SECS]%d, [FLAGS]%d\n", d->secs, d->flags);
    printf("TRANID: %x\n", d->xid);
    printf("CIADDR: %x\n", d->ciaddr);
    printf("YIADDR: %x\n", d->yiaddr);
    printf("SIADDR: %x\n", d->siaddr);
    printf("GIADDR: %x\n", d->giaddr);
    printf("HCADDR: %llx\n", d->chaddr);
    for (int i = 0; i < d->option_num; i++) {
        printf(" [TYPE] %x\n", d->options[i].type);
        printf(" ");
        dump_data_size(d->options[i].data, d->options[i].len);
    }
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
        case ETYPE_IPv4:
            e->ipv4 = parse_ipv4(raw + 14);
            break;
        default:
            e->data = raw + 14;
    }
    return e;
}

void print_udp_packet(UDP *udp) {
    printf("Sender port: %llx\n",  udp->sender_port);
    printf("Recver port: %llx\n",  udp->recver_port);
    for (int i = 0; i < 30; i++) {
        if (udp->data[i] == '\n') break;
        if (udp->data[i] == '\0') break;
        printf("%c", udp->data[i]);
    }
    printf("\n");
}

void print_ipv4_packet(IPv4 *ipv4) {
    printf("Sender addr: %llx\n",  ipv4->sender_addr);
    printf("Recver addr: %llx\n",  ipv4->recver_addr);
    switch (ipv4->protocol) {
        case IP_PROTOCOL_UDP:
            printf("Protocol: UDP\n");
            print_udp_packet(ipv4->udp);
            break;
        default:
            printf("Protocol: Unknown\n");
    }
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
            print_ipv4_packet(e->ipv4);
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



char *read_a_packet() {
  int rx_head_prev = read_reg_wrapper(RDH_OFFSET);
  int rx_head;

  int rdt_tail_prev = read_reg_wrapper(RDT_OFFSET);

  // 怠惰
  while ((rx_head_prev - 1) == rdt_tail_prev ||
          (rx_head_prev == 0 && rdt_tail_prev == (ring_buffer_length - 1))) {
     rx_head_prev = read_reg_wrapper(RDH_OFFSET);
  }

  int next_rx = (rx_head_prev + delta_dt) % ring_buffer_length;
  write_reg_wrapper(RDT_OFFSET, next_rx);

  while(1) {
      rx_head = read_reg_wrapper(RDH_OFFSET);
      if (rx_head != rx_head_prev) break;
  }
  return buf_rd + 2048 * rx_head_prev;
}

void write_a_packet(char *buf) {
  int tx_head_prev = read_reg_wrapper(TDH_OFFSET) % ring_buffer_length;
  int tx_tail_prev = read_reg_wrapper(TDT_OFFSET);

  for (int j = 0, i = 2048 * tx_head_prev; i < 2048 * (tx_head_prev + 1);
          i++, j++) {
      buf_td[i] = buf[j];
  }

  int next_tx = (tx_head_prev + delta_dt) % ring_buffer_length;
  write_reg_wrapper(TDT_OFFSET, next_tx);
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
  ring_base_addr_rd_p = addr;
  for (int i = 0; i < ring_buffer_length; i++) {
      addr[i].addr = ring_buf_base + i * 2048;
      addr[i].length = entry_size;
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
  ring_base_addr_td_p = addr2;
  for (int i = 0; i < ring_buffer_length; i++) {
      addr2[i].addr = ring_buf_base + i * 2048;
      addr2[i].length = entry_size;
      addr2[i].cso = 0;
      addr2[i].css = 0;
      addr2[i].sta = 0;
      addr2[i].cmd = (1 << 0) | (1 << 1) | (0 << 2) | (1 << 3) | (0 << 4) |
          (0 << 5) | (0 << 6) | (0 << 7);
      addr2[i].special = 0;
  }

  // setup buffer of rd and td
  buf_td = ring_base_addr_td + 16 * ring_buffer_length;
  buf_rd = ring_base_addr_rd + 16 * ring_buffer_length;

  enable_transmit();
  print_log("enable interrupts");
  enable_interrupts(0xFFFFFFFF);


  init_dhcp(IPADDR/0x100, IPADDR);
  // wait until NIC send packet physically
  /* test send / recv */

  /*
  char buf[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDCEFGHIJKLMNOPQRSTUVWXYZABCDEFG";
  for (int i = 0; i < 8; i++) {
      printf("%d\n", i);
      buf[0] = 'A' + i;
      write_a_packet(buf);
      sleep(1);
  }
  for (int i = 0; i < 8; i++) {
      printf("%d\n", i);
      write_a_packet(buf);
      sleep(1);
  }

  print_log("waiting read buffer");
  for (int i = 0; i < 1000; i++) {
      unsigned char *bufr = read_a_packet();
      Ethernet *e = parse_packet(bufr);
      if (e->type == ETYPE_ARP) {
          print_packet(e);
          dump_data(bufr);
      }
  }*/


  // arp reqeuest / reply
  while(1) {
      unsigned char *bufr = read_a_packet();
      Ethernet *e = parse_packet(bufr);
      if(e->type == ETYPE_ARP) {
          print_packet(e);
          dump_data(bufr);
          mac_addr tmp = e->recver_mac;
          e->recver_mac = e->sender_mac;
          e->sender_mac = MACADDR;
          e->arp->sender_mac_addr = e->sender_mac;
          e->arp->recver_mac_addr = e->recver_mac;
          e->arp->recver_ip_addr = e->arp->sender_ip_addr;
          e->arp->sender_ip_addr = IPADDR;
          e->arp->operation = ARPOPERATION_REPLY;

          char buf[100];
          memset(buf, 0, 100);
          gen_ethernet_packet(e, buf);
          print_packet(e);
          write_a_packet(buf);
          dump_data(buf);
          printf("\n\n");
      }
      else if (e->type == ETYPE_IPv4) {
          print_packet(e);


          char buf[1024];
          memset(buf, 0, 1024);

          e->recver_mac = e->sender_mac;
          e->sender_mac = MACADDR;

          unsigned long long tmp = e->ipv4->sender_addr;
          e->ipv4->sender_addr = e->ipv4->recver_addr;
          e->ipv4->recver_addr = tmp;
          unsigned int tmp2 = e->ipv4->udp->sender_port;
          e->ipv4->udp->sender_port = e->ipv4->udp->recver_port;
          e->ipv4->udp->recver_port = tmp2;


          UDP *udp = e->ipv4->udp;
          if (udp->recver_port == 68) {
              DHCP *dhcp = parse_dhcp(udp->data);
              char dhcp_data[512];
              memset(dhcp_data, 0, 512);
              DHCP *d;
              print_dhcp(dhcp);
              dump_data(bufr);
              switch(dhcp->type) {
                  case DHCPDiscover:
                      printf("Discover!!\n");
                      d = handle_discover(dhcp);
                      break;
                  case DHCPRequest:
                      printf("Request!!\n");
                      d = handle_request(dhcp);
                      break;
                  default:
                      continue;
              }
              if (d == NULL) {
                  printf("oopps");
                  continue;
              }
              int size = gen_dhcp_packet(d, dhcp_data);
              e->ipv4->udp->length = size;
              e->ipv4->udp->data = dhcp_data;

              e->ipv4->length = size + 36;

              print_dhcp(parse_dhcp(dhcp_data));

              e->ipv4->sender_addr = IPADDR;
              e->ipv4->recver_addr = 0xffffffff;

              gen_ethernet_packet(e, buf);
              print_packet(e);
              write_a_packet(buf);
              dump_data(buf);
          } else {
              gen_ethernet_packet(e, buf);
              write_a_packet(buf);
              dump_data(bufr);
              dump_data(buf);
              print_packet(e);
          }
          printf("\n\n");
      }
      else {
          print_packet(e);
          continue;
      }
  }

  sleep(2);
  return 0;
}
