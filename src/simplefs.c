#include "simplefs.h"
#include "common.h"
#include "uart.h"

// --- O "DISCO" VIRTUAL ---
// Este grande array na RAM simula nosso dispositivo de armazenamento.
unsigned char ram_disk[NUM_DATA_BLOCKS * BLOCK_SIZE];

// --- ESTADO GLOBAL DO SISTEMA DE ARQUIVOS ---
static Superblock sb;
static uint32_t *inode_bitmap;
static uint32_t *data_bitmap;
static Inode *inode_table;
static void *data_area;
static uint32_t current_dir_inode_num;
static char current_path_string[256];

// --- FUNÇÕES AUXILIARES DE BAIXO NÍVEL ---

// Lê um bloco do disco para um buffer
static void read_block(uint32_t block_num, void* buffer) {
    memcpy(buffer, &ram_disk[block_num * BLOCK_SIZE], BLOCK_SIZE);
}

// Escreve o conteúdo de um buffer em um bloco do disco
static void write_block(uint32_t block_num, const void* buffer) {
    memcpy(&ram_disk[block_num * BLOCK_SIZE], buffer, BLOCK_SIZE);
}

// Define um bit em um bitmap
static void set_bitmap_bit(uint32_t* bitmap, uint32_t index) {
    bitmap[index / 32] |= (1 << (index % 32));
}

// Limpa (zera) um bit em um bitmap
static void clear_bitmap_bit(uint32_t* bitmap, uint32_t index) {
    bitmap[index / 32] &= ~(1 << (index % 32));
}

// Encontra o primeiro inode livre no bitmap
static int find_free_inode() {
    for (int i = 0; i < NUM_INODES; i++) {
        if (!((inode_bitmap[i / 32] >> (i % 32)) & 1)) {
            return i;
        }
    }
    return -1; // Nenhum inode livre
}

// Encontra o primeiro bloco de dados livre no bitmap
static int find_free_data_block() {
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!((data_bitmap[i / 32] >> (i % 32)) & 1)) {
            return i;
        }
    }
    return -1; // Nenhum bloco de dados livre
}


// --- API DO SISTEMA DE ARQUIVOS ---

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

void fs_ls() {
    Inode current_dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block[BLOCK_SIZE / sizeof(DirectoryEntry)];

    for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
        if (current_dir_inode.direct_pointers[i] != 0) {
            read_block(current_dir_inode.direct_pointers[i], dir_block);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (dir_block[j].filename[0] != '\0') {
                    Inode entry_inode = inode_table[dir_block[j].inode_number];
                    if (entry_inode.type == 2) { // Diretório
                        uart_puts("d ");
                    } else { // Arquivo
                        uart_puts("- ");
                    }
                    uart_puts(dir_block[j].filename);
                    uart_puts("\n");
                }
            }
        }
    }
}

// Função auxiliar para encontrar uma entrada em um diretório
static int find_entry(const char* name, DirectoryEntry* entry) {
    Inode dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block[BLOCK_SIZE / sizeof(DirectoryEntry)];

    for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
        if (dir_inode.direct_pointers[i] != 0) {
            read_block(dir_inode.direct_pointers[i], dir_block);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (dir_block[j].filename[0] != '\0' && strcmp(dir_block[j].filename, name) == 0) {
                    if (entry) *entry = dir_block[j];
                    return dir_block[j].inode_number;
                }
            }
        }
    }
    return -1; // Não encontrado
}


int fs_mkdir(const char* dirname) {
    if (strlen(dirname) >= MAX_FILENAME_LEN) return -1; // Nome muito longo
    if (find_entry(dirname, NULL) != -1) return -2; // Já existe

    // Encontra inode e bloco de dados livres
    int inode_idx = find_free_inode();
    int data_block_idx = find_free_data_block();
    if (inode_idx == -1 || data_block_idx == -1) return -3; // Sem espaço

    // Adiciona a nova entrada no diretório atual
    Inode current_dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block[BLOCK_SIZE / sizeof(DirectoryEntry)];
    int entry_added = 0;
    
    for (int i = 0; i < MAX_DIRECT_POINTERS && !entry_added; i++) {
        if (current_dir_inode.direct_pointers[i] != 0) {
            read_block(current_dir_inode.direct_pointers[i], dir_block);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (dir_block[j].filename[0] == '\0') {
                    strcpy(dir_block[j].filename, dirname);
                    dir_block[j].inode_number = inode_idx;
                    write_block(current_dir_inode.direct_pointers[i], dir_block);
                    entry_added = 1;
                    break;
                }
            }
        }
    }

    if (!entry_added) return -4; // Diretório pai cheio

    // Aloca e configura o novo inode e bloco de dados
    set_bitmap_bit(inode_bitmap, inode_idx);
    set_bitmap_bit(data_bitmap, data_block_idx);

    Inode new_inode;
    new_inode.type = 2; // Diretório
    new_inode.size = 2 * sizeof(DirectoryEntry);
    new_inode.direct_pointers[0] = data_block_idx;
    for (int i = 1; i < MAX_DIRECT_POINTERS; i++) new_inode.direct_pointers[i] = 0;
    inode_table[inode_idx] = new_inode;

    // Cria as entradas "." e ".." no novo diretório
    DirectoryEntry new_dir_entries[BLOCK_SIZE / sizeof(DirectoryEntry)] = {0};
    strcpy(new_dir_entries[0].filename, ".");
    new_dir_entries[0].inode_number = inode_idx;
    strcpy(new_dir_entries[1].filename, "..");
    new_dir_entries[1].inode_number = current_dir_inode_num;
    write_block(data_block_idx, new_dir_entries);
    
    return 0;
}

