#include <mmu.h>
#include <memlayout.h>
#include <arch.h>

# KCODE is the kernel virtual address of the first byte of physical memory.
# The linker loads the executable as if starting at KCODE+2MB, but we
# ask the loader to load the kernel at physical 2MB and then set up the
# necessary memory mapping to switch to the higher address.
# The value of KCODE must match the definitions in kernel.h and kernel.ld.

#define KBASE 0xC0000000

# PADDR(x) is the physical memory address corresponding to x.
# Until we set up the memory map, fairly late in this file, we have to
# refer to PADDR(symbol) instead of symbol, so that we use the
# physical address.

#define PADDR(x) ((x) - KBASE)

    .section .text,"ax",%progbits
    .globl kern_entry
kern_entry:
    # only one hart for now

    # setup boot pgdir
    la a2, PADDR(__boot_pgdir)
    srli a2, a2, 2
    addi a2, a2, 1024
    ori a2, a2, PTE_V

    la a1, __boot_pgdir + (KBASE >> 20)
    sw a2, 0(a1)
    addi a2, a2, 1024
    sw a2, 4(a1)

    la t0, PADDR(__boot_pgdir)
    srli t0, t0, RISCV_PGSHIFT
    li t1, 1 << 31
    or t0, t0, t1
    csrw satp, t0

    addi a0, x0, 0
    la sp, stack + KSTACKSIZE

    tail kern_init

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
    .space PGSIZE
    _pt_start:
    .rept   2048
    .long (PTE_V | PTE_R | PTE_W | PTE_X | PTE_G) | (((. - _pt_start) >> 2) << 10)
    .endr