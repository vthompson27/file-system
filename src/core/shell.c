#include "shell.h"
#include "uart.h"
#include "common.h"
#include "sfs.h"

#define CMD_BUFFER_SIZE 128
#define MAX_ARGS 16

static void read_command(char *buffer) {
    int i = 0;
    while (i < CMD_BUFFER_SIZE - 1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_puts("\n");
            break;
        } else if ((c == 127 || c == 8) && i > 0) {
            i--;
            uart_puts("\b \b");
        } else if (c >= ' ' && c <= '~') {
            buffer[i++] = c;
            uart_putc(c);
        }
    }
    buffer[i] = '\0';
}

static int parse_command(char *buffer, char **argv) {
    int argc = 0;
    char *p = buffer;
    while (*p && argc < MAX_ARGS) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p == ' ') *p++ = '\0';
    }
    return argc;
}

static enum {
    CMD_UNKNOWN,
    CMD_HELP,
    CMD_LS,
    CMD_MKDIR,
    CMD_TOUCH,
    CMD_CAT,
    CMD_WRITE,
    CMD_CD,
    CMD_RM,
    CMD_FORMAT,
    CMD_STAT
} resolve_command(const char *cmd) {
    if (strcmp(cmd, "help") == 0) return CMD_HELP;
    if (strcmp(cmd, "ls") == 0) return CMD_LS;
    if (strcmp(cmd, "mkdir") == 0) return CMD_MKDIR;
    if (strcmp(cmd, "touch") == 0) return CMD_TOUCH;
    if (strcmp(cmd, "cat") == 0) return CMD_CAT;
    if (strcmp(cmd, "write") == 0) return CMD_WRITE;
    if (strcmp(cmd, "cd") == 0) return CMD_CD;
    if (strcmp(cmd, "rm") == 0) return CMD_RM;
    if (strcmp(cmd, "format") == 0) return CMD_FORMAT;
    if (strcmp(cmd, "stat") == 0) return CMD_STAT;
    return CMD_UNKNOWN;
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

        switch (resolve_command(argv[0])) {
            case CMD_HELP:
                uart_puts("Comandos disponiveis:\n");
                uart_puts("  ls             - Lista arquivos e diretorios\n");
                uart_puts("  mkdir <n>      - Cria um novo diretorio\n");
                uart_puts("  touch <n>      - Cria um novo arquivo vazio\n");
                uart_puts("  cat <n>        - Mostra o conteudo de um arquivo\n");
                uart_puts("  write <f> <t>  - Escreve/anexa texto <t> ao arquivo <f>\n");
                uart_puts("  cd <n>         - Muda de diretorio (use '..' para voltar)\n");
                uart_puts("  rm <n>         - Deleta um arquivo ou diretorio vazio\n");
                uart_puts("  stat           - Mostra estatisticas de uso do disco\n");
                uart_puts("  format         - Re-formata o sistema de arquivos\n");
                break;
            case CMD_LS:
                fs_ls();
                break;
            case CMD_MKDIR:
                argc > 1 ? fs_mkdir(argv[1]) : uart_puts("Uso: mkdir <nome>\n");
                break;
            case CMD_TOUCH:
                argc > 1 ? fs_touch(argv[1]) : uart_puts("Uso: touch <nome>\n");
                break;
            case CMD_CAT:
                argc > 1 ? fs_cat(argv[1]) : uart_puts("Uso: cat <arquivo>\n");
                break;
            case CMD_WRITE:
                if (argc > 2) {
                    for (int i = 2; i < argc - 1; i++) {
                        char* end = argv[i] + strlen(argv[i]);
                        *end = ' ';
                    }
                    fs_write(argv[1], argv[2]);
                } else {
                    uart_puts("Uso: write <arquivo> <texto>\n");
                }
                break;
            case CMD_CD:
                argc > 1 ? fs_cd(argv[1]) : uart_puts("Uso: cd <diretorio>\n");
                break;
            case CMD_RM:
                argc > 1 ? fs_rm(argv[1]) : uart_puts("Uso: rm <nome>\n");
                break;
            case CMD_FORMAT:
                uart_puts("Formatando...\n");
                fs_format();
                fs_mount();
                uart_puts("Pronto.\n");
                break;
            case CMD_STAT:
                fs_stat();
                break;
            case CMD_UNKNOWN:
            default:
                uart_puts("Comando desconhecido: ");
                uart_puts(argv[0]);
                uart_puts("\nUse 'help' para ver a lista de comandos.\n");
                break;
        }
    }
}
