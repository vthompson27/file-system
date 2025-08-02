.section ".text.boot"

.global _start

_start:
    // Configura o ponteiro de pilha (Stack Pointer)
    ldr sp, =_stack_top

    // Zera a seção .bss
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge call_main
    str r2, [r0], #4
    b clear_bss

call_main:
    // Chama a função principal em C (bl = branch with link)
    bl main

// Se main retornar, entra em loop infinito
hang:
    wfe
    b hang