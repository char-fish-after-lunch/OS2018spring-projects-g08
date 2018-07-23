#include <clock.h>
#include <console.h>
#include <types.h>
#include <stdio.h>
#include <intr.h>
#include <kdebug.h>
#include <monitor.h>
#include <picirq.h>
#include <pmm.h>
#include <arch.h>
#include <kio.h>
#include <string.h>
#include <trap.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#if defined(ARCH_RISCV64) || defined(ARCH_SOC)
#include <smp.h>
#else
#include <mp.h>
#endif
//#include <mod.h>

int kern_init(uintptr_t hartid, uintptr_t good) __attribute__((noreturn));
void grade_backtrace(void);
static void lab1_switch_test(void);
static volatile int bsp_started;

extern struct cpu cpus[];

static void ap_init(uintptr_t hartid, uintptr_t good){
    load_pgdir(NULL);
    intr_enable();  // enable irq interrupt

    pic_init();  // init interrupt controller
    idt_init();  // init interrupt descriptor table

    proc_init_ap();
    mycpu()->started = 1;
    kprintf("AP %d has started.\n", myid());

    clock_init();  // init clock interrupt
    intr_enable();

    cpu_idle();                 // run idle process    
}

static void start_others(){
    bsp_started = 1;
    int my = myid(), i;
    for(i = 0; i < NCPU; i ++){
        if(i == my)
            continue;
        while(!cpus[i].started);
    }
}

int kern_init(uintptr_t hartid, uintptr_t good) {
    asm volatile ("mv tp, %0;" : : "r"(cpus + hartid));
    if(hartid != 0){
        // wait for bsp to do init work
        while(!bsp_started);
        ap_init(hartid, good); // not expected to return
    }

    extern char edata[], end[];
    memset(edata, 0, end - edata);

    cons_init();  // init the console

    const char *message = "(THU.CST) os is loading ...\n";
    kprintf("%s\n\n", message);

    smp_init();
    
    print_kerninfo();

    /* Only to initialize lcpu_count. */
    /* We don't support NUMA. */
	// mp_init();

	size_t nr_used_pages_store = nr_used_pages();

    assert(myid() == hartid);
	//debug_init();		// init debug registers
    pmm_init();  // init physical memory management
    
    /* We don't support NUMA. */
    // pmm_init_ap();



    pic_init();  // init interrupt controller
    idt_init();  // init interrupt descriptor table

    vmm_init();                 // init virtual memory management
    sched_init();		// init scheduler
    proc_init();                // init process table
    sync_init();		// init sync struct



    ide_init();                 // init ide devices
    start_others();


#ifdef UCONFIG_SWAP
	swap_init();		// init swap
#endif
    fs_init();
    // rdtime in mbare mode crashes
    clock_init();  // init clock interrupt
    //mod_init();


    intr_enable();  // enable irq interrupt

    cpu_idle();                 // run idle process
}

void __attribute__((noinline))
grade_backtrace2(int arg0, int arg1, int arg2, int arg3) {
    mon_backtrace(0, NULL, NULL);
}

void __attribute__((noinline)) grade_backtrace1(int arg0, int arg1) {
    grade_backtrace2(arg0, (int)&arg0, arg1, (int)&arg1);
}

void __attribute__((noinline)) grade_backtrace0(int arg0, int arg1, int arg2) {
    grade_backtrace1(arg0, arg2);
}

void grade_backtrace(void) { grade_backtrace0(0, (int)kern_init, 0xffff0000); }