
.section text
        j ra, init
        add a0, zero, 65
        j ra, putc
        add a0, zero, zero
        j ra, exit

#include "../platform.inc"