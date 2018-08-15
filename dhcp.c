#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "dhcp.h"

typedef enum {
    IPUsed,
    IPReserved,
    IPAvailable
} IPStatus;

typedef struct {
    IPStatus st;
    unsigned long long addr;
} IPAddr;


// mask 255.255.255.0
IPAddr ipaddrs[256];
unsigned int ip_addr_upper = 0xc0a800;
unsigned int host_ip = 0xc0a8000b;

void init_dhcp(unsigned int x, unsigned int y) {
    for (int i = 0; i < 256; i++) {
        ipaddrs[i].st = IPAvailable;
    }
    ip_addr_upper = x;
    host_ip = y;
}

void use(int x) {
    ipaddrs[x].st = IPUsed;
}

int is_reserverd(unsigned long long haddr, int addr) {
    IPAddr ad = ipaddrs[addr];
    if (ad.st != IPReserved) return 0;
    if (ad.addr != haddr) return 0;
    ipaddrs[addr].st = IPUsed;
    return 1;
}

int reserve(unsigned long long haddr) {
    for (int i = 15; i < 256; i++) {
        IPAddr ad = ipaddrs[i];
        if (ad.addr == haddr) {
            return ip_addr_upper * 0x100 + i;
        }
        if (ad.st == IPAvailable) {
            ipaddrs[i].st = IPReserved;
            ipaddrs[i].addr = haddr;
            return ip_addr_upper * 0x100 + i;
        }
    }
    printf("hoge\n");
    return -1;
}

DHCP *handle_discover(DHCP *d) {
    unsigned int r = reserve(d->chaddr);
    if (r < 0) {
        return NULL;
    }

    DHCP *dhcp = (DHCP *)malloc(sizeof(DHCP));
    memset(dhcp, 0, sizeof(DHCP));

    dhcp->type = DHCPOffer;
    dhcp->op = 2;
    dhcp->htype = 0x1;
    dhcp->hlen = 0x6;
    dhcp->ops = d->ops;
    dhcp->xid = d->xid;
    dhcp->secs = d->secs;
    dhcp->flags = d->flags;
    dhcp->ciaddr = 0;
    dhcp->yiaddr = r;
    dhcp->siaddr = host_ip;
    dhcp->giaddr = 0;
    dhcp->chaddr = d->chaddr;
    
    return dhcp;
} 

DHCP *handle_request(DHCP *d) {
    unsigned int addr = 0;
    for (int i = 0; i < d->option_num; i++) {
        DHCPOption op = d->options[i];
        if (op.type != 50) continue;

        for (int i = 0; i < 4; i++) {
            addr *= 0x100;
            addr += (unsigned char)(op.data[i]);
        }

    }

    printf("%x\n", addr);
    if (!is_reserverd(d->chaddr, addr & 0xff)) {
        return NULL;
    }
    DHCP *dhcp = (DHCP *)malloc(sizeof(DHCP));
    memset(dhcp, 0, sizeof(DHCP));

    dhcp->type = DHCPAck;
    dhcp->op = 2;
    dhcp->htype = 0x1;
    dhcp->hlen = 0x6;
    dhcp->ops = d->ops;
    dhcp->xid = d->xid;
    dhcp->secs = d->secs;
    dhcp->flags = d->flags;
    dhcp->ciaddr = 0;
    dhcp->yiaddr = addr;
    dhcp->siaddr = host_ip;
    dhcp->giaddr = 0;
    dhcp->chaddr = d->chaddr;

    return dhcp;
}
