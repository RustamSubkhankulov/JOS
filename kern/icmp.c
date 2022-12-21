#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/icmp.h>

int is_icmp_req(const icmp_pkt_t *pkt)
{
    assert(pkt);

    return (pkt->code == 0 && pkt->type == ECHO_REQ);
}

// Warning: return value is little endian
static uint16_t icmp_csum(const void *addr, size_t len)
{
    uint16_t checksum;
    uint32_t sum = 0;

    uint16_t *as_words = (uint16_t *)addr;

    size_t idx = 0;

    while (len > 1)  {
        /*  This is the inner loop */
        sum += BSWAP_16(as_words[idx]);
        len -= 2;
        idx++;
    }

    /*  Add left-over byte, if any */
    if (len > 0)
        sum += *(uint8_t *)(as_words + idx);

    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    checksum = ~sum;

    return checksum;
}

eth_pkt_t mk_icmp_response(const eth_pkt_t *pkt)
{
    eth_pkt_t response = *pkt;

    // Fill Ethernet header
    response.hdr.dst = pkt->hdr.src;
    response.hdr.src = pkt->hdr.dst;

    icmp_pkt_t *icmp_pkt = (icmp_pkt_t *)response.data;
    icmp_pkt_t *icmp_req = (icmp_pkt_t *)pkt->data;

    // Fill IPv4 header
    icmp_pkt->hdr.checksum = 0;
    icmp_pkt->hdr.dst_ip   = icmp_req->hdr.src_ip;
    icmp_pkt->hdr.src_ip   = icmp_req->hdr.dst_ip;

    // Fill icmp header
    icmp_pkt->type       = ECHO_REPL;
    icmp_pkt->checksum   = 0;

    dump_ip_pkt((ip_pkt_t *)&icmp_req->hdr);

    memcpy(icmp_pkt->data, icmp_req->data, BSWAP_16(icmp_req->hdr.pkt_len) - sizeof(ip_hdr_t));

    int icmp_len = BSWAP_16(icmp_req->hdr.pkt_len) - sizeof(ip_hdr_t);
    uint16_t csum_le = icmp_csum((uint8_t *)icmp_pkt + sizeof(ip_hdr_t), icmp_len);

    icmp_pkt->checksum = BSWAP_16(csum_le);

    return response;
}

void dump_icmp_pkt(icmp_pkt_t *pkt)
{
    cprintf("Dump ICMP start ------------\n");

    cprintf("ICMP at <%p>:\n"
            "\ttype = 0x%x\n"
            "\tcode = 0x%x\n"
            "\tcsum = 0x%x\n"
            "\tid   = 0x%x\n"
            "\tseqn = 0x%x\n"
            "\tsrc IP = ",
                pkt,
                pkt->type,
                pkt->code,
                BSWAP_16(pkt->checksum),
                BSWAP_16(pkt->identifier),
                BSWAP_16(pkt->seq_number)
            );
    print_ip_addr(&pkt->hdr.src_ip);

    cprintf("\n\tdst ip = ");
    print_ip_addr(&pkt->hdr.dst_ip);

    cprintf("\nDump ICMP end --------------\n");
}
