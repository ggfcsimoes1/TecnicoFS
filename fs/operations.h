#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, pthread_rwlock_t inumber_buffer[],int * num_locks);
int delete(char *name, pthread_rwlock_t inumber_buffer[],int * num_locks);
int lookup(char *name, pthread_rwlock_t inumber_buffer[],int * num_locks);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
