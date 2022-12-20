#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/udp.h>

// Warning: send port arguments in little endian
int
fill_udp_hdr(uint16_t  src_port, 
             uint16_t  dst_port,
             uint16_t  len,
             udp_pkt_t *dst_struct)
{
    assert(dst_struct);
    assert(len < ETHERNET_MTU - MAX_IP_HDR_SIZE - sizeof(udp_hdr_t));

    udp_hdr_t *hdr = (udp_hdr_t*) dst_struct;

    hdr->src_port = BSWAP_16(src_port);
    hdr->dst_port = BSWAP_16(dst_port);

    // FIXME: should we include only actual data length here?
    len += sizeof(udp_hdr_t);
    hdr->dtg_length = BSWAP_16(len);

    return 0;
}

// TODO: implement. Not implemented because VPADLU
uint16_t udp_checksum(const void *src, size_t len)
{
    // HINT: use UDP pseudo header
    (void) src;
    (void) len;

    return 0;
}

udp_pkt_t *mk_udp_pkt(
    uint16_t src_port, uint16_t dst_port,
    void *data, size_t len,
    void *buff)
{
    memset(buff, 0, sizeof(udp_hdr_t));

    udp_pkt_t *pkt = (udp_pkt_t *)buff;

    fill_udp_hdr(src_port, dst_port, len, pkt);

    memcpy(pkt->datagram, data, len);

    pkt->hdr.checksum = udp_checksum(buff, len + sizeof(udp_hdr_t));

    return buff;
}
