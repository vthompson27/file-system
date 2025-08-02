#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>

// --- Configurações do Sistema de Arquivos ---
#define BLOCK_SIZE 512
#define MAX_FILENAME_LEN 28
#define NUM_INODES 128
#define NUM_DATA_BLOCKS 8192 // 8192 * 512 bytes = 4MB de disco
#define MAX_DIRECT_POINTERS 12
#define FS_MAGIC 0x5346534A // "SFSJ" em ASCII (Simple File System J)
#define ATTR_FILE         1
#define ATTR_DIRECTORY    2

// --- Estruturas de Dados ---

// Superbloco (sempre no bloco 0)
typedef struct {
    uint32_t magic_number;
    uint32_t total_blocks;
    uint32_t inode_bitmap_start_block;
    uint32_t data_bitmap_start_block;
    uint32_t inode_table_start_block;
    uint32_t data_area_start_block;
    uint32_t root_inode_number;
} Superblock;

// Inode (Nó de Índice)
typedef struct {
    uint8_t type; // 1 para arquivo, 2 para diretório
    uint32_t size;
    uint32_t direct_pointers[MAX_DIRECT_POINTERS];
} Inode;

// Entrada de Diretório
typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint32_t inode_number;
} DirectoryEntry;


// --- API do Sistema de Arquivos ---

// Formata o disco virtual, criando as estruturas iniciais
void fs_format();

// Monta o sistema de arquivos (lê o superbloco e prepara para uso)
void fs_mount();

// Lista os arquivos no diretório atual
void fs_ls();

// Cria um novo diretório
int fs_mkdir(const char* dirname);

// Cria um novo arquivo vazio
int fs_touch(const char* filename);

// Muda o diretório atual
int fs_cd(const char* path);

// Lê e exibe o conteúdo de um arquivo
int fs_cat(const char* filename);

// Escreve/anexa texto a um arquivo
int fs_write(const char* filename, const char* text);

// Deleta um arquivo
int fs_rm(const char* filename);

const char* fs_get_current_path();

#endif // SIMPLEFS_H