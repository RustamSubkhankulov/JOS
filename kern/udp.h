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

#endif /* !UDP_H */
