#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/arp.h>

int is_arp_req(const arp_pkt_t *pkt)
{
    assert(pkt);

    const uint8_t *dst_mac = pkt->hdr.dst.octants;

    for (int i = 0; i < 6; i++)
    {
        if(dst_mac[i] != 0xFF) return 0;
    }

    uint16_t type = BSWAP_16(pkt->hdr.type);

    int res = (type == ARP_TYPE) && (BSWAP_16(pkt->oper) == ARP_OPER_REQ);

    return res;
}

void dump_arp_pkt(arp_pkt_t *pkt)
{
    cprintf("Dump ARP start ------------\n");

    cprintf("ARP at <%p>:\n"
            "\thtype = 0x%x\n"
            "\tptype = 0x%x\n"
            "\thlen  = 0x%x\n"
            "\tplen  = 0x%x\n"
            "\toper  = 0x%x\n"
            "\tsrc MAC = ",
                pkt,
                BSWAP_16(pkt->htype),
                BSWAP_16(pkt->ptype),
                pkt->hlen,
                pkt->plen,
                BSWAP_16(pkt->oper)
            );
    print_mac_addr(&pkt->sha);

    cprintf("\n\tsrc ip = ");
    print_ip_addr(&pkt->spa);

    cprintf("\n\tdst MAC = ");
    print_mac_addr(&pkt->tha);

    cprintf("\n\tdst ip = ");
    print_ip_addr(&pkt->tpa);

    cprintf("\nDump ARP end --------------\n");
}

arp_pkt_t mk_arp_responce(const arp_pkt_t *request, const mac_addr_t *src_mac)
{
    arp_pkt_t responce = *request;

    // Change sender and target vice versa
    responce.tha = responce.sha;

    ip_addr_t sender = responce.tpa;
    responce.tpa = responce.spa;
    responce.spa = sender;

    // Fill in our own MAC
    responce.sha = *src_mac;

    // We now send a responce operation
    responce.oper = BSWAP_16(ARP_OPER_RESP);

    responce.hdr.src = *src_mac;
    responce.hdr.dst = responce.tha;
    
    dump_eth_pkt((eth_pkt_t *)&responce);

    dump_arp_pkt(&responce);

    return responce;
}
