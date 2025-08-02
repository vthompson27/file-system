#include "uart.h"
#include "shell.h"
#include "sfs.h"

void main() {
    uart_init();
    uart_puts("\n===== SimpleFS Bare-Metal no Raspberry Pi 3 =====\n");
\
    fs_format();
    fs_mount();
    uart_puts("Sistema de arquivos formatado e montado.\n");
    uart_puts("Digite 'help' para ver os comandos.\n");
\
    shell_start();
}
