#include "sfs.h"
#include "common.h"
#include "uart.h"
#include "fs_defs.h"

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
