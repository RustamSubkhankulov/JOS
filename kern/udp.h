#ifndef UDP_H
#define UDP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

#define MAX_UDP_PORTS 65536
#define ETHERNET_MTU 1500
#define MAX_IP_HDR_SIZE 60
#define BSWAP_16(val)  \
    ((uint16_t)(((uint16_t)val >> 8) | ((uint16_t)val << 8)))

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
    uint8_t datagram[ETHERNET_MTU - MAX_IP_HDR_SIZE - sizeof(udp_hdr_t)];
} __attribute__((packed)) udp_pkt_t;

int fill_udp_hdr(uint16_t src_port, uint16_t dst_port, uint16_t len, udp_pkt_t *dst_struct);

uint16_t udp_checksum(const void *src, size_t len);

udp_pkt_t *mk_udp_pkt(uint16_t src_port, uint16_t dst_port, void *data, size_t len, void *buff);

#endif /* !UDP_H */
