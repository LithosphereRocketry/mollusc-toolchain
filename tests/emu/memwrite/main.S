
.section text
        j ra, init
        add a0, zero, 65
        st a0, zero, 0x100
        add a0, zero, zero
        ld a0, zero, 0x100
        j ra, putc
        add a0, zero, zero
        j ra, exit

#include "../platform.inc"