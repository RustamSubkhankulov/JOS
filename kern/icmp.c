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

    // TODO: implement

    return 0;
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

icmp_pkt_t mk_icmp_responce(const icmp_pkt_t *request)
{
    icmp_pkt_t responce = *request;

 
    return responce;
}
