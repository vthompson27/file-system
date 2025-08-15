#include "uart.h"
#include <stdint.h>

// Endereços de memória (MMIO) para os periféricos da Raspberry Pi 2/3
#define PERIPHERAL_BASE   0x3F000000
#define GPIO_BASE         (PERIPHERAL_BASE + 0x200000)
#define AUX_BASE          (PERIPHERAL_BASE + 0x215000)

// Registradores GPIO
#define GPFSEL1           ((volatile uint32_t*)(GPIO_BASE + 0x04))
#define GPPUD             ((volatile uint32_t*)(GPIO_BASE + 0x94))
#define GPPUDCLK0         ((volatile uint32_t*)(GPIO_BASE + 0x98))

// Registradores Auxiliares e Mini UART
#define AUX_ENABLES       ((volatile uint32_t*)(AUX_BASE + 0x04))
#define AUX_MU_IO_REG     ((volatile uint32_t*)(AUX_BASE + 0x40))
#define AUX_MU_IER_REG    ((volatile uint32_t*)(AUX_BASE + 0x44))
#define AUX_MU_IIR_REG    ((volatile uint32_t*)(AUX_BASE + 0x48))
#define AUX_MU_LCR_REG    ((volatile uint32_t*)(AUX_BASE + 0x4C))
#define AUX_MU_MCR_REG    ((volatile uint32_t*)(AUX_BASE + 0x50))
#define AUX_MU_LSR_REG    ((volatile uint32_t*)(AUX_BASE + 0x54))
#define AUX_MU_CNTL_REG   ((volatile uint32_t*)(AUX_BASE + 0x60))
#define AUX_MU_BAUD_REG   ((volatile uint32_t*)(AUX_BASE + 0x68))

// Função para criar um atraso (delay) simples
void delay(int32_t count) {
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
         : "=r"(count): [count]"0"(count) : "cc");
}

void uart_init() {
    uint32_t selector;

    // Configura os pinos GPIO 14 e 15 para a função ALT5 da Mini UART
    selector = *GPFSEL1;
    selector &= ~((7 << 12) | (7 << 15)); // Limpa os bits para GPIO 14 e 15
    selector |= (2 << 12) | (2 << 15);    // Define ALT5 para GPIO 14 e 15
    *GPFSEL1 = selector;

    // Desabilita resistores de pull-up/pull-down para os pinos 14 e 15
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;

    // Inicializa a Mini UART
    *AUX_ENABLES = 1;      // Habilita a Mini UART (bit 0)
    *AUX_MU_CNTL_REG = 0;  // Desabilita transmissor e receptor durante a configuração
    *AUX_MU_IER_REG = 0;   // Desabilita interrupções
    *AUX_MU_LCR_REG = 3;   // Define modo de 8 bits
    *AUX_MU_MCR_REG = 0;   // Define RTS para nível alto
    *AUX_MU_IIR_REG = 6;   // Limpa as filas (FIFOs)

    // Define o baud rate para 115200
    // Baudrate = system_clock_freq / (8 * (baud_reg + 1))
    // Com system_clock_freq = 250MHz (padrão), baud_reg = 270
    *AUX_MU_BAUD_REG = 270;

    // Habilita o transmissor e o receptor
    *AUX_MU_CNTL_REG = 3;
}

void uart_putc(unsigned char c) {
    // Espera até que o transmissor esteja pronto (bit 5 do LSR)
    while (!(*AUX_MU_LSR_REG & 0x20));
    *AUX_MU_IO_REG = c;
}

unsigned char uart_getc() {
    // Espera até que haja dados para ler (bit 0 do LSR)
    while (!(*AUX_MU_LSR_REG & 0x01));
    return *AUX_MU_IO_REG & 0xFF;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_puts_right_aligned(int num, int width) {
    char buf[16];
    itoa(num, buf);
    int len = strlen(buf);
    for (int i = 0; i < width - len; i++) {
        uart_puts(" ");
    }
    uart_puts(buf);
}

void uart_puts_aligned(const char* text, int num1, int num2, const char* suffix) {
    char buf1[16], buf2[16];
    itoa(num1, buf1);
    itoa(num2, buf2);

    int text_len = strlen(text);
    int num1_len = strlen(buf1);
    int num2_len = strlen(buf2);
    int suffix_len = suffix ? strlen(suffix) : 0;

    // Espaços entre texto e números:
    // total = LINE_WIDTH - text_len - num1_len - (3 + num2_len + suffix_len)
    // 3 = espaço + '/' + espaço
    int spaces = LINE_WIDTH - text_len - num1_len - num2_len - suffix_len - 3;

    if (spaces < 1) spaces = 1;

    uart_puts(text);
    for (int i = 0; i < spaces; i++) uart_puts(" ");
    uart_puts(buf1);
    if (num2 >= 0) {
        uart_puts(" / ");
        uart_puts(buf2);
    }
    if (suffix) uart_puts(suffix);
    uart_puts("\n");
}
