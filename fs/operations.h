#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, pthread_rwlock_t *iNumberBuffer[], int *numLocks);
int delete(char *name, pthread_rwlock_t *iNumberBuffer[], int *numLocks);
int lookup(char *name, pthread_rwlock_t *iNumberBuffer[], int *numLocks);
int move(char* name1, char* name2,pthread_rwlock_t *iNumberBuffer[], int *numLocks);
void print_tecnicofs_tree(FILE *fp, pthread_rwlock_t *iNumberBuffer[], int * numLocks);

int lookup_cd(char *name, pthread_rwlock_t *iNumberBuffer[], int *numLocks);
int lookup_m(char *name, pthread_rwlock_t *iNumberBuffer[], int *numLocks);

#endif /* FS_H */
