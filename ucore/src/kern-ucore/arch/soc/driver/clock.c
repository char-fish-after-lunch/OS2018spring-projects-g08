#include <clock.h>
#include <types.h>
#include <sbi.h>
#include <kio.h>
#include <stdio.h>
#include <arch.h>
#include <smp.h>


#define ADR_CMPL 0b11000000011111111111111111000000
#define ADR_CMPH 0b11000000011111111111111111000100
#define ADR_TMEL 0b11000000011111111111111111001000
#define ADR_TMEH 0b11000000011111111111111111001100
#define ADR_MSIP 0b11000000011111111111111111010000

volatile size_t ticks;

static inline uint64_t get_cycles(void) {
#if __riscv_xlen == 64
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
#else
    uint32_t lo, hi, tmp;
    __asm__ __volatile__(
        "1:\n"
        "rdtimeh %0\n"
        "rdtime %1\n"
        "rdtimeh %2\n"
        "bne %0, %2, 1b"
        : "=&r"(hi), "=&r"(lo), "=&r"(tmp));
    return ((uint64_t)hi << 32) | lo;
#endif
}

static uint64_t timebase;

/* *
 * clock_init - initialize 8253 clock to interrupt 100 times per second,
 * and then enable IRQ_TIMER.
 * */
void clock_init(void) {
    // divided by 500 when using Spike(2MHz)
    // divided by 100 when using QEMU(10MHz)

    *((unsigned*)ADR_CMPH) = 0;
    *((unsigned*)ADR_CMPL) = 125000;

    timebase = 1e7 / 100;
    clock_set_next_event();

    set_csr(mie, MIP_MTIP);

    // initialize time counter 'ticks' to zero
    ticks = 0;

    kprintf("++ setup timer interrupts\n");
}

void clock_set_next_event(void){
    *((unsigned*)ADR_TMEH) = 0;
    *((unsigned*)ADR_TMEL) = 0;
}
