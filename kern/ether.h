#ifndef ETHER_H
#define ETHER_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <kern/ip.h>

#define ETH_TYPE_IP 0x0800

typedef struct mac_addr
{
    uint8_t octants[6];
} mac_addr_t;

typedef struct ethernet_header
{
    mac_addr_t dst;
    mac_addr_t src;
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

typedef struct ethernet_packet
{
    eth_hdr_t hdr;
    uint8_t data[ETHERNET_MTU];
    uint32_t checksum;

} __attribute__((packed)) eth_pkt_t;

int fill_eth_hdr(mac_addr_t src, mac_addr_t dst, eth_pkt_t *dst_struct);

uint16_t eth_checksum(const void *src, size_t len);

eth_pkt_t *mk_eth_pkt(mac_addr_t src_mac, mac_addr_t dst_mac,
                      ip_port_t src_ip,   ip_port_t dst_ip, 
                      void *data, size_t len, void *buff);

void print_mac_addr(const mac_addr_t *addr);

void dump_eth_pkt(eth_pkt_t *pkt);

#endif /* !ETHER_H */
