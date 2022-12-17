#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/udp.h>
#include <kern/ip.h>

// Wraps arbitrary datum into UDP header.
int
wrap_in_udp(
    uint16_t    src_port,
    uint16_t    dst_port,
    const void *data,
    uint16_t    len,
    udp_pkt_t  *dst_struct
)
{
    assert(dst_struct);
    assert(data);
    assert(len < sizeof(udp_pkt_t) - sizeof(udp_hdr_t));

    udp_hdr_t *hdr = (udp_hdr_t*) dst_struct;

    hdr->src_port = src_port;
    hdr->dst_port = dst_port;

    // FIXME: should we include only actual data length here?
    hdr->dtg_length = sizeof(udp_pkt_t);

    memcpy(dst_struct->datagram, data, len);

    // FIXME: implement checksum
    hdr->checksum = udp_checksum(hdr, sizeof(udp_pkt_t));

    return 0;
}

// TODO: implement, not implemented because VPADLU
uint16_t udp_checksum(const void *src, size_t len)
{
    // HINT: use UDP pseudo header
    (void) src;
    (void) len;

    return 0;
}

// Wrap data into UDP and IPv4 headers
int make_udp_pkt(ip_port_t src, ip_port_t dst, const void *data,
                    size_t len, ip_pkt_t *dst_struct)
{
    fill_in_ipv4(src.addr, dst.addr, UDP, dst_struct);
    wrap_in_udp(src.port, dst.port, data, len, (udp_pkt_t *)dst_struct->data);

    return 0;
}

#define ISALPHA(ch) ((ch > 'a' && ch < 'z') || (ch > 'A' && ch < 'Z'))

void dump_pkt(ip_pkt_t *pkt)
{
    ip_hdr_t  *ip_hdr  = (ip_hdr_t *)pkt;
    udp_hdr_t *udp_hdr = (udp_hdr_t *)(pkt->data);

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
        ip_hdr->pkt_len,
        ip_hdr->checksum
    );

    uint8_t *as_str = (uint8_t *)pkt;

    int zero_counter = 0;

    for(int y = 0; y < pkt->hdr.pkt_len; y += 10) {
        for (int x = 0; x < 10 && x + y < pkt->hdr.pkt_len; x++) {
            uint8_t ch = as_str[x + y];
            if (ch == 0) {
                zero_counter++;
            }
            else {
                if (zero_counter > 1) {
                    cprintf("\n...Skipped %d zeroes...\n", zero_counter);
                    y = x + y;
                    x = 0;
                }
                else if (zero_counter == 1) {
                    cprintf("[] (0) ");
                }

                zero_counter = 0;

                if (ISALPHA(ch)) {
                    cputchar(ch);
                }
                else {
                    cprintf("[%c] (%d) ", ch, ch);
                }
            }
        }

        if (zero_counter == 0)
            cputchar('\n');
    }

    if (zero_counter > 1) {
        cprintf("\n...Skipped %d zeroes... ", zero_counter);
    }
    else if (zero_counter == 1) {
        cprintf("[] (0) ");
    }
    
    cputchar('\n');
    cputchar('\n');

    return;
}
