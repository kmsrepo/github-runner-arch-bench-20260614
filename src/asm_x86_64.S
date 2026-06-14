.text
.global asm_integer_kernel
.type asm_integer_kernel, @function
asm_integer_kernel:
    testq %rdi, %rdi
    je .Lzero_iterations

    movq %rsi, %rax
    movabsq $0x9e3779b97f4a7c15, %rdx
    addq %rsi, %rdx
    movq %rax, %xmm0
    movq %rdx, %xmm4
    punpcklqdq %xmm4, %xmm0

    movdqa .Ladd_constants(%rip), %xmm1
    movdqa .Lmix_constants(%rip), %xmm2
    movdqa .Lxor_constants(%rip), %xmm3

.Lloop:
    paddq %xmm1, %xmm0
    movdqa %xmm0, %xmm4
    psrlq $31, %xmm4
    pxor %xmm4, %xmm0
    paddq %xmm2, %xmm0
    movdqa %xmm0, %xmm4
    psllq $17, %xmm4
    pxor %xmm4, %xmm0
    pxor %xmm3, %xmm0
    subq $1, %rdi
    jne .Lloop

    movq %xmm0, %rax
    pshufd $0x4e, %xmm0, %xmm0
    movq %xmm0, %rdx
    xorq %rdx, %rax
    ret

.Lzero_iterations:
    movq %rsi, %rax
    ret

.section .rodata
.balign 16
.Ladd_constants:
    .quad 0x9e3779b97f4a7c15, 0xbf58476d1ce4e5b9
.Lmix_constants:
    .quad 0x94d049bb133111eb, 0xd6e8feb86659fd93
.Lxor_constants:
    .quad 0xa0761d6478bd642f, 0xe7037ed1a0b428db
