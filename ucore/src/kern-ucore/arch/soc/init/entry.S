#include <mmu.h>
#include <memlayout.h>
#include <arch.h>

# KCODE is the kernel virtual address of the first byte of physical memory.
# The linker loads the executable as if starting at KCODE+2MB, but we
# ask the loader to load the kernel at physical 2MB and then set up the
# necessary memory mapping to switch to the higher address.
# The value of KCODE must match the definitions in kernel.h and kernel.ld.

#define KBASE 0xFFFFFFC000000000

# PADDR(x) is the physical memory address corresponding to x.
# Until we set up the memory map, fairly late in this file, we have to
# refer to PADDR(symbol) instead of symbol, so that we use the
# physical address.

#define PADDR(x) ((x) - KBASE)

    .section .text,"ax",%progbits
    .globl kern_entry
kern_entry:

    # setup paging
    la t0, __boot_pgdir
    srli t0, t0, RISCV_PGSHIFT
    li t1, SATP_SV39
    or t0, t0, t1
    csrw satp, t0

    # reallocate
    la t0, reallocated # note: we use mcmodel=medany, which is PC-relative addressing,
    li t1, KBASE # so following additions are needed.
    add t0, t0, t1
    add a1, a1, t1 # reallocate fdt
    jr t0

reallocated:

    # Load VA stack pointer
    la sp, stack + KSTACKSIZE
    slli t0, a0, KSTACKSHIFT
    add sp, sp, t0

    mv s0, zero # clear frame pointer

    tail kern_init

1:
    wfi
    j 1b


.section .data
    .align RISCV_PGSHIFT
    # Initial stack
    .comm stack, KSTACKSIZE * 8 # FIXME: Most 8 processers.

    .align RISCV_PGSHIFT
    .global __boot_pgdir
__boot_pgdir:
    .space PUX(DRAM_BASE) * PTE_SIZE - (. - __boot_pgdir)
    # map va DRAM_BASE + (0G~1G) to pa DRAM_BASE + (0G~1G) (temporary)
    .quad DRAM_BASE / 4 + (PTE_V | PTE_R | PTE_W | PTE_X)
    .space PUX(KBASE) * PTE_SIZE - (. - __boot_pgdir)
    # map va KBASE + (0G~4G) to pa 0G~4G
    .quad 0x00000000 / 4 + (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G)
    .quad 0x40000000 / 4 + (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G)
    .quad 0x80000000 / 4 + (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G)
    .quad 0xC0000000 / 4 + (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G)
    .space PGSIZE - (. - __boot_pgdir)