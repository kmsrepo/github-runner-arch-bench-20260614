.text
.global sustained_simd_lanes
.type sustained_simd_lanes, @function
sustained_simd_lanes:
    movl $4, %eax
    ret

.global sustained_simd_kernel
.type sustained_simd_kernel, @function
sustained_simd_kernel:
    testq %rdi, %rdi
    je .Lzero_iterations

    vmovdqa .Llane_offsets(%rip), %ymm5
    vpbroadcastq %rsi, %ymm0
    vpaddq %ymm5, %ymm0, %ymm0
    vmovdqa .Ladd_constants(%rip), %ymm1
    vmovdqa .Lmix_constants(%rip), %ymm2
    vmovdqa .Lxor_constants(%rip), %ymm3

.Lloop:
    vpaddq %ymm1, %ymm0, %ymm0
    vpsrlq $31, %ymm0, %ymm4
    vpxor %ymm4, %ymm0, %ymm0
    vpaddq %ymm2, %ymm0, %ymm0
    vpsllq $17, %ymm0, %ymm4
    vpxor %ymm4, %ymm0, %ymm0
    vpxor %ymm3, %ymm0, %ymm0
    subq $1, %rdi
    jne .Lloop

    vextracti128 $1, %ymm0, %xmm1
    vpxor %xmm1, %xmm0, %xmm0
    vpshufd $0x4e, %xmm0, %xmm1
    vpxor %xmm1, %xmm0, %xmm0
    vmovq %xmm0, %rax
    vzeroupper
    ret

.Lzero_iterations:
    movq %rsi, %rax
    ret

.section .rodata
.balign 32
.Llane_offsets:
    .quad 0x0000000000000000, 0x9e3779b97f4a7c15
    .quad 0xbf58476d1ce4e5b9, 0x94d049bb133111eb
.Ladd_constants:
    .quad 0x9e3779b97f4a7c15, 0xbf58476d1ce4e5b9
    .quad 0x94d049bb133111eb, 0xd6e8feb86659fd93
.Lmix_constants:
    .quad 0xa0761d6478bd642f, 0xe7037ed1a0b428db
    .quad 0x8ebc6af09c88c6e3, 0x589965cc75374cc3
.Lxor_constants:
    .quad 0x1d8e4e27c47d124f, 0xeb44accab455d165
    .quad 0x6b05c2d7b3eade77, 0xa4093822299f31d0
