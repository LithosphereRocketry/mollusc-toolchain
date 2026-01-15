.section text
    j ra, init
    ld.cr s2, zero, zero
    add a0, zero, s2
    j ra, putc
    sr a0, s2, 8
    j ra, putc
    sr a0, s2, 16
    j ra, putc
    sr a0, s2, 24
    j ra, putc
    add a0, zero, zero
    j ra, exit

#include "../platform.inc"