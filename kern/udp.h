#ifndef UDP_H
#define UDP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <kern/ip.h>

#define MAX_UDP_PORTS 65536

typedef struct udp_header
{
    uint16_t src_port;   /* Optional in IPv4 */
    uint16_t dst_port;
    uint16_t dtg_length; /* Datagram length */
    uint16_t checksum;   /* Optional in IPv4 */
} __attribute__((packed)) udp_hdr_t;

typedef struct udp_packet
{
    udp_hdr_t hdr;
     
    /* FIXME: We are not currently supporting datagrams bigger than
              ethernet MTU. */
    uint8_t datagram[ETHERNET_MTU - sizeof(ip_hdr_t) - sizeof(udp_hdr_t)];
} __attribute__((packed)) udp_pkt_t;

typedef struct ip_and_port
{
    ip_addr_t addr;
    uint16_t  port;
} __attribute__((packed)) ip_port_t;

int wrap_in_udp(uint16_t src_port, uint16_t dst_port, 
                const void *data, uint16_t len,
                udp_pkt_t  *dst_struct);

uint16_t udp_checksum(const void *src, size_t len);

int make_udp_pkt(ip_port_t src, ip_port_t dst, const void *data, size_t len, ip_pkt_t *dst_struct);

void dump_pkt(ip_pkt_t *pkt);

ip_port_t make_addr_port(uint32_t ip_addr, uint16_t port);

#endif /* !UDP_H */
