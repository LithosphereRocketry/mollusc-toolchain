
.section .text
#include "../platform.inc"

_start:
        j ra, init
        lui s2, 0x8000
        add a0, zero, 65
        st a0, s2, 0x100
        add a0, zero, zero
        ld a0, s2, 0x100
        j ra, putc
        add a0, zero, zero
        j ra, exit
