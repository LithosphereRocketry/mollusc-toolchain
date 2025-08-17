.section text

    j ra, init
    la hellorld
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
    ldp s3, s2, zero
    add zero, zero, zero
puts_shiftloop:
    and a0, s3, 0xFF
!a0 j zero, puts_done
    j ra, putc
    sub s4, s4, 1
    sr s3, s3, 8
?s4 j zero, puts_shiftloop
    add s2, s2, 4
    j zero, puts_loop

puts_done:
    ldp ra, sp, 0
    ldp s2, sp, 4
    ldp s3, sp, 8
    ldp s4, sp, 12
    add sp, sp, 16
    jx zero, zero, ra

hellorld:
.asciz "Hellorld!\n"