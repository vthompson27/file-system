#ifndef FS_DEFS_H
#define FS_DEFS_H

#include <stdint.h>

// --- Configurações ---
#define BLOCK_SIZE 512
#define MAX_FILENAME_LEN 28
#define NUM_INODES 128
#define NUM_DATA_BLOCKS 8192  // 8192 * 512 bytes = 4MB
#define MAX_DIRECT_POINTERS 12
#define FS_MAGIC 0x5346534A    // "SFSJ" em ASCII
#define ATTR_FILE 1
#define ATTR_DIRECTORY 2

// --- Estruturas ---

typedef struct {
    uint32_t magic_number;
    uint32_t total_blocks;
    uint32_t inode_bitmap_start_block;
    uint32_t data_bitmap_start_block;
    uint32_t inode_table_start_block;
    uint32_t data_area_start_block;
    uint32_t root_inode_number;
} Superblock;

typedef struct {
    uint8_t type;  // 1 = arquivo, 2 = diretório
    uint32_t size;
    uint32_t direct_pointers[MAX_DIRECT_POINTERS];
} Inode;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint32_t inode_number;
} DirectoryEntry;

extern Superblock sb;
extern uint32_t *inode_bitmap;
extern uint32_t *data_bitmap;
extern Inode *inode_table;
extern void *data_area;
extern uint32_t current_dir_inode_num;
extern char current_path_string[256];

void read_block(uint32_t block_num, void* buffer);
void write_block(uint32_t block_num, const void* buffer);
void set_bitmap_bit(uint32_t* bitmap, uint32_t index);
void clear_bitmap_bit(uint32_t* bitmap, uint32_t index);
int find_free_inode();
int find_free_data_block();

#endif
