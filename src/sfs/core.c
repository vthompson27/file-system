#include "sfs.h"
#include "common.h"
#include "uart.h"
#include "fs_defs.h"

// --- O "DISCO" VIRTUAL ---
unsigned char ram_disk[NUM_DATA_BLOCKS * BLOCK_SIZE];

// --- ESTADO GLOBAL DO SISTEMA DE ARQUIVOS ---
Superblock sb;
uint32_t *inode_bitmap;
uint32_t *data_bitmap;
Inode *inode_table;
void *data_area;
uint32_t current_dir_inode_num;
char current_path_string[256];

// --- FUNÇÕES AUXILIARES DE BAIXO NÍVEL ---

// Lê um bloco do disco para um buffer
void read_block(uint32_t block_num, void* buffer) {
    memcpy(buffer, &ram_disk[block_num * BLOCK_SIZE], BLOCK_SIZE);
}

// Escreve o conteúdo de um buffer em um bloco do disco
void write_block(uint32_t block_num, const void* buffer) {
    memcpy(&ram_disk[block_num * BLOCK_SIZE], buffer, BLOCK_SIZE);
}

// Define um bit em um bitmap
void set_bitmap_bit(uint32_t* bitmap, uint32_t index) {
    bitmap[index / 32] |= (1 << (index % 32));
}

// Limpa (zera) um bit em um bitmap
void clear_bitmap_bit(uint32_t* bitmap, uint32_t index) {
    bitmap[index / 32] &= ~(1 << (index % 32));
}

// Encontra o primeiro inode livre no bitmap
int find_free_inode() {
    for (int i = 0; i < NUM_INODES; i++) {
        if (!((inode_bitmap[i / 32] >> (i % 32)) & 1)) {
            return i;
        }
    }
    return -1; // Nenhum inode livre
}

// Encontra o primeiro bloco de dados livre no bitmap
int find_free_data_block() {
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!((data_bitmap[i / 32] >> (i % 32)) & 1)) {
            return i;
        }
    }
    return -1; // Nenhum bloco de dados livre
}

// --- FUNÇÕES AUXILIARES DO DISCO VIRTUAL ---

void fs_format() {
    // 1. Configurar o superbloco
    sb.magic_number = FS_MAGIC;
    sb.total_blocks = NUM_DATA_BLOCKS;
    sb.inode_bitmap_start_block = 1;
    sb.data_bitmap_start_block = 2;
    sb.inode_table_start_block = 3;
    sb.root_inode_number = 0;

    // Calcula o início da área de dados
    uint32_t inode_table_blocks = (NUM_INODES * sizeof(Inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sb.data_area_start_block = sb.inode_table_start_block + inode_table_blocks;

    // Escreve o superbloco
    write_block(0, &sb);

    // 2. Limpa os bitmaps e a tabela de inodes
    char empty_block[BLOCK_SIZE] = {0};
    write_block(sb.inode_bitmap_start_block, empty_block);
    write_block(sb.data_bitmap_start_block, empty_block);
    for (uint32_t i = 0; i < inode_table_blocks; i++) {
        write_block(sb.inode_table_start_block + i, empty_block);
    }

    // Monta para ter os ponteiros corretos
    fs_mount();

    // 3. Reservar blocos para metadados
    for(uint32_t i = 0; i < sb.data_area_start_block; i++) {
        set_bitmap_bit(data_bitmap, i);
    }

    // 4. Criar o diretório raiz
    int root_inode_idx = find_free_inode(); // Deve ser 0
    set_bitmap_bit(inode_bitmap, root_inode_idx);

    int root_data_block_idx = find_free_data_block();
    set_bitmap_bit(data_bitmap, root_data_block_idx);

    // Configura o inode raiz
    Inode root_inode;
    root_inode.type = 2; // Diretório
    root_inode.size = 2 * sizeof(DirectoryEntry);
    root_inode.direct_pointers[0] = root_data_block_idx;
    for(int i = 1; i < MAX_DIRECT_POINTERS; i++) root_inode.direct_pointers[i] = 0;

    // Cria as entradas "." e ".."
    DirectoryEntry root_entries[2];
    strcpy(root_entries[0].filename, ".");
    root_entries[0].inode_number = root_inode_idx;
    strcpy(root_entries[1].filename, "..");
    root_entries[1].inode_number = root_inode_idx;

    // Escreve tudo no disco
    inode_table[root_inode_idx] = root_inode;
    write_block(root_data_block_idx, root_entries);
}

void fs_mount() {
    // Lê o superbloco do disco
    read_block(0, &sb);
    if (sb.magic_number != FS_MAGIC) {
        uart_puts("ERRO: Magic number invalido! Formatando o disco...\n");
        fs_format();
        read_block(0, &sb);
    }

    // Configura os ponteiros para as áreas de metadados na RAM
    inode_bitmap = (uint32_t*)&ram_disk[sb.inode_bitmap_start_block * BLOCK_SIZE];
    data_bitmap = (uint32_t*)&ram_disk[sb.data_bitmap_start_block * BLOCK_SIZE];
    inode_table = (Inode*)&ram_disk[sb.inode_table_start_block * BLOCK_SIZE];
    data_area = &ram_disk[sb.data_area_start_block * BLOCK_SIZE];

    current_dir_inode_num = sb.root_inode_number;
    strcpy(current_path_string, "/");
}

int find_entry(const char* name, DirectoryEntry* entry) {
    Inode current_dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block[BLOCK_SIZE / sizeof(DirectoryEntry)];

    for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
        if (current_dir_inode.direct_pointers[i] != 0) {
            read_block(current_dir_inode.direct_pointers[i], dir_block);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (strcmp(dir_block[j].filename, name) == 0) {
                    *entry = dir_block[j];
                    return dir_block[j].inode_number;
                }
            }
        }
    }
    return -1; // Entrada não encontrada
}

const char* fs_get_current_path() {
    return current_path_string;
}