int fs_touch(const char* filename) {
    if (strlen(filename) >= MAX_FILENAME_LEN) {
        uart_puts("Erro: Nome do arquivo muito longo.\n");
        return -1;
    }
    if (find_entry(filename, NULL) != -1) {
        uart_puts("Erro: Arquivo ou diretorio ja existe.\n");
        return -2;
    }

    // 1. Encontrar um inode livre para o novo arquivo.
    int inode_idx = find_free_inode();
    if (inode_idx == -1) {
        uart_puts("Erro: Sem inodes livres no disco.\n");
        return -3;
    }

    // 2. Encontrar um slot vazio no diretório atual.
    Inode current_dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block[BLOCK_SIZE / sizeof(DirectoryEntry)];
    int entry_added = 0;
    
    for (int i = 0; i < MAX_DIRECT_POINTERS && !entry_added; i++) {
        if (current_dir_inode.direct_pointers[i] != 0) {
            read_block(current_dir_inode.direct_pointers[i], dir_block);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (dir_block[j].filename[0] == '\0') {
                    // Slot vazio encontrado!
                    strcpy(dir_block[j].filename, filename);
                    dir_block[j].inode_number = inode_idx;
                    write_block(current_dir_inode.direct_pointers[i], dir_block);
                    entry_added = 1;
                    break;
                }
            }
        }
    }

    if (!entry_added) {
        uart_puts("Erro: Diretorio atual esta cheio.\n");
        return -4;
    }

    // 3. Alocar e configurar o novo inode.
    set_bitmap_bit(inode_bitmap, inode_idx);
    
    Inode new_inode;
    new_inode.type = 1; // Tipo Arquivo
    new_inode.size = 0; // Tamanho inicial zero
    for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
        new_inode.direct_pointers[i] = 0; // Nenhum bloco de dados alocado
    }
    inode_table[inode_idx] = new_inode;

    uart_puts("Arquivo '");
    uart_puts(filename);
    uart_puts("' criado.\n");
    return 0;
}


int fs_cd(const char* path) {
    if (strcmp(path, "/") == 0) {
        current_dir_inode_num = sb.root_inode_number;
        strcpy(current_path_string, "/");
        return 0;
    }
    
    DirectoryEntry entry;
    int inode_num = find_entry(path, &entry);

    if (inode_num != -1 && inode_table[inode_num].type == ATTR_DIRECTORY) {
        current_dir_inode_num = inode_num;

        // Lógica para atualizar a string do caminho
        if (strcmp(path, "..") == 0) {
            // Se for '..', remove o último componente do caminho
            uint32_t len = strlen(current_path_string);
            if (len > 1) { // Não altera se já for "/"
                for (int i = len - 2; i >= 0; i--) {
                    if (current_path_string[i] == '/') {
                        current_path_string[i+1] = '\0';
                        break;
                    }
                }
            }
        } else {
            // Se for um diretório normal, adiciona ao caminho
            if (strcmp(current_path_string, "/") != 0) {
                strcat(current_path_string, path);
            } else {
                // Evita "//" no início
                strcpy(current_path_string + 1, path);
            }
            strcat(current_path_string, "/");
        }
        return 0;
    }
    
    uart_puts("Diretorio nao encontrado: ");
    uart_puts(path);
    uart_puts("\n");
    return -1;
}

