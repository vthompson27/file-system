#ifndef SHELL_H
#define SHELL_H

/**
 * @brief Inicia o loop principal do shell.
 * * Esta função exibe um prompt, lê comandos do usuário via UART,
 * faz o parse desses comandos e chama as funções correspondentes
 * do sistema de arquivos.
 */
void shell_start();

#endif