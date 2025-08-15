#ifndef SFS_H
#define SFS_H

#include "fs_defs.h"

// API do Sistema de Arquivos

void fs_format();
void fs_mount();
void fs_stat();
int find_entry(const char* name, DirectoryEntry* entry);
void fs_ls();
int  fs_mkdir(const char* dirname);
int  fs_touch(const char* filename);
int  fs_cd(const char* path);
int  fs_cat(const char* filename);
int  fs_write(const char* filename, const char* text);
int  fs_rm(const char* filename);
const char* fs_get_current_path();

#endif
