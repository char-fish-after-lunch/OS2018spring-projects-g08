#include <assert.h>
#include <types.h>
#include <stdio.h>
#include <kio.h>

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void) {
    extern char etext[], edata[], end[], kern_init[];
    kprintf("Special kernel symbols:\n");
    kprintf("  entry  0x%08x (virtual)\n", kern_init);
    kprintf("  etext  0x%08x (virtual)\n", etext);
    kprintf("  edata  0x%08x (virtual)\n", edata);
    kprintf("  end    0x%08x (virtual)\n", end);
    kprintf("Kernel executable memory footprint: %dKB\n",
            (end - kern_init + 1023) / 1024);
}



void print_debuginfo(uintptr_t eip){
    panic("Not implemented.\n");
}

struct stackframe {
	uintptr_t fp;
    uintptr_t ra;
};


void print_stackframe(void) {
    // panic("Not Implemented!");
    uintptr_t fp, sp, pc;
    fp = (uintptr_t)__builtin_frame_address(0);
    asm("mv %0, sp;" : "=r"(sp));
    pc = (uintptr_t)print_stackframe;

    kprintf("Stack frame:\n");

    while(1){
        kprintf("\tPC=%08x , FP=%08x, SP=%08x\n",  pc, fp, sp);
        uintptr_t low, high;
        struct stackframe* frame;

        low = sp + sizeof(struct stackframe);
        high = (sp + (4096 << 1)) & ~((4096 << 1) - 1);
        if(fp < low || fp > high || fp & 0x7)
            break;
        frame = (struct stackframe *)fp - 1;
		sp = fp;
		fp = frame->fp;
		pc = frame->ra - 0x8;
    }
}

