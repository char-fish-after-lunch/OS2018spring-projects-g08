#include <assert.h>
#include <clock.h>
#include <console.h>
#include <types.h>
#include <kdebug.h>
#include <memlayout.h>
#include <mmu.h>
#include <arch.h>
#include <stdio.h>
#include <kio.h>
#include <vmm.h>
#include <swap.h>
#include <trap.h>
#include <proc.h>
#include <sync.h>
#include <sched.h>
#include <unistd.h>
#include <syscall.h>
#include <error.h>

#define ADR_PLIC 0b11000000011111111111111111110000
#define IRQ_SERIAL 1
#define IRQ_KEYBOARD 2
#define IRQ_NETWORK 3
#define IRQ_RESERVED 4

#define TICK_NUM 100

static void print_ticks() {
    kprintf("%d ticks\n", TICK_NUM);
#ifdef DEBUG_GRADE
    kprintf("End of Test.\n");
    panic("EOT: kernel seems ok.");
#endif
}

/**
 * @brief      Load supervisor trap entry in RISC-V
 */
void idt_init(void) {
    extern void __alltraps(void);
    /* Set sscratch register to 0, indicating to exception vector that we are
     * presently executing in the kernel */
    write_csr(mscratch, 0);
    /* Set the exception vector address */
    write_csr(mtvec, &__alltraps);
    /* Allow kernel to access user memory */
    set_csr(mstatus, MSTATUS_SUM);
    /* Allow keyboard interrupt */
    set_csr(mie, MIP_MEIP);
}

/* trap_in_kernel - test if trap happened in kernel */
bool trap_in_kernel(struct trapframe *tf) {
    return (tf->status & MSTATUS_MPP) != 0;
}

void print_trapframe(struct trapframe *tf) {
    kprintf("trapframe at %p\n", tf);
    kprintf("  status   0x%016lx\n", tf->status);
    kprintf("  epc      0x%016lx\n", tf->epc);
    kprintf("  badvaddr 0x%016lx\n", tf->badvaddr);
    kprintf("  cause    0x%016lx\n", tf->cause);
    print_stackframe();
    print_regs(&tf->gpr);
}

void print_regs(struct pushregs *gpr) {
    kprintf("  zero     0x%016lx\n", gpr->zero);
    kprintf("  ra       0x%016lx\n", gpr->ra);
    kprintf("  sp       0x%016lx\n", gpr->sp);
    kprintf("  gp       0x%016lx\n", gpr->gp);
    kprintf("  tp       0x%016lx\n", gpr->tp);
    kprintf("  t0       0x%016lx\n", gpr->t0);
    kprintf("  t1       0x%016lx\n", gpr->t1);
    kprintf("  t2       0x%016lx\n", gpr->t2);
    kprintf("  s0       0x%016lx\n", gpr->s0);
    kprintf("  s1       0x%016lx\n", gpr->s1);
    kprintf("  a0       0x%016lx\n", gpr->a0);
    kprintf("  a1       0x%016lx\n", gpr->a1);
    kprintf("  a2       0x%016lx\n", gpr->a2);
    kprintf("  a3       0x%016lx\n", gpr->a3);
    kprintf("  a4       0x%016lx\n", gpr->a4);
    kprintf("  a5       0x%016lx\n", gpr->a5);
    kprintf("  a6       0x%016lx\n", gpr->a6);
    kprintf("  a7       0x%016lx\n", gpr->a7);
    kprintf("  s2       0x%016lx\n", gpr->s2);
    kprintf("  s3       0x%016lx\n", gpr->s3);
    kprintf("  s4       0x%016lx\n", gpr->s4);
    kprintf("  s5       0x%016lx\n", gpr->s5);
    kprintf("  s6       0x%016lx\n", gpr->s6);
    kprintf("  s7       0x%016lx\n", gpr->s7);
    kprintf("  s8       0x%016lx\n", gpr->s8);
    kprintf("  s9       0x%016lx\n", gpr->s9);
    kprintf("  s10      0x%016lx\n", gpr->s10);
    kprintf("  s11      0x%016lx\n", gpr->s11);
    kprintf("  t3       0x%016lx\n", gpr->t3);
    kprintf("  t4       0x%016lx\n", gpr->t4);
    kprintf("  t5       0x%016lx\n", gpr->t5);
    kprintf("  t6       0x%016lx\n", gpr->t6);
}

