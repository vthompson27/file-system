#ifndef UART_H
#define UART_H

/**
 * @brief Inicializa a Mini UART na Raspberry Pi 3.
 * * Configura os pinos GPIO 14 e 15 para suas funções alternativas
 * de UART (TX e RX) e ajusta os registradores da Mini UART para
 * permitir a comunicação serial a 115200 baud.
 */
void uart_init();

/**
 * @brief Envia um único caractere pela UART.
 * @param c O caractere a ser enviado.
 */
void uart_putc(unsigned char c);

/**
 * @brief Lê um único caractere da UART.
 * * Esta é uma função bloqueante; ela espera até que um
 * caractere seja recebido.
 * @return O caractere lido.
 */
unsigned char uart_getc();

/**
 * @brief Envia uma string (terminada em nulo) pela UART.
 * @param s A string a ser enviada.
 */
void uart_puts(const char *s);

#endif // UART_H