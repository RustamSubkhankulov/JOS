#ifndef ICMP_H
#define ICMP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include <kern/ether.h>

enum ICMP_type
{
    ECHO_REQ  = 0x8,
    ECHO_REPL = 0x0,
};

typedef struct icmp_pkt
{
    ip_hdr_t hdr;
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t seq_number;
    uint8_t data[ETHERNET_MTU - sizeof(ip_hdr_t)];
} __attribute__ ((packed)) icmp_pkt_t;

int is_icmp_req(const icmp_pkt_t *pkt);

eth_pkt_t mk_icmp_response(const eth_pkt_t *pkt);

void dump_icmp_pkt(icmp_pkt_t *pkt);

#endif /* !ICMP_H */
