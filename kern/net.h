#ifndef JOS_KERN_NET_H
#define JOS_KERN_NET_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/assert.h>
#include <inc/x86.h>

void init_net(void);

#endif /* !JOS_KERN_NET_H */