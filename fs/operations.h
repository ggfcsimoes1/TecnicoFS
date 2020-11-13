#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, pthread_rwlock_t *inumber_buffer[],int * num_locks);
int delete(char *name, pthread_rwlock_t *inumber_buffer[], int * num_locks);
int lookup(char *name, pthread_rwlock_t *inumber_buffer[], int * num_locks);
int move(char* name1, char* name2,pthread_rwlock_t *inumber_buffer[], int * num_locks);
void print_tecnicofs_tree(FILE *fp);

int lookup_cd(char *name, pthread_rwlock_t *inumber_buffer[], int * num_locks);
int lookup_m(char *name, pthread_rwlock_t *inumber_buffer[], int * num_locks);

#endif /* FS_H */