int fs_cat(const char* filename) {
    DirectoryEntry entry;
    int inode_num = find_entry(filename, &entry);

    if (inode_num == -1 || inode_table[inode_num].type != 1) {
        uart_puts("Arquivo nao encontrado.\n");
        return -1;
    }

    Inode file_inode = inode_table[inode_num];
    char buffer[BLOCK_SIZE + 1];
    buffer[BLOCK_SIZE] = '\0';
    uint32_t bytes_to_read = file_inode.size;

    for (int i = 0; i < MAX_DIRECT_POINTERS && bytes_to_read > 0; i++) {
        if (file_inode.direct_pointers[i] != 0) {
            read_block(file_inode.direct_pointers[i], buffer);
            if (bytes_to_read < BLOCK_SIZE) {
                buffer[bytes_to_read] = '\0';
            }
            uart_puts(buffer);
            bytes_to_read -= (bytes_to_read < BLOCK_SIZE ? bytes_to_read : BLOCK_SIZE);
        }
    }
    uart_puts("\n");
    return 0;
}


int fs_write(const char* filename, const char* text) {
    if (strlen(filename) >= MAX_FILENAME_LEN) {
        uart_puts("Erro: Nome do arquivo muito longo.\n");
        return -1;
    }
    if (strlen(text) == 0) {
        return 0; // Nada a escrever
    }

    int inode_num = find_entry(filename, NULL);

    // Se o arquivo não existe, cria ele primeiro
    if (inode_num == -1) {
        if (fs_touch(filename) != 0) {
            uart_puts("Erro: Nao foi possivel criar o arquivo.\n");
            return -1;
        }
        inode_num = find_entry(filename, NULL);
    }
    
    Inode file_inode = inode_table[inode_num];
    if (file_inode.type != 1) {
        uart_puts("Erro: Nao e um arquivo.\n");
        return -1;
    }

    uint32_t text_len = strlen(text);
    const char* text_ptr = text;
    uint32_t bytes_to_write = text_len;
    
    // Posição inicial para escrita (final do arquivo)
    uint32_t offset_in_block = file_inode.size % BLOCK_SIZE;
    uint32_t current_block_ptr_idx = file_inode.size / BLOCK_SIZE;

    // Se houver um bloco parcialmente preenchido, completa ele primeiro
    if (offset_in_block > 0) {
        char buffer[BLOCK_SIZE];
        uint32_t block_to_write_num = file_inode.direct_pointers[current_block_ptr_idx-1];
        read_block(block_to_write_num, buffer);
        
        uint32_t space_in_block = BLOCK_SIZE - offset_in_block;
        uint32_t write_now_len = (bytes_to_write < space_in_block) ? bytes_to_write : space_in_block;
        
        memcpy(buffer + offset_in_block, text_ptr, write_now_len);
        write_block(block_to_write_num, buffer);
        
        text_ptr += write_now_len;
        bytes_to_write -= write_now_len;
        file_inode.size += write_now_len;
    }
    
    // Aloca novos blocos para o restante do texto
    while (bytes_to_write > 0) {
        current_block_ptr_idx = file_inode.size / BLOCK_SIZE;
        if (current_block_ptr_idx >= MAX_DIRECT_POINTERS) {
            uart_puts("Erro: Arquivo atingiu o tamanho maximo.\n");
            break; 
        }

        int new_block_idx = find_free_data_block();
        if (new_block_idx == -1) {
            uart_puts("Erro: Disco cheio.\n");
            break;
        }
        
        set_bitmap_bit(data_bitmap, new_block_idx);
        file_inode.direct_pointers[current_block_ptr_idx] = new_block_idx;

        char buffer[BLOCK_SIZE] = {0};
        uint32_t write_now_len = (bytes_to_write < BLOCK_SIZE) ? bytes_to_write : BLOCK_SIZE;
        memcpy(buffer, text_ptr, write_now_len);
        write_block(new_block_idx, buffer);

        text_ptr += write_now_len;
        bytes_to_write -= write_now_len;
        file_inode.size += write_now_len;
    }
    
    // Salva o inode atualizado
    inode_table[inode_num] = file_inode;
    
    uart_puts("Texto anexado ao arquivo '");
    uart_puts(filename);
    uart_puts("'.\n");

    return 0;
}

