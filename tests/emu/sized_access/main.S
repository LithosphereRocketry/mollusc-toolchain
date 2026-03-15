
.section text
#include "../platform.inc"

_start:
        ; check for sized access feature
        j ra, init
        ld.cr t0, zero, zero
        and t0, t0, 0x10
        eq p1, t0, zero
?p1     add a0, zero, 0xFF
?p1     j ra, exit
        
        add sp, sp, -4
        st zero, sp, 0
        lui t0, 0x1234
        add t0, t0, 0x1234
        st.h t0, sp, 2
        add t0, zero, 0x56
        st.b t0, sp, 1

        ld s2, sp, zero
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
