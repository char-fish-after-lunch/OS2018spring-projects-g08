#include <assert.h>
#include <types.h>
#include <stdio.h>
#include <kio.h>


struct stab {
	uint32_t st_name;	// index into string table of name
	uint8_t st_info;		// type of symbol
	uint8_t st_other;	// misc info (usually empty)
	uint16_t sh_shndx;	// description field
	uintptr_t st_value;	// value of symbol
    uint64_t st_size;
};


struct stab* real_stab_begin;
extern struct stab __STAB_BEGIN__[];	// beginning of stabs table
extern struct stab __STAB_END__[];	// end of stabs table
extern char __STABSTR_BEGIN__[];	// beginning of string table
extern char __STABSTR_END__[];	// end of string table

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void) {
    extern char etext[], edata[], end[], kern_init[];
    kprintf("Special kernel symbols:\n");
    kprintf("  entry  0x%016llx (virtual)\n", kern_init);
    kprintf("  etext  0x%016llx (virtual)\n", etext);
    kprintf("  edata  0x%016llx (virtual)\n", edata);
    kprintf("  end    0x%016llx (virtual)\n", end);
    kprintf("Kernel executable memory footprint: %dKB\n",
            (end - kern_init + 1023) / 1024);
}



void print_debuginfo(uintptr_t eip){
    panic("Not implemented.\n");
}

void kdebug_init(void){
    kprintf("INIT!!!\n");
    real_stab_begin = __STAB_BEGIN__ + 1;
    kprintf("%016llx %016llx %016llx\n", __STAB_BEGIN__, real_stab_begin, __STAB_END__);
    int N = __STAB_END__ - real_stab_begin;
    struct stab tmp;
    for(int i = 0; i < N; i ++){
        for(int j = i + 1; j < N; j ++){
            if(real_stab_begin[j].st_value < real_stab_begin[i].st_value){
                tmp = real_stab_begin[i];
                real_stab_begin[i] = real_stab_begin[j];
                real_stab_begin[j] = tmp;
            }
        }
    }
    kprintf("LAST %016llx, %016llx\n", real_stab_begin[0].st_value, real_stab_begin[N - 1].st_value);
}

static char* lookup_addr(uintptr_t eip, uintptr_t *offset) { 
    return "(unknown)";
    // FIXME:!!!
    // binary search
    int N = __STAB_END__ - real_stab_begin;
    // kprintf("%s\n", __STABSTR_BEGIN__[real_stab_begin[800].st_name]);
    // kprintf("TOTAL %d\n", N);
    kprintf("DD %016llx %016llx\n", real_stab_begin, __STAB_END__);
    int l = 0, r = N - 1;
    while(l < r){
        int m = (l + r) >> 1;
        if(__STAB_BEGIN__[m].st_value > eip)
            r = m;
        else
            l = m + 1;
    }
    if(__STAB_BEGIN__[l].st_value > eip)
        -- l;
    if(l < 0){
        *offset = eip;
        return "(unknown)";
    }
    *offset = eip - real_stab_begin[l].st_value;
    kprintf("FOUND %d. %d\n", l, real_stab_begin[l].st_name);
    return __STABSTR_BEGIN__ + real_stab_begin[l].st_name;
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
        uintptr_t offset;
        char* symb = lookup_addr(pc, &offset);
        kprintf("\tPC=%016llx <%s+%0llx>, FP=%016llx, SP=%016llx\n",  pc, symb, offset, fp, sp);
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

