.text
.global sustained_simd_lanes
.type sustained_simd_lanes, %function
sustained_simd_lanes:
    cntd x0
    ret

.global sustained_simd_kernel
.type sustained_simd_kernel, %function
sustained_simd_kernel:
    cbz x0, .Lzero_iterations

    ptrue p0.d
    dup z0.d, x1
    index z5.d, #0, #1
    add z0.d, z0.d, z5.d

    mov x2, #0x7c15
    movk x2, #0x7f4a, lsl #16
    movk x2, #0x79b9, lsl #32
    movk x2, #0x9e37, lsl #48
    dup z1.d, x2

    mov x2, #0x5b9
    movk x2, #0xce4e, lsl #16
    movk x2, #0x476d, lsl #32
    movk x2, #0xbf58, lsl #48
    dup z2.d, x2

    mov x2, #0x11eb
    movk x2, #0x1331, lsl #16
    movk x2, #0x49bb, lsl #32
    movk x2, #0x94d0, lsl #48
    dup z3.d, x2

.Lloop:
    add z0.d, z0.d, z1.d
    lsr z4.d, z0.d, #31
    eor z0.d, z0.d, z4.d
    add z0.d, z0.d, z2.d
    lsl z4.d, z0.d, #17
    eor z0.d, z0.d, z4.d
    eor z0.d, z0.d, z3.d
    subs x0, x0, #1
    b.ne .Lloop

    cntd x2
    lsl x3, x2, #3
    add x3, x3, #15
    and x3, x3, #0xfffffffffffffff0
    sub sp, sp, x3
    st1d { z0.d }, p0, [sp]

    mov x0, #0
    mov x4, #0
.Lreduce:
    ldr x5, [sp, x4, lsl #3]
    eor x0, x0, x5
    add x4, x4, #1
    cmp x4, x2
    b.ne .Lreduce

    add sp, sp, x3
    ret

.Lzero_iterations:
    mov x0, x1
    ret
