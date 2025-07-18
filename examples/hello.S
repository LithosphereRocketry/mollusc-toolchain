.section text
.global main

main:   
        j ra, fibonacci
halt:
        j zero, halt

fibonacci:
        add a1, zero, 1
        add a2, zero, 0
fibonacci_loop:
        eq p1, a0, 0
    ?p1 add a0, zero, a2
    ?p1 jx zero, zero, ra
        eq p1, a0, 1
    ?p1 add a0, zero, a1
    ?p1 jx zero, zero, ra
        add a2, a1, a2
        add a1, a2, a1
        sub a0, a0, 2
        j zero, fibonacci_loop

