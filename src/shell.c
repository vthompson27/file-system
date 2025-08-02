// Arquivo: src/shell.c (versão corrigida)

#include "shell.h"
#include "uart.h"
#include "simplefs.h"
#include "common.h"

#define CMD_BUFFER_SIZE 128
#define MAX_ARGS 16 // Aumentado para permitir frases mais longas

// Lê uma linha de comando da UART
static void read_command(char *buffer) {
    int i = 0;
    while (i < CMD_BUFFER_SIZE - 1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_puts("\n");
            break;
        } else if ((c == 127 || c == 8) && i > 0) { // Backspace
            i--;
            uart_puts("\b \b"); // Apaga o caractere no terminal
        } else if (c >= ' ' && c <= '~') {
            buffer[i++] = c;
            uart_putc(c); // Echo do caractere
        }
    }
    buffer[i] = '\0';
}

// Divide o comando em argumentos (argc/argv)
static int parse_command(char *buffer, char **argv) {
    int argc = 0;
    char *p = buffer;
    while (*p && argc < MAX_ARGS) {
        while (*p == ' ') p++; // Pula espaços
        if (*p == '\0') break;

        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}

void shell_start() {
    char cmd_buffer[CMD_BUFFER_SIZE];
    char* argv[MAX_ARGS];
    int argc;

    while (1) {
        uart_puts("SimpleFS:");
        uart_puts(fs_get_current_path());
        uart_puts("$ ");
        read_command(cmd_buffer);
        argc = parse_command(cmd_buffer, argv);

        if (argc == 0) continue;

        if (strcmp(argv[0], "help") == 0) {
            uart_puts("Comandos disponiveis:\n");
            uart_puts("  ls        - Lista arquivos e diretorios\n");
            uart_puts("  mkdir <n> - Cria um novo diretorio\n");
            uart_puts("  touch <n> - Cria um novo arquivo vazio\n");
            uart_puts("  cat <n>   - Mostra o conteudo de um arquivo\n");
            uart_puts("  write <f> <t> - Escreve/anexa texto <t> ao arquivo <f>\n");
            uart_puts("  cd <n>    - Muda de diretorio (use '..' para voltar)\n");
            uart_puts("  rm <n>    - Deleta um arquivo ou diretorio vazio\n");
            uart_puts("  stat      - Mostra estatisticas de uso do disco\n");
            uart_puts("  format    - Re-formata o sistema de arquivos\n");
        } else if (strcmp(argv[0], "ls") == 0) {
            fs_ls();
        } else if (strcmp(argv[0], "mkdir") == 0) {
            if (argc > 1) fs_mkdir(argv[1]);
            else uart_puts("Uso: mkdir <nome_do_diretorio>\n");
        } else if (strcmp(argv[0], "touch") == 0) {
            if (argc > 1) fs_touch(argv[1]);
            else uart_puts("Uso: touch <nome_do_arquivo>\n");
        } else if (strcmp(argv[0], "cat") == 0) {
            if (argc > 1) fs_cat(argv[1]);
            else uart_puts("Uso: cat <nome_do_arquivo>\n");
        } else if (strcmp(argv[0], "write") == 0) {
            if (argc > 2) {
                // --- INÍCIO DA CORREÇÃO ---
                // O texto a ser escrito começa em argv[2].
                // O parse_command trocou todos os espaços por '\0'.
                // Vamos percorrer os argumentos e trocar os '\0' de volta para espaços,
                // efetivamente "juntando" a frase.
                for (int i = 2; i < argc - 1; i++) {
                    // Encontra o final da palavra atual (que é um '\0')
                    char* end_of_word = argv[i] + strlen(argv[i]);
                    // Substitui o '\0' por um espaço, conectando com a próxima palavra.
                    *end_of_word = ' ';
                }
                // Agora, argv[2] aponta para o início da string completa.
                fs_write(argv[1], argv[2]);
                // --- FIM DA CORREÇÃO ---
            } else {
                uart_puts("Uso: write <arquivo> <texto>\n");
            }
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argc > 1) fs_cd(argv[1]);
            else uart_puts("Uso: cd <nome_do_diretorio>\n");
        } else if (strcmp(argv[0], "rm") == 0) {
            if (argc > 1) fs_rm(argv[1]);
            else uart_puts("Uso: rm <nome_do_arquivo>\n");
        } else if (strcmp(argv[0], "stat") == 0) {
            fs_stat();
        }else if (strcmp(argv[0], "format") == 0) {
            uart_puts("Formatando o sistema de arquivos...\n");
            fs_format();
            fs_mount();
            uart_puts("Pronto.\n");
        } else {
            uart_puts("Comando desconhecido: ");
            uart_puts(argv[0]);
            uart_puts("\n");
        }
    }
}