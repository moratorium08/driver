typedef unsigned char byte;

typedef struct {
  byte type;
  byte len;
  byte *data;
} DHCPOption;

typedef enum {
    DHCPDiscover = 1,
    DHCPOffer = 2,
    DHCPRequest = 3,
    DHCPAck = 5,
} DHCPType;

typedef struct {
    DHCPType type;
    byte op;
    byte htype;
    byte hlen;
    byte ops;
    unsigned xid;
    unsigned short secs;
    unsigned short flags;
    unsigned ciaddr;
    unsigned yiaddr;
    unsigned siaddr;
    unsigned giaddr;
    unsigned long long chaddr;

    int option_num;
    DHCPOption options[64 / 3 + 1]; // めんどいので
} DHCP;
DHCP *handle_discover(DHCP *dhcp);
DHCP *handle_request(DHCP *dhcp);
void init_dhcp(unsigned int x, unsigned int y);
