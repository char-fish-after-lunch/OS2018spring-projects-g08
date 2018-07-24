#include <intr.h>
#include <arch.h>

/* intr_enable - enable irq interrupt */
void intr_enable(void) { set_csr(mstatus, MSTATUS_MIE); }

/* intr_disable - disable irq interrupt */
void intr_disable(void) { clear_csr(mstatus, MSTATUS_MIE); }
