#ifndef UART_H
#define UART_H

#include "common.h"

#define LINE_WIDTH 44

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

/**
 * @brief Esta função converte um número inteiro em uma string e a alinha à direita,
 * preenchendo com espaços à esquerda até atingir a largura especificada.
 * A string resultante é enviada para o UART.
 * @param num O número a ser enviado.
 * @param width A largura total da string resultante.
 */
void uart_puts_right_aligned(int num, int width);

/**
 * @brief Envia uma string formatada com dois números inteiros alinhados à direita,
 * seguidos por um sufixo opcional. A função garante que o texto, os números e o sufixo
 * estejam alinhados dentro de uma largura de linha definida.
 * @param text O texto a ser enviado.
 * @param num1 O primeiro número a ser enviado.
 * @param num2 O segundo número a ser enviado (opcional).
 * @param suffix Um sufixo opcional a ser adicionado após os números.
 */
void uart_puts_aligned(const char* text, int num1, int num2, const char* suffix);

#endif // UART_H
