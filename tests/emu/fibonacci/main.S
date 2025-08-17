.section text
    j ra, init

    add a0, zero, 3
    add a1, zero, 0
    add a2, zero, 1

loop:
    sub a0, a0, 1
    add a3, a2, a1
    add a1, zero, a2
    add a2, zero, a3
    eq p1, a0, 0
!p1 j zero, loop

    add s2, zero, 4
    add s3, zero, a1
print_loop:
    and a0, s3, 0xFF
    j ra, putc
    sr s3, s3, 8
    sub s2, s2, 1
    eq p1, s2, 0
!p1 j zero, print_loop

    add a0, zero, zero
    j ra, exit

#include "../platform.inc"