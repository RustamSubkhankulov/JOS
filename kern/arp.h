#ifndef ARP_H
#define ARP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <kern/ether.h>

#define ARP_TYPE       0x0806
#define ETHERNET_HTYPE 0x0001
#define ARP_OPER_REQ   1
#define ARP_OPER_RESP  2

typedef struct arp_pkt
{
    eth_hdr_t hdr;
    uint16_t htype; // Ethernet = 0x0001
    uint16_t ptype; // Protocol type. Ip = ETH_TYPE_IP = 0x0800
    uint8_t  hlen;  // MAC addr len = 6
    uint8_t  plen;  // IP addr len = 4
    uint16_t oper;  // Operation = REQ/RESP
    mac_addr_t sha; // Sender hardware address (Src MAC)
    ip_addr_t  spa; // Sender protocol address (Src IP)
    mac_addr_t tha; // Target hardware address (My MAC)
    ip_addr_t  tpa; // Target protocol address (My IP)
} __attribute__ ((packed)) arp_pkt_t;

int is_arp_req(const arp_pkt_t *pkt);

void dump_arp_pkt(arp_pkt_t *pkt);

arp_pkt_t mk_arp_response(const arp_pkt_t *request, const mac_addr_t *src_mac);

#endif /* !ARP_H */