static inline void print_pgfault(struct trapframe *tf) {
    // The page fault test is in kernel anyway, so print a 'K/' here
    kprintf("page falut at 0x%016lx: K/", tf->badvaddr);
    if (tf->cause == CAUSE_LOAD_PAGE_FAULT) {
        kprintf("R\n");
    } else if (tf->cause == CAUSE_STORE_PAGE_FAULT) {
        kprintf("W\n");
    } else {
        kprintf("0x%016lx\n", tf->cause);
    }
}

static int pgfault_handler(struct trapframe *tf) {
    extern struct mm_struct *check_mm_struct;
    struct mm_struct *mm;
    if (check_mm_struct != NULL) {
        assert(current == idleproc);
        mm = check_mm_struct;
    }
    else {        
        if (current == NULL) {
            print_trapframe(tf);
            print_pgfault(tf);
            panic("unhandled page fault.\n");
        }
        mm = current->mm;
    }
    if (!trap_in_kernel(tf)) {
        if (!USER_ACCESS(tf->badvaddr, tf->badvaddr + 1)) {
            kprintf("user read kernel!\n");
            print_trapframe(tf);
            return -1;
        }
    }
    return do_pgfault(mm, tf->status | 3, tf->badvaddr);
}

static volatile int in_swap_tick_event = 0;
extern struct mm_struct *check_mm_struct;

void interrupt_handler(struct trapframe *tf) {

    intptr_t cause = (tf->cause << 1) >> 1;
    uint32_t irq_source;
    switch (cause) {
        case IRQ_U_SOFT:
            kprintf("User software interrupt\n");
            break;
        case IRQ_S_SOFT:
            kprintf("Supervisor software interrupt\n");
            break;
        case IRQ_H_SOFT:
            kprintf("Hypervisor software interrupt\n");
            break;
        case IRQ_M_SOFT:
            kprintf("Machine software interrupt\n");
            break;
        case IRQ_U_TIMER:
            kprintf("User timer interrupt\n");
            break;
        case IRQ_M_TIMER:
            // "All bits besides SSIP and USIP in the sip register are
            // read-only." -- privileged spec1.9.1, 4.1.4, p59
            // In fact, Call sbi_set_timer will clear STIP, or you can clear it
            // directly.
            // clear_csr(sip, SIP_STIP);
            clock_set_next_event();
            
            // if(ticks % 100 == 0 && myid() == 1)
            //     kprintf("TIMER on 1\n");
            if(myid() == 0){ // TODO: this is not so symmetry
            // find a more elegant solution
                ++ticks;
                if(ticks % 100 == 0){
                    print_ticks();
                }
            }

            // run_timer_list();

            break;
        case IRQ_H_TIMER:
            kprintf("Hypervisor software interrupt\n");
            break;
        case IRQ_S_TIMER:
            kprintf("Supervisor software interrupt\n");
            break;
        case IRQ_U_EXT:
            kprintf("User external interrupt\n");
            break;
        case IRQ_S_EXT:
            kprintf("Supervisor external interrupt\n");
            break;
        case IRQ_H_EXT:
            kprintf("Hypervisor external interrupt\n");
            break;
        case IRQ_M_EXT:
            kprintf("Machine external interrupt\n");
            irq_source = *((uint32_t*)ADR_PLIC);
            switch(irq_source){
                case IRQ_SERIAL:
                    serial_intr();
                    break;
            }
            *((uint32_t*)ADR_PLIC) = irq_source;
            // dev_stdin_write(cons_getc());
            break;
        default:
            print_trapframe(tf);
            break;
    }
    clear_csr(mip, 1 << ((cause << 1) >> 1));
}

