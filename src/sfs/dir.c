#include "sfs.h"
#include "common.h"
#include "uart.h"
#include "fs_defs.h"

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

  uint32_t metadata_blocks = sb.data_area_start_block;
  uint32_t user_blocks_used = used_data_blocks - metadata_blocks;
  uint32_t total_user_blocks = NUM_DATA_BLOCKS - metadata_blocks;

char buf[32], buf2[32];

uart_puts("--- Estatisticas do Sistema de Arquivos ---\n");

itoa(used_inodes, buf);
itoa(NUM_INODES, buf2);
uart_puts(" Inodes utilizados      ");
int len_spaces = LINE_WIDTH - strlen(" Inodes utilizados      ") - strlen(buf) - strlen(buf2) - 3;
for (int i = 0; i < len_spaces; i++) uart_puts(" ");
uart_puts(buf);
uart_puts(" / ");
uart_puts(buf2);
uart_puts("\n");

itoa(user_blocks_used, buf);
itoa(total_user_blocks, buf2);
uart_puts(" Blocos de dados usados ");
len_spaces = LINE_WIDTH - strlen(" Blocos de dados usados ") - strlen(buf) - strlen(buf2) - 3;
for (int i = 0; i < len_spaces; i++) uart_puts(" ");
uart_puts(buf);
uart_puts(" / ");
uart_puts(buf2);
uart_puts("\n");

itoa(user_blocks_used * BLOCK_SIZE, buf);
uart_puts(" Espaço utilizado       ");
len_spaces = LINE_WIDTH - strlen(" Espaço utilizado       ") - strlen(buf) - strlen(" Bytes");
for (int i = 0; i < len_spaces; i++) uart_puts(" ");
uart_puts(buf);
uart_puts(" Bytes\n");

itoa(total_user_blocks * BLOCK_SIZE, buf);
uart_puts(" Espaço total (util)    ");
len_spaces = LINE_WIDTH - strlen(" Espaço total (util)    ") - strlen(buf) - strlen(" Bytes");
for (int i = 0; i < len_spaces; i++) uart_puts(" ");
uart_puts(buf);
uart_puts(" Bytes\n");

uart_puts("-------------------------------------------\n");
}
