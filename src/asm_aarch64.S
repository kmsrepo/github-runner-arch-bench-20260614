.text
.global asm_integer_kernel
.type asm_integer_kernel, %function
asm_integer_kernel:
    cbz x0, .Lzero_iterations

    mov x4, #0x7c15
    movk x4, #0x7f4a, lsl #16
    movk x4, #0x79b9, lsl #32
    movk x4, #0x9e37, lsl #48
    add x4, x1, x4
    dup v0.2d, x1
    ins v0.d[1], x4

    adr x5, .Ladd_constants
    ldr q1, [x5]
    adr x5, .Lmix_constants
    ldr q2, [x5]
    adr x5, .Lxor_constants
    ldr q3, [x5]

.Lloop:
    add v0.2d, v0.2d, v1.2d
    ushr v4.2d, v0.2d, #31
    eor v0.16b, v0.16b, v4.16b
    add v0.2d, v0.2d, v2.2d
    shl v4.2d, v0.2d, #17
    eor v0.16b, v0.16b, v4.16b
    eor v0.16b, v0.16b, v3.16b
    subs x0, x0, #1
    b.ne .Lloop

    umov x0, v0.d[0]
    umov x2, v0.d[1]
    eor x0, x0, x2
    ret

.Lzero_iterations:
    mov x0, x1
    ret

.section .rodata
.balign 16
.Ladd_constants:
    .quad 0x9e3779b97f4a7c15, 0xbf58476d1ce4e5b9
.Lmix_constants:
    .quad 0x94d049bb133111eb, 0xd6e8feb86659fd93
.Lxor_constants:
    .quad 0xa0761d6478bd642f, 0xe7037ed1a0b428db
