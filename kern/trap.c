#include "trap.h"

#include "arm.h"
#include "sysregs.h"
#include "mmu.h"
#include "syscall.h"
#include "peripherals/irq.h"

#include "uart.h"
#include "console.h"
#include "clock.h"
#include "timer.h"
#include "proc.h"
#include "sd.h"
#include "spinlock.h"

uint32_t irqinitcnt;
struct spinlock irqinitlock;

uint32_t irqinitcnt;
struct spinlock irqinitlock;

void
irq_init()
{
    acquire(&irqinitlock);

    if (irqinitcnt){
        release(&irqinitlock);
        return;
    }
    irqinitcnt = 1;

    cprintf("irq_init: - irq init\n");
    clock_init();
    put32(ENABLE_IRQS_1, AUX_INT);
    put32(ENABLE_IRQS_2, VC_ARASANSDIO_INT);
    put32(GPU_INT_ROUTE, GPU_IRQ2CORE(0));

    release(&irqinitlock);
}

void
interrupt(struct trapframe *tf)
{
    int src = get32(IRQ_SRC_CORE(cpuid()));
    if (src & IRQ_CNTPNSIRQ) {
        timer_reset();
        timer();
    } else if (src & IRQ_TIMER) {
        clock_reset();
        clock();
    } else if (src & IRQ_GPU) {
        int p1 = get32(IRQ_PENDING_1), p2 = get32(IRQ_PENDING_2);
        if (p1 & AUX_INT) {
            uart_intr();
        } else if (p2 & VC_ARASANSDIO_INT) {
            sd_intr();
        } else {
            cprintf("unexpected gpu intr p1 %x, p2 %x, sd %d, omitted\n", p1, p2, p2 & VC_ARASANSDIO_INT);
        }
    } else {
        cprintf("unexpected interrupt at cpu %d\n", cpuid());
    }
}

void
trap(struct trapframe *tf)
{
    uint64_t fa;
    int ec = resr() >> EC_SHIFT, iss = resr() & ISS_MASK;
    lesr(0);  /* Clear esr. */
    //debug_print("%x %x\n", (uint64_t)ec, iss);
    switch (ec) {
    case EC_UNKNOWN:
        interrupt(tf);
        break;

    case EC_SVC64:
        if (iss == 0) {
            syscall();
        } else {
            cprintf("unexpected svc iss 0x%x\n", iss);
        }
        break;

    case EC_DABORT:
        asm("MRS %[r], FAR_EL1": [r]"=r" (fa)::);
        cprintf ("data abort: instruction 0x%x, fault addr 0x%x\n", tf->pc, fa);
        panic("address above limit\n");
        break;

    default:
        panic("trap: unexpected irq.\n");
    }
}


void
irq_error(uint64_t type)
{
    cprintf("irq_error: - irq_error\n");
    panic("irq_error: irq of type %d unimplemented. \n", type);
}
