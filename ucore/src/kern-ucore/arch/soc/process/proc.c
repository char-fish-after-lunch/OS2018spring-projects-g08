#include <proc.h>
#include <pmm.h>
#include <slab.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>
#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
#include <smp.h>
#else
#include <mp.h>
#endif
#include <memlayout.h>

void forkret(void);
void forkrets(struct trapframe *tf);

// alloc_proc - create a proc struct and init fields
struct proc_struct *alloc_proc(void)
{
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
		proc->state = PROC_UNINIT;
		proc->pid = -1;
		proc->runs = 0;
		proc->kstack = 0;
		proc->need_resched = 0;
		proc->parent = NULL;
		proc->mm = NULL;
		memset(&(proc->context), 0, sizeof(struct context));
		proc->tf = NULL;
		proc->cr3 = boot_cr3;
		proc->flags = 0;
		memset(proc->name, 0, PROC_NAME_LEN);
		proc->wait_state = 0;
		proc->cptr = proc->optr = proc->yptr = NULL;
		list_init(&(proc->thread_group));
		proc->rq = NULL;
		list_init(&(proc->run_link));
		proc->time_slice = 0;
		proc->sem_queue = NULL;
		event_box_init(&(proc->event_box));
		proc->fs_struct = NULL;
		proc->cpu_affinity = myid();
		spinlock_init(&proc->lock);
	}
	return proc;
}

int
copy_thread(uint32_t clone_flags, struct proc_struct *proc, uintptr_t rsp,
	    struct trapframe *tf)
{
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    *(proc->tf) = *tf;

    // Set a0 to 0 so a child process knows it's just forked
    proc->tf->gpr.a0 = 0;
    proc->tf->gpr.sp = (rsp == 0) ? (uintptr_t)proc->tf : rsp;

    proc->context.ra = (uintptr_t)forkret;
    proc->context.sp = (uintptr_t)(proc->tf);

	return 0;
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
    struct trapframe tf;
    memset(&tf, 0, sizeof(struct trapframe));
    tf.gpr.s0 = (uintptr_t)fn;
    tf.gpr.s1 = (uintptr_t)arg;
    tf.status = (read_csr(mstatus) | MSTATUS_MPP | MSTATUS_MPIE) & ~MSTATUS_MIE;
    tf.epc = (uintptr_t)kernel_thread_entry;
    return do_fork(clone_flags | CLONE_VM |__CLONE_PINCPU, 0, &tf);
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
void forkret(void)
{
	if (!trap_in_kernel(current->tf)) {
		/* We don't support NUMA. */
		// kern_leave();
	}
	forkrets(current->tf);
}

int kernel_execve(const char *name, const char **argv, const char **kenvp)
{
	kprintf("kernel execve:::\n");
	uintptr_t argc = 0, ret;
    while (argv[argc] != NULL) {
        argc ++;
    }
    asm volatile(
        "li a0, %1\n"
        "lw a1, %2\n"
        "lw a2, %3\n"
        "lw a3, %4\n"
        "li a7, 20\n"
        "ecall\n"
        "sw a0, %0"
        : "=m"(ret)
        : "i"(SYS_exec), "m"(name), "m"(argv), "m"(kenvp)
        : "memory");
    return ret;
}

int
init_new_context(struct proc_struct *proc, struct elfhdr *elf,
		 int argc, char **kargv, int envc, char **kenvp)
{
	uintptr_t stacktop = USTACKTOP - argc * PGSIZE;
	char **uargv = (char **)(stacktop - argc * sizeof(char *));
	int i;
	for (i = 0; i < argc; i++) {
		uargv[i] = strcpy((char *)(stacktop + i * PGSIZE), kargv[i]);
	}
	stacktop = (uintptr_t) uargv - sizeof(uintptr_t);
	*(uintptr_t *)stacktop = argc;

	struct trapframe *tf = proc->tf;
	// Keep sstatus
	uintptr_t mstatus = tf->status;
	memset(tf, 0, sizeof(struct trapframe));

	tf->gpr.sp = stacktop;
	tf->epc = elf->e_entry;
	tf->status = mstatus & ~(MSTATUS_MPP);
	return 0;
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void cpu_idle(void)
{
	// while (1) {
	// 	assert((read_rflags() & FL_IF) != 0);
	// 	asm volatile ("hlt");
	// }
    while (1) {
        if (current->need_resched) {
            schedule();
        }
    }
}

int do_execve_arch_hook(int argc, char **kargv)
{
	return 0;
}

int ucore_kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	kernel_thread(fn, arg, clone_flags);
}

void de_thread_arch_hook(struct proc_struct *proc)
{
}
