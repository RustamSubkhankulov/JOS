#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/ether.h>

int fill_eth_hdr(mac_addr_t src, mac_addr_t dst, eth_pkt_t *dst_struct)
{
    assert(dst_struct);

    eth_hdr_t *hdr = (eth_hdr_t *)dst_struct;

    hdr->src = src;
    hdr->dst = dst;

    hdr->type = BSWAP_16(ETH_TYPE_IP);

    // FIXME: implement checksum
    dst_struct->checksum = 0;

    return 0;
}

// TODO: implement. Not implemented because VPADLU
uint16_t eth_checksum(const void *src, size_t len)
{
    // HINT: use UDP pseudo header
    (void) src;
    (void) len;

    return 0;
}

eth_pkt_t *mk_eth_pkt(
    mac_addr_t src_mac, mac_addr_t dst_mac,
    ip_port_t  src_ip,  ip_port_t  dst_ip,
    void *data, size_t len,
    void *buff)
{
    eth_pkt_t *pkt = (eth_pkt_t *) buff;

    memset(buff, 0, sizeof(eth_hdr_t));

    fill_eth_hdr(src_mac, dst_mac, pkt);
    
    mk_ip_pkt(src_ip, dst_ip, data, len, pkt->data);

    pkt->checksum = eth_checksum(pkt, sizeof(udp_pkt_t));

    return buff;
}

void print_mac_addr(const mac_addr_t *addr)
{
    cprintf(
        "%02x:%02x:%02x:%02x:%02x:%02x",
            addr->octants[0],
            addr->octants[1],
            addr->octants[2],
            addr->octants[3],
            addr->octants[4],
            addr->octants[5]
        );
}

void dump_eth_pkt(eth_pkt_t *pkt)
{
    cprintf("Start of ETH dump --------------\n");

    eth_hdr_t *hdr = (eth_hdr_t *)pkt;

    cprintf("\tdst MAC = ");
    print_mac_addr(&hdr->dst);
    cprintf("\n\tsrc MAC = ");
    print_mac_addr(&hdr->src);

    cprintf("\n\ttype = %x\n"
            "Raw payload:\n",
            BSWAP_16(hdr->type)
    );

    uint8_t *as_char = (uint8_t *)pkt;

    for (size_t byte = 0; byte < 100; byte += 10)
    {
        cputchar('\t');
        for (int i = 0; i < 10; i++)
        {
            unsigned char ch = as_char[byte + i];

            if (ISALPHA(ch))
            {
                cputchar(ch);
                cputchar('\t');
            }
            else
            {
                cprintf("0x%02x\t", ch);
            }
        }
        cputchar('\n');
    }

    cprintf("End of ETH dump --------------\n");
}
