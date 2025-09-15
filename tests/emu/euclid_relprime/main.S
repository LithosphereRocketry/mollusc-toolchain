.section text

    j ra, init
    ; return relprime(0x13b0);
    lui a0, 0x13b0
    add a0, a0, 0x13b0
    j ra, relprime
    j ra, putc
    add a0, zero, 0
    j ra, exit

relprime: ; int relprime(int n) {
    sub sp, sp, 12
    stp ra, sp, 0
    stp s2, sp, 4
    stp s3, sp, 8
    add s2, zero, a0
    add s3, zero, 2 ;   int m = 2;

relprime_loop:
;   while(
    add a0, zero, s2
    add a1, zero, s3
    j ra, gcd    ; gcd(n, m)
    eq !p1, a0, 1    ; != 1) {
?p1 add s3, s3, 1   ;     m++;
?p1 j zero, relprime_loop ;   }
    add a0, zero, s3
    ldp ra, sp, 0
    ldp s2, sp, 4
    ldp s3, sp, 8
    jx zero, zero, ra ;   return m; }

gcd: ; int gcd(int a, int b) {
    eq p1, a1, 0        ; if(b == 0)
?p1 jx zero, zero, ra   ;   return a; 

gcd_loop: ;   while(
    eq p1, a0, a1 ; a != b
?p1 j zero, gcd_done ;) {
    lt p1, a1, a0 ;     if(a > b)
?p1 sub a0, a0, a1 ;     { a -= b; } 
!p1 sub a1, a1, a0 ;     else { b -= a; }
    j zero, gcd_loop ;   }
gcd_done:
    add a0, zero, a1    ;   return b;
    jx zero, zero, ra ; }

#include "../platform.inc"