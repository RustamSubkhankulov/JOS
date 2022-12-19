#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/ip.h>

int
fill_ipv4_hdr(ip_addr_t  src,  ip_addr_t  dst,
             protocol_t prot, ip_pkt_t  *dst_struct)
{
    assert(dst_struct);

    ip_hdr_t *hdr = (ip_hdr_t*) dst_struct;

    hdr->version = IP_VERSION;
    hdr->hdr_len = 5;

    // FIXME: check what to store here
    hdr->DSCP = 0;
    hdr->ECN = 0;

    // FIXME: should we include only actual data length here?
    hdr->pkt_len = BSWAP_16(sizeof(ip_pkt_t));

    // FIXME: check if 0 when not fragmented
    hdr->identifier = 0;

    hdr->flags      = IP_HDR_NO_FRGM;
    hdr->pkt_offset = 0;
    hdr->ttl        = DEFAULT_TTL;
    hdr->protocol   = prot;

    hdr->src_ip = src;
    hdr->dst_ip = dst;

    return 0;
}

// TODO: implement
uint16_t ip_checksum(const void *src, size_t len)
{
    (void) src;
    (void) len;

    return 0;
}

ip_pkt_t *mk_ip_pkt(
    ip_port_t  src,  ip_port_t  dst,
    void *data, size_t len,
    void *buff)
{
    memset(buff, 0, sizeof(ip_hdr_t));

    ip_pkt_t *pkt = (ip_pkt_t *)buff;
    fill_ipv4_hdr(src.addr, dst.addr, UDP, pkt);
    
    // dump_pkt(pkt);

    mk_udp_pkt(src.port, dst.port, data, len, pkt->data);

    pkt->hdr.checksum = ip_checksum(buff, len + sizeof(ip_hdr_t));

    return buff;
}

uint32_t get_ip_addr_word(const ip_addr_t *addr)
{
    return bswap32(addr->word);
}

uint16_t get_ip_pkt_len(const ip_pkt_t *pkt)
{
    return BSWAP_16(pkt->hdr.pkt_len);
}

ip_port_t make_addr_port(uint32_t ip_addr, uint16_t port)
{
    ip_port_t res;

    res.port = port;

    uint32_t inv_addr = bswap32(ip_addr);

    cprintf("addr = 0x%x; inverted addr = 0x%x\n", ip_addr, inv_addr);

    res.addr.word = inv_addr;

    return res;
}

// --------- < Dump > ----------

#define ISALPHA(ch) ((ch > 'a' && ch < 'z') || (ch > 'A' && ch < 'Z'))

void dump_pkt(ip_pkt_t *pkt)
{
    ip_hdr_t  *ip_hdr  = (ip_hdr_t *)pkt;
    udp_hdr_t *udp_hdr = (udp_hdr_t *)(pkt->data);

    uint16_t pkt_len = get_ip_pkt_len(pkt);

    cprintf(
        "\nIPv4 pkt at <%p>:\n"
        "\tsrc = %03d.%03d.%03d.%03d:%04d\n"
        "\tdst = %03d.%03d.%03d.%03d:%04d\n"
        "\tttl = %d\n"
        "\tprotocol = %d\n"
        "\tlen = %d\n"
        "\tIPv4 checksum = 0x%x\n\n",
        pkt,
        
        ip_hdr->src_ip.oct0,
        ip_hdr->src_ip.oct1,
        ip_hdr->src_ip.oct2,
        ip_hdr->src_ip.oct3,
        udp_hdr->src_port,

        ip_hdr->dst_ip.oct0,
        ip_hdr->dst_ip.oct1,
        ip_hdr->dst_ip.oct2,
        ip_hdr->dst_ip.oct3,
        udp_hdr->dst_port,

        ip_hdr->ttl,
        ip_hdr->protocol,
        pkt_len,
        ip_hdr->checksum
    );

    uint8_t *as_str = (uint8_t *)pkt;

    int zero_counter = 0;

    for(int y = 0; y < pkt_len; y += 10) {
        for (int x = 0; x < 10 && x + y < pkt_len; x++) {
            uint8_t ch = as_str[x + y];
            if (ch == 0) {
                zero_counter++;
            } else {
                if (zero_counter > 1) {
                    cprintf("\n...Skipped %d zeroes...\n", zero_counter);
                    y = x + y;
                    x = 0;
                } else if (zero_counter == 1) {
                    cprintf("[] (0) ");
                }

                zero_counter = 0;

                if (ISALPHA(ch)) {
                    cputchar(ch);
                } else {
                    cprintf("[%c] (0x%x) ", ch, ch);
                }
            }
        }

        if (zero_counter == 0)
            cputchar('\n');
    }

    if (zero_counter > 1) {
        cprintf("\n...Skipped %d zeroes... ", zero_counter);
    } else if (zero_counter == 1) {
        cprintf("[] (0) ");
    }
    
    cputchar('\n');
    cputchar('\n');

    return;
}
#undef ISALPHA

