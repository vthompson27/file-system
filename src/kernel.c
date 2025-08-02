#include "uart.h"
#include "shell.h"
#include "simplefs.h"

void main() {
    // Inicializa a comunicação serial
    uart_init();
    uart_puts("\n===== SimpleFS Bare-Metal no Raspberry Pi 3 =====\n");

    // Formata e monta o nosso sistema de arquivos na RAM
    fs_format();
    fs_mount();
    uart_puts("Sistema de arquivos formatado e montado.\n");
    uart_puts("Digite 'help' para ver os comandos.\n");

    // Inicia o shell para interação com o usuário
    shell_start();
}