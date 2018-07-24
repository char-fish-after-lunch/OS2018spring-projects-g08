#include <console.h>
#include <types.h>
#include <sbi.h>
#include <sync.h>

#define CONSBUFSIZE 512

#define ADR_SERIAL_DAT 0xf000
#define ADR_SERIAL_BUF 0xf004

static struct {
    uint8_t buf[CONSBUFSIZE];
    uint32_t rpos;
    uint32_t wpos;
} cons;

/* *
 * cons_intr - called by device interrupt routines to feed input
 * characters into the circular console input buffer.
 * */
void cons_intr(int (*proc)(void)) {
    int c;
    while ((c = (*proc)()) != -1) {
        if (c != 0) {
            cons.buf[cons.wpos++] = c;
            if (cons.wpos == CONSBUFSIZE) {
                cons.wpos = 0;
            }
        }
    }
}

/* kbd_intr - try to feed input characters from keyboard */
void kbd_intr(void) {
    serial_intr();
}

/* serial_proc_data - get data from serial port */
int serial_proc_data(void) {
    if(!(*((volatile unsigned*)ADR_SERIAL_BUF) & 0xf0))
        return -1;
    int c = *((int*)ADR_SERIAL_DAT);
    if (c == 127) {
    //TODO: don't know what this does
        c = '\b';
    }
    return c;
}

/* serial_intr - try to feed input characters from serial port */
void serial_intr(void) {
    cons_intr(serial_proc_data);
}

/* serial_putc - print character to serial port */
void serial_putc(int c) {
    while(!(*((volatile unsigned*)ADR_SERIAL_BUF) & 0xf));
    *((unsigned*)ADR_SERIAL_DAT) = (unsigned)c;
}

/* cons_init - initializes the console devices */
void cons_init(void) {
    sbi_console_getchar();
}

/* cons_putc - print a single character @c to console devices */
void cons_putc(int c) {
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        serial_putc(c);
    }
    local_intr_restore(intr_flag);
}

/* *
 * cons_getc - return the next input character from console,
 * or 0 if none waiting.
 * */
int cons_getc(void) {
    int c = 0;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        // poll for any pending input characters,
        // so that this function works even when interrupts are disabled
        // (e.g., when called from the kernel monitor).
        serial_intr();

        // grab the next character from the input buffer.
        if (cons.rpos != cons.wpos) {
            c = cons.buf[cons.rpos++];
            if (cons.rpos == CONSBUFSIZE) {
                cons.rpos = 0;
            }
        }
    }
    local_intr_restore(intr_flag);
    return c;
}
