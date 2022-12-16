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
    uint16_t    len,
    const void *data,
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
int make_udp_packet(ip_port_t src, ip_port_t dst, const void *data,
                    size_t len, ip_pkt_t *dst_struct)
{
    
    return 0;
}
