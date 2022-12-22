#ifndef JOS_KERN_NET_H
#define JOS_KERN_NET_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/virtio.h>
#include <kern/pmap.h>

const static enum Pci_class            Virtio_nic_pci_class    = PCI_CLASS_NETWORK_CONTROLLER;
const static enum Pci_network_subclass Virtio_nic_pci_subclass = PCI_SUBCLASS_ETHERNET;

const static uint16_t Virtio_nic_pci_device_id = 0x1000; // network card
const static uint16_t Virtio_nic_device_id     = 0x01;   // network card

/* Receive and send queues */
#define QUEUE_NUM 2

#define RCV_MAX_SIZE 2048 // size of maximum incoming packet + net header + ethernet header = 1526
#define SND_MAX_SIZE 2048

#define MAC_ADDR_NUM 6

/* Queues indexes in virtio_dev_t struct.queues */
#define RCVQ 0
#define SNDQ 1

typedef struct Virtio_nic_dev
{
    virtio_dev_t virtio_dev;
    uint8_t MAC[MAC_ADDR_NUM];

} virtio_nic_dev_t;

/* Flags in virtio_net_hdr_t.flags */
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    0x1 
#define VIRTIO_NET_HDR_F_DATA_VALID    0x2 
#define VIRTIO_NET_HDR_F_RSC_INFO      0x4 

/* Types of virtio_net_hdr_t.gso_type */
#define VIRTIO_NET_HDR_GSO_NONE        0x0 
#define VIRTIO_NET_HDR_GSO_TCPV4       0x1 
#define VIRTIO_NET_HDR_GSO_UDP         0x3 
#define VIRTIO_NET_HDR_GSO_TCPV6       0x4 
#define VIRTIO_NET_HDR_GSO_ECN         0x80 

typedef struct Virtio_net_hdr 
{ 
    uint8_t flags;                     // flags (see above)
    uint8_t gso_type;                  // segmentation offload (types above)
    uint16_t hdr_len;                  // size of header to be used during segmentation
    uint16_t gso_size;                 // maximum segment size (not including header)
    uint16_t csum_start;               // position to begin calculating the checksum
    uint16_t csum_offset;              // position after csum_start to store the checksum

} virtio_net_hdr_t;

/* Network Device Registers   Offs  Size*/
#define VIRTIO_PCI_NET_MAC1   0x14  // 1
#define VIRTIO_PCI_NET_MAC2   0x15  // 1
#define VIRTIO_PCI_NET_MAC3   0x16  // 1
#define VIRTIO_PCI_NET_MAC4   0x17  // 1
#define VIRTIO_PCI_NET_MAC5   0x18  // 1
#define VIRTIO_PCI_NET_MAC6   0x19  // 1
#define VIRTIO_PCI_NET_STATUS 0x1A  // 1

/* Network Card Feature Bits */
#define VIRTIO_NET_F_CSUM                (1 << 0 )
#define VIRTIO_NET_F_GUEST_CSUM          (1 << 1 )
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS (1 << 2 )
#define VIRTIO_NET_F_MTU                 (1 << 3 )
#define VIRTIO_NET_F_MAC                 (1 << 5 )
#define VIRTIO_NET_F_GUEST_TSO4          (1 << 7 )
#define VIRTIO_NET_F_GUEST_TSO6          (1 << 8 )
#define VIRTIO_NET_F_GUEST_ECN           (1 << 9 )
#define VIRTIO_NET_F_GUEST_UFO           (1 << 10)
#define VIRTIO_NET_F_HOST_TSO4           (1 << 11)
#define VIRTIO_NET_F_HOST_TSO6           (1 << 12)
#define VIRTIO_NET_F_HOST_ECN            (1 << 13)
#define VIRTIO_NET_F_HOST_UFO            (1 << 14)
#define VIRTIO_NET_F_MRG_RXBUF           (1 << 15)
#define VIRTIO_NET_F_STATUS              (1 << 16)
#define VIRTIO_NET_F_CTRL_VQ             (1 << 17)
#define VIRTIO_NET_F_CTRL_RX             (1 << 18)
#define VIRTIO_NET_F_CTRL_VLAN           (1 << 19)
#define VIRTIO_NET_F_GUEST_ANNOUNCE      (1 << 21)
#define VIRTIO_NET_F_MQ                  (1 << 22)
#define VIRTIO_NET_F_CTRL_MAC_ADDR       (1 << 23)
#define VIRTIO_NET_F_RSC_EXT             (1 << 61)
#define VIRTIO_NET_F_STANDBY             (1 << 62)

/* Reserved Feature Bits */
#define VIRTIO_F_RING_INDIRECT_DESC 28
#define VIRTIO_F_RING_EVENT_IDX     29
#define VIRTIO_F_VERSION_1          32
#define VIRTIO_F_ACCESS_PLATFORM    33 
#define VIRTIO_F_RING_PACKED        34
#define VIRTIO_F_IN_ORDER           35
#define VIRTIO_F_ORDER_PLATFORM     36
#define VIRTIO_F_SR_IOV             37
#define VIRTIO_F_NOTIFICATION_DATA  38

void init_net(void);
int virtio_nic_snd_buffer(const buffer_info_t* buffer_info);
int virtio_nic_rcv_buffer(      buffer_info_t* rcv_buffer);

#endif /* !JOS_KERN_NET_H */