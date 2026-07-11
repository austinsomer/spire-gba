    .section .crt0, "ax"
    .arm
    .align 2
    .global _start
_start:
    b   rom_entry           @ 0x00: entry branch
    .space 0xBC             @ 0x04-0xBF: header (filled by gbafix.py)

rom_entry:
    @ IRQ stack
    mov r0, #0x12
    msr cpsr_c, r0
    ldr sp, =0x03007FA0
    @ system/user stack
    mov r0, #0x1F
    msr cpsr_c, r0
    ldr sp, =0x03007F00

    @ copy .data ROM -> IWRAM
    ldr r0, =__data_lma
    ldr r1, =__data_start
    ldr r2, =__data_end
1:  cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 1b

    @ zero .bss
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
2:  cmp r1, r2
    strlo r3, [r1], #4
    blo 2b

    ldr r0, =main
    bx  r0

    .global irq_stub
irq_stub:
    @ minimal IRQ handler: ack all pending in IF and BIOS IFBIOS
    ldr r0, =0x04000200      @ REG_IE
    ldr r1, [r0]             @ IE|IF
    and r1, r1, r1, lsr #16
    strh r1, [r0, #2]        @ ack IF
    ldr r0, =0x03007FF8      @ IFBIOS
    ldrh r2, [r0]
    orr r2, r2, r1
    strh r2, [r0]
    bx lr

    .pool
