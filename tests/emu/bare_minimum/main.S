
.section text
#include "../platform.inc"

_start:
        j ra, init
        add a0, zero, 65
        j ra, putc
        add a0, zero, zero
        j ra, exit
