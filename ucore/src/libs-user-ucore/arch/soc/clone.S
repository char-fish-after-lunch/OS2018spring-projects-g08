#include <unistd.h>

.text
.globl __clone
__clone:                        # __clone(clone_flags, stack, fn, arg)
		// !TODO: implement this
1:
    j 1b
