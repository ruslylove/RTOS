.syntax unified
.cpu cortex-m33
.thumb

/* FreeRTOS port handlers — FreeRTOSConfig.h maps port names to these CMSIS names */
.extern SVC_Handler
.extern PendSV_Handler
.extern SysTick_Handler

    .section .isr_vector, "a", %progbits
    .align 2
    /* Cortex-M33 required vectors */
    .word   _stack_top          /* 0: Initial stack pointer */
    .word   Reset_Handler       /* 1: Reset */
    .word   NMI_Handler         /* 2: NMI */
    .word   HardFault_Handler   /* 3: HardFault */
    .word   MemManage_Handler   /* 4: MemManage */
    .word   BusFault_Handler    /* 5: BusFault */
    .word   UsageFault_Handler  /* 6: UsageFault */
    .word   SecureFault_Handler /* 7: SecureFault */
    .word   0                   /* 8: Reserved */
    .word   0                   /* 9: Reserved */
    .word   0                   /* 10: Reserved */
    .word   SVC_Handler         /* 11: SVCall — FreeRTOS vPortSVCHandler */
    .word   DebugMon_Handler    /* 12: Debug Monitor */
    .word   0                   /* 13: Reserved */
    .word   PendSV_Handler      /* 14: PendSV — FreeRTOS xPortPendSVHandler */
    .word   SysTick_Handler     /* 15: SysTick — FreeRTOS xPortSysTickHandler */
    /* External interrupts (IRQ0 .. ) — fill as needed */
    .rept   240
    .word   Default_IRQ_Handler
    .endr

    .section .text.Reset_Handler, "ax", %progbits
    .global Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    /* Copy .data from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
1:  cmp r1, r2
    bhs 2f
    ldr r3, [r0], #4
    str r3, [r1], #4
    b   1b

    /* Zero .bss */
2:  ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
3:  cmp r0, r1
    bhs 4f
    str r2, [r0], #4
    b   3b

4:  bl main
    b  .
    .size Reset_Handler, .-Reset_Handler

    .section .text.Default_Handler, "ax", %progbits
    .global Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b .
    .size Default_Handler, .-Default_Handler

    .global Default_IRQ_Handler
    .type   Default_IRQ_Handler, %function
Default_IRQ_Handler:
    b .
    .size Default_IRQ_Handler, .-Default_IRQ_Handler

    /* Weak aliases for fault/system handlers */
    .weak NMI_Handler
    .set  NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .set  HardFault_Handler, Default_Handler
    .weak MemManage_Handler
    .set  MemManage_Handler, Default_Handler
    .weak BusFault_Handler
    .set  BusFault_Handler, Default_Handler
    .weak UsageFault_Handler
    .set  UsageFault_Handler, Default_Handler
    .weak SecureFault_Handler
    .set  SecureFault_Handler, Default_Handler
    .weak DebugMon_Handler
    .set  DebugMon_Handler, Default_Handler