int fs_rm(const char* filename) {
    // Proibir a exclusão de "." e ".."
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        uart_puts("Erro: Nao e possivel deletar '.' ou '..'.\n");
        return -1;
    }

    uint32_t entry_block_num; // Bloco onde a entrada de diretório está
    uint32_t entry_index;     // Índice da entrada dentro do bloco

    // 1. Encontrar a entrada de diretório para obter o número do inode
    DirectoryEntry entry;
    int inode_num = -1;
    int found = 0;
    Inode dir_inode = inode_table[current_dir_inode_num];
    DirectoryEntry dir_block_buffer[BLOCK_SIZE / sizeof(DirectoryEntry)];

    for (int i = 0; i < MAX_DIRECT_POINTERS && !found; i++) {
        if (dir_inode.direct_pointers[i] != 0) {
            read_block(dir_inode.direct_pointers[i], dir_block_buffer);
            for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                if (dir_block_buffer[j].filename[0] != '\0' && strcmp(dir_block_buffer[j].filename, filename) == 0) {
                    entry = dir_block_buffer[j];
                    inode_num = entry.inode_number;
                    entry_block_num = dir_inode.direct_pointers[i];
                    entry_index = j;
                    found = 1;
                    break;
                }
            }
        }
    }

    if (!found) {
        uart_puts("Erro: Arquivo ou diretorio nao encontrado.\n");
        return -1;
    }

    // 2. Ler o inode do item a ser deletado
    Inode target_inode = inode_table[inode_num];

    // 3. Lógica de deleção baseada no tipo (arquivo ou diretório)
    if (target_inode.type == ATTR_DIRECTORY) {
        // Lógica para deletar um diretório
        int entry_count = 0;
        DirectoryEntry content_buffer[BLOCK_SIZE / sizeof(DirectoryEntry)];
        for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
            if (target_inode.direct_pointers[i] != 0) {
                read_block(target_inode.direct_pointers[i], content_buffer);
                for (uint32_t j = 0; j < (BLOCK_SIZE / sizeof(DirectoryEntry)); j++) {
                    if (content_buffer[j].filename[0] != '\0') {
                        entry_count++;
                    }
                }
            }
        }
        
        // Um diretório vazio tem exatamente 2 entradas: "." e ".."
        if (entry_count > 2) {
            uart_puts("Erro: O diretorio nao esta vazio.\n");
            return -3;
        }
    }

    // 4. Se for um arquivo ou um diretório vazio, a lógica de liberação é a mesma:
    // Liberar os blocos de dados no bitmap de dados
    for (int i = 0; i < MAX_DIRECT_POINTERS; i++) {
        if (target_inode.direct_pointers[i] != 0) {
            clear_bitmap_bit(data_bitmap, target_inode.direct_pointers[i]);
        }
    }

    // Liberar o inode no bitmap de inodes
    clear_bitmap_bit(inode_bitmap, inode_num);

    // 5. Apagar a entrada no diretório pai
    read_block(entry_block_num, dir_block_buffer);
    dir_block_buffer[entry_index].filename[0] = '\0'; // Marca como vazia
    write_block(entry_block_num, dir_block_buffer);

    uart_puts("Item '");
    uart_puts(filename);
    uart_puts("' deletado.\n");

    return 0;
}

const char* fs_get_current_path() {
    return current_path_string;
}

void fs_stat() {
    uint32_t used_inodes = 0;
    for (int i = 0; i < NUM_INODES; i++) {
        if ((inode_bitmap[i / 32] >> (i % 32)) & 1) {
            used_inodes++;
        }
    }

    uint32_t used_data_blocks = 0;
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if ((data_bitmap[i / 32] >> (i % 32)) & 1) {
            used_data_blocks++;
        }
    }
    
    // Calcula o espaço útil (descontando os metadados)
    uint32_t metadata_blocks = sb.data_area_start_block;
    uint32_t user_blocks_used = used_data_blocks - metadata_blocks;
    uint32_t total_user_blocks = NUM_DATA_BLOCKS - metadata_blocks;
    
    char buffer[32];

    uart_puts("--- Estatisticas do Sistema de Arquivos ---\n");
    
    // Estatísticas de Inodes
    itoa(used_inodes, buffer);
    uart_puts("Inodes utilizados: ");
    uart_puts(buffer);
    uart_puts(" / ");
    itoa(NUM_INODES, buffer);
    uart_puts(buffer);
    uart_puts("\n");

    // Estatísticas de Blocos de Dados
    itoa(user_blocks_used, buffer);
    uart_puts("Blocos de dados utilizados: ");
    uart_puts(buffer);
    uart_puts(" / ");
    itoa(total_user_blocks, buffer);
    uart_puts(buffer);
    uart_puts("\n");

    // Estatísticas de Espaço em Bytes
    itoa(user_blocks_used * BLOCK_SIZE, buffer);
    uart_puts("Espaco utilizado: ");
    uart_puts(buffer);
    uart_puts(" Bytes\n");
    
    itoa(total_user_blocks * BLOCK_SIZE, buffer);
    uart_puts("Espaco total (util): ");
    uart_puts(buffer);
    uart_puts(" Bytes\n");

    uart_puts("-------------------------------------------\n");
}