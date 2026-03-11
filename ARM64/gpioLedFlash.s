.equ O_RDWR, 00000002
.equ O_DSYNC, 00010000
.equ MAP_SHARED, 1
.equ PROT_READ, 1
.equ PROT_WRITE, 2
.equ GPIO0_SET, 0x1c
.equ GPIO0_CLR, 0x28
.equ GPIO_BASE_ADDRESS, 0xfe200000
.equ loopamount, 0x10000000
.section .text
.global _start

_start:
        mov x8, #56
        mov x0, 0x0
        ldr x1, =File
        mov x2, #(O_RDWR+O_DSYNC)
        svc 0
        mov x9, x0
        bpl Mmap
        mov x8, #40
        mov x0, #1
        ldr x1, =errmessage
        ldr x2, =errlen
        svc 0
        b errexit

Mmap:
        mov x8, #0xDE
        mov x0, #0
        eor x1, x1, x1
        mov x1, #0x1000
        mov x2, #(PROT_READ + PROT_WRITE)
        mov x3, #MAP_SHARED
        mov x4, x9
        ldr x5, =GPIO_BASE_ADDRESS
        svc 0
        mov x10, x0
        bpl OutputEnable
        mov x8, #40
        mov x0, #1
        ldr x1, =mmaperrmessage
        ldr x2, =errlen
        svc 0
        b errexit

OutputEnable:
        eor x1, x1, x1
        mov x1, #1
        str w1, [x10]
        bpl Flash
        mov x8, #40
        mov x0, #1
        ldr x1, =outputerrmessage
        ldr x2, =outputlen
        svc 0
        b errexit

Flash:
//      ldr x0, [x10,
        eor x1, x1, x1
        mov x1, #1
        str w1, [x10, GPIO0_SET]
        eor x11, x11, x11
        ldr x12, =loopamount
        loop1:
                add x11, x11, #1
                cmp x11, x12
                bne loop1
        eor x11, x11, x11
        eor x1, x1, x1
        mov x1, #1
        str w1, [x10, GPIO0_CLR]
        loop2:
                add x11, x11, #1
                cmp x11, x12
                bne loop2
        b Flash

errexit:
        mov x8, #0x5D
        mov x0, #1
        svc 0

exit:
        mov x8, #0x5D
        mov x0, #0
        svc 0

.data
File:
        .asciz "/dev/mem"
mmaperrmessage:
        .asciz "mmap() failed"
mmaplen = .-mmaperrmessage
errmessage:
        .asciz "Couldn't open file"
errlen = .-errmessage
outputerrmessage:
        .asciz "Couldn't set gpio 0 to output"
outputlen = .-outputerrmessage
