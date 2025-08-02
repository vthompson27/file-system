// Arquivo: src/uart.c (VERSÃO FINAL PARA PL011 UART)

#include "uart.h"
#include <stdint.h>

// Endereços de memória (MMIO) para os periféricos da Raspberry Pi 2/3
#define PERIPHERAL_BASE   0x3F000000
#define GPIO_BASE         (PERIPHERAL_BASE + 0x200000)
#define UART0_BASE        (PERIPHERAL_BASE + 0x201000) // Endereço da PL011 UART

// Registradores GPIO
#define GPFSEL1           ((volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPPUD             ((volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK0         ((volatile uint32_t*)(GPIO_BASE + 0x98))

// Registradores da PL011 UART
#define UART0_DR          ((volatile uint32_t*)(UART0_BASE + 0x00))
#define UART0_FR          ((volatile uint32_t*)(UART0_BASE + 0x18))
#define UART0_IBRD        ((volatile uint32_t*)(UART0_BASE + 0x24))
#define UART0_FBRD        ((volatile uint32_t*)(UART0_BASE + 0x28))
#define UART0_LCRH        ((volatile uint32_t*)(UART0_BASE + 0x2C))
#define UART0_CR          ((volatile uint32_t*)(UART0_BASE + 0x30))
#define UART0_IMSC        ((volatile uint32_t*)(UART0_BASE + 0x38))
#define UART0_ICR         ((volatile uint32_t*)(UART0_BASE + 0x44))

// Função de delay
void delay(int32_t count) {
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
         : "=r"(count): [count]"0"(count) : "cc");
}

void uart_init() {
    // Desabilita a UART0 temporariamente
    *UART0_CR = 0;

    // Configura os pinos GPIO 14 (TX) e 15 (RX) para a função ALT0 da UART0
    uint32_t selector = *GPFSEL1;
    selector &= ~((7 << 12) | (7 << 15)); // Limpa os bits para GPIO 14 e 15
    selector |= (4 << 12) | (4 << 15);    // Define ALT0 para GPIO 14 e 15
    *GPFSEL1 = selector;

    // Desabilita resistores de pull-up/pull-down
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;

    // Limpa todas as interrupções pendentes
    *UART0_ICR = 0x7FF;

    // Define o baud rate para 115200 (assumindo clock de 48MHz no QEMU)
    // Baudrate Divisor = 48,000,000 / (16 * 115200) = 26.0416...
    // IBRD = 26
    // FBRD = int((0.0416 * 64) + 0.5) = 3
    *UART0_IBRD = 26;
    *UART0_FBRD = 3;

    // Habilita FIFO e define o formato da linha (8 bits de dados)
    *UART0_LCRH = (1 << 4) | (1 << 5) | (1 << 6);

    // Desabilita todas as interrupções
    *UART0_IMSC = 0;

    // Habilita a UART, o transmissor (TXE) e o receptor (RXE)
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(unsigned char c) {
    // Espera até que o transmissor não esteja ocupado (bit 5 do FR - TXFF)
    while (*UART0_FR & (1 << 5));
    *UART0_DR = c;
}

unsigned char uart_getc() {
    // Espera até que o receptor não esteja vazio (bit 4 do FR - RXFE)
    while (*UART0_FR & (1 << 4));
    return *UART0_DR;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}