#include <inc/assert.h>
#include <inc/error.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/traceopt.h>
#include <kern/net.h>

void init_net(void)
{
    if (trace_net)
        cprintf("Net initiaization started. \n");

    // routine

    if (trace_net)
        cprintf("Net initialization successfully finished. \n");

    return;
}
