#include <unistd.h>
#include <types.h>
#include <stdarg.h>
#include <syscall.h>
#include <mboxbuf.h>
#include <stat.h>
#include <dirent.h>

#define MAX_ARGS            5

int syscall(int num, ...) {
    va_list ap;
    va_start(ap, num);
    uintptr_t a[MAX_ARGS];
    int i, ret;
    for (i = 0; i < MAX_ARGS; i ++) {
        a[i] = va_arg(ap, uintptr_t);
    }
    va_end(ap);

    asm volatile (
        "lw a0, %1\n"
        "lw a1, %2\n"
        "lw a2, %3\n"
        "lw a3, %4\n"
        "lw a4, %5\n"
        "lw a5, %6\n"
        "ecall\n"
        "sw a0, %0"
        : "=m" (ret)
        : "m" (num),
          "m" (a[0]),
          "m" (a[1]),
          "m" (a[2]),
          "m" (a[3]),
          "m" (a[4])
        : "memory"
      );
    return ret;
}