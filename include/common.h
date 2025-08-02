#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "uart.h"

// Definição da macro NULL para nosso ambiente bare-metal
#define NULL ((void*)0)

/**
 * @brief Calcula o comprimento de uma string.
 * @param str A string terminada em nulo.
 * @return O número de caracteres na string.
 */
uint32_t strlen(const char *str);

/**
 * @brief Compara duas strings.
 * @param s1 A primeira string.
 * @param s2 A segunda string.
 * @return 0 se as strings forem iguais, outro valor caso contrário.
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief Copia uma string de uma fonte para um destino.
 * @param dest O buffer de destino.
 * @param src A string de origem.
 * @return Um ponteiro para o destino.
 */
char* strcpy(char *dest, const char *src);

/**
 * @brief Copia um bloco de memória.
 * @param dest O ponteiro de destino.
 * @param src O ponteiro de origem.
 * @param n O número de bytes a serem copiados.
 */
void* memcpy(void *dest, const void *src, uint32_t n);


/**
 * @brief Preenche um bloco de memória com um valor.
 * @param s O ponteiro para o bloco de memória.
 * @param c O valor a ser definido (convertido para unsigned char).
 * @param n O número de bytes a serem definidos.
 */
void* memset(void *s, int c, uint32_t n);

/**
 * @brief Concatena a string de origem ao final da string de destino.
 * @param dest A string de destino.
 * @param src A string de origem.
 * @return Um ponteiro para a string de destino.
 */
char* strcat(char *dest, const char *src);

/**
 * @brief Converte um inteiro para uma string.
 * @param n O número a ser convertido.
 * @param buffer O buffer onde a string será armazenada.
 */
void itoa(int n, char* buffer);

#endif // COMMON_H
