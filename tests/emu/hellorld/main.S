.section text

    j ra, init
    lr a0, hellorld
    j ra, puts
    add a0, zero, zero
    j ra, exit

puts:
    sub sp, sp, 16
    stp ra, sp, 0
    stp s2, sp, 4
    stp s3, sp, 8
    stp s4, sp, 12

    add s2, zero, a0

puts_loop:
    add s4, zero, 4
    ld s3, s2, zero
    add zero, zero, zero
puts_shiftloop:
    and a0, s3, 0xFF
    eq p1, a0, zero
?p1 j zero, puts_done
    j ra, putc
    sub s4, s4, 1
    sr s3, s3, 8
    eq p1, s4, zero
!p1 j zero, puts_shiftloop
    add s2, s2, 4
    j zero, puts_loop

puts_done:
    ld ra, sp, 0
    ld s2, sp, 4
    ld s3, sp, 8
    ld s4, sp, 12
    add sp, sp, 16
    jx zero, zero, ra

hellorld:
.asciz "Hellorld!\n"

#include "../platform.inc"