void exception_handler(struct trapframe *tf) {
    int ret;
    switch (tf->cause) {
        case CAUSE_MISALIGNED_FETCH:
            kprintf("Instruction address misaligned\n");
            break;
        case CAUSE_FETCH_ACCESS:
            kprintf("Instruction access fault\n");
            break;
        case CAUSE_ILLEGAL_INSTRUCTION:
            kprintf("Illegal instruction\n");
            break;
        case CAUSE_BREAKPOINT:
            kprintf("Breakpoint\n");
            break;
        case CAUSE_MISALIGNED_LOAD:
            kprintf("Load address misaligned\n");
            break;
        case CAUSE_LOAD_ACCESS:
            kprintf("Load access fault\n");
            break;
        case CAUSE_MISALIGNED_STORE:
            print_trapframe(tf);
            kprintf("AMO address misaligned\n");
            assert(0);
            break;
        case CAUSE_STORE_ACCESS:
            kprintf("Store/AMO access fault\n");
            break;
        case CAUSE_USER_ECALL:
            // kprintf("Environment call from U-mode\n");
            tf->epc += 4;
            syscall();
            break;
        case CAUSE_SUPERVISOR_ECALL:
            kprintf("Environment call from S-mode\n");
            tf->epc += 4;
            syscall();
            break;
        case CAUSE_HYPERVISOR_ECALL:
            kprintf("Environment call from H-mode\n");
            tf->epc += 4;
            break;
        case CAUSE_MACHINE_ECALL:
            kprintf("Environment call from M-mode\n");
            tf->epc += 4;
            syscall();
            break;
        case CAUSE_FETCH_PAGE_FAULT:
            print_trapframe(tf);
            panic("Instruction page fault\n");
            break;
        case CAUSE_LOAD_PAGE_FAULT:
            // kprintf("Load page fault\n");
            if ((ret = pgfault_handler(tf)) != 0) {
                if (current == NULL) {
                    panic("handle pgfault failed. ret=%d\n", ret);
                } else {
                    if (trap_in_kernel(tf)) {
                        panic("handle pgfault failed in kernel mode. ret=%d\n", ret);
                    }
                    kprintf("killed by kernel.\n");
                    do_exit(-E_KILLED);
                }
            }
            break;
        case CAUSE_STORE_PAGE_FAULT:
            // kprintf("Store/AMO page fault\n");
            if ((ret = pgfault_handler(tf)) != 0) {
                if (current == NULL) {
                    panic("handle pgfault failed. ret=%d\n", ret);
                } else {
                    if (trap_in_kernel(tf)) {
                        panic("handle pgfault failed in kernel mode. ret=%d\n", ret);
                    }
                    kprintf("killed by kernel.\n");
                    do_exit(-E_KILLED);
                }
            }
            break;
        default:
            print_trapframe(tf);
            break;
    }
}

static inline void trap_dispatch(struct trapframe* tf) {
    if ((intptr_t)tf->cause < 0) {
        // interrupts
        interrupt_handler(tf);
    } else {
        // exceptions
        exception_handler(tf);
    }
}

/* *
 * trap - handles or dispatches an exception/interrupt. if and when trap() returns,
 * the code in kern/trap/trapentry.S restores the old CPU state saved in the
 * trapframe and then uses the iret instruction to return from the exception.
 * */
void
trap(struct trapframe *tf) {
    // dispatch based on what type of trap has occurred
    if (current == NULL) {
        trap_dispatch(tf);
    } else {
        struct trapframe *otf = current->tf;
        current->tf = tf;

        bool in_kernel = trap_in_kernel(tf);
        trap_dispatch(tf);

        current->tf = otf;
        if (!in_kernel) {
            if (current->flags & PF_EXITING) {
                do_exit(-E_KILLED);
            }
            if (current->need_resched) {
                schedule();
            }
        }
    }
}


int ucore_in_interrupt()
{
	return 0;
}