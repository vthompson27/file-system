/* Arquivo: linker.ld (versão para 32-bit) */
SECTIONS
{
    /* O código começa no endereço 0x8000 para 32-bit */
    . = 0x8000;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    /* ... o resto do arquivo pode continuar igual ... */
    .rodata : {
        *(.rodata .rodata.*)
    }
    
    .data : {
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x1000;
    _stack_top = .;
}