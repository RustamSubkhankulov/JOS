#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>
#include <inc/string.h>

#include <kern/traceopt.h>
#include <kern/udp.h>
#include <kern/ip.h>

// Wraps arbitrary datum into IPv4 header.
int
wrap_in_ipv4(
    ip_addr_t      dst,
    ip_addr_t      src,
    const uint8_t *data,
    size_t         len,
    protocol_t     prot,
    ip_pkt_t      *dst_struct
)
{
    assert(dst_struct);
    assert(data);
    assert(len < sizeof(ip_pkt_t) - sizeof(ip_hdr_t));

    ip_hdr_t *hdr = (ip_hdr_t*) dst_struct;

    hdr->version = IP_VERSION;
    hdr->hdr_len = 5;

    // FIXME: check what to store here
    hdr->DSCP = 0;
    hdr->ECN = 0;

    // FIXME: should we include only actual data length here?
    hdr->pkt_len = sizeof(ip_pkt_t);

    // FIXME: check if 0 when not fragmented
    hdr->identifier = 0;

    hdr->flags      = IP_HDR_NO_FRGM;
    hdr->pkt_offset = 0;
    hdr->ttl        = DEFAULT_TTL;
    hdr->protocol   = prot;

    // FIXME: add checksum calculation
    hdr->checksum = pkt_checksum(hdr, sizeof(ip_pkt_t));

    

    return 0;
}

// TODO: implement
uint16_t pkt_checksum(const void *src, size_t len)
{
    (void) src;
    (void) len;

    return 0;
}

