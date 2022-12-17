#ifndef IP_H
#define IP_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

#define DEFAULT_TTL  64
#define IP_VERSION   4
#define ETHERNET_MTU 1500

#define IP_HDR_NO_FRGM   (1U << 1U)
#define IP_HDR_MORE_FRGM (1U << 2U)

typedef enum Protocol
{
    UDP  = 17,
} protocol_t;

typedef union ip_addr
{
    // IP addr as 4 octets, i.e. 192.168.1.322.
    // Order is inverted to represent little-endianness
    struct {
        uint8_t oct3;
        uint8_t oct2;
        uint8_t oct1;
        uint8_t oct0;
    } __attribute__((packed));

    // IP addr as a single 32-bit word
    uint32_t word;

} ip_addr_t;

typedef struct ip_header
{
    // TODO: check bitfield order

    struct {
        uint8_t  hdr_len     : 4;  // Header length in 32-bit words. 
                                   // Can take values from 5 to 15.
                                   // This particular header is 5 words long.
        
        uint8_t  version     : 4;  // IPv4.version = 4
    } __attribute__((packed));

    struct {
        uint8_t  ECN         : 2;  // Explicit Congestion Notification
        uint8_t  DSCP        : 6;  // Differentiated Services Code Point
    } __attribute__((packed));

    uint16_t pkt_len;          // Includes header and data
    uint16_t identifier;       // Used if pkt is fragmented
    
    struct {
        uint16_t pkt_offset  : 13; // Offset in fragment sequence
                                   // First pkt in sequence has this field 0.

        uint8_t  flags       : 3;  // Bit 0 = 0 rsrvd
                                   // Bit 1 = do not fragment
                                   // Bit 2 = has fragments yet to come
    } __attribute__((packed));

    uint8_t  ttl;              // Time To Live

    uint8_t  protocol;         // Protocol number.
                               // REF: https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml    
    
    uint16_t checksum;         // Should be recalculated at each translation step
                               // Should be 0 if not used

    ip_addr_t src_ip;
    ip_addr_t dst_ip;
} __attribute__((packed)) ip_hdr_t;

typedef struct ip_packet
{
    ip_hdr_t hdr;
    uint8_t data[ETHERNET_MTU - sizeof(ip_hdr_t)];
} __attribute__((packed)) ip_pkt_t;

int fill_in_ipv4(ip_addr_t src, ip_addr_t dst, 
                 protocol_t prot, ip_pkt_t *dst_struct);

int wrap_in_ipv4(ip_addr_t src, ip_addr_t dst, const void *data,
                 size_t len, protocol_t prot, ip_pkt_t *dst_struct);

uint16_t ip_checksum(const void *src, size_t len);

#endif /* !IP_H */
