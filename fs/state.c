#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "state.h"
#include "../tecnicofs-api-constants.h"
#include <pthread.h>


/* -------------------------------------------------------------------------*/

/*Generic error function, prints whatever is given as input & exits the program
	errorMessage: the error message
*/
void error(const char* errorMessage){
    printf("%s\n",errorMessage);
    exit(EXIT_FAILURE);
}

/*Locks a thread according to the type of lock (void*) chosen @ the start.
	rw: 'r', 'w'
*/
void lock(pthread_rwlock_t * lock, char rw){
	
	if(rw == 'r' && pthread_rwlock_rdlock(lock) != 0){
		error("Error: locking failed");
        
	}
	else if (rw == 'w' && pthread_rwlock_wrlock(lock) != 0){
		error("Error: locking failed");
	}
    
}

/*Unlocks a previously locked thread
    lock: pthread_rwlock_t
*/
void unlock(pthread_rwlock_t * lock){	 
	 if(pthread_rwlock_unlock(lock) != 0){
	 	error("Error: unlocking failed");
	 }
}

/*Adds a lock to a thread-specific array which keeps track of whats being locked
    lock: pthread_rwlock_t
    current_inumber: inumber which holds the lock
    numLocks: number of locks held in the array
*/
void addToBuffer(int current_inumber, pthread_rwlock_t *iNumberBuffer[], int *numLocks){
	iNumberBuffer[*(numLocks)]= &inode_table[current_inumber].lock;
	*(numLocks)+=1;
}

/*Unlocks every locked thread placed in the buffer array
    numLocks: int
    INumberBuffer[]: pthread_rwlock_t
*/
void unlockAll(int * numLocks, pthread_rwlock_t *iNumberBuffer[]){
    while (*(numLocks) > 0){
        *(numLocks)-=1;
        unlock(iNumberBuffer[*(numLocks)]);
    }
    return;
}

/*Initializes a lock, associating it to a node in the i-node table
    inumber: the number where the node resides in the table
*/
void lockInit(int inumber){
    if (pthread_rwlock_init(&inode_table[inumber].lock, NULL) != 0){
        error("Error: failed to create lock");
    }
}

/*Destroys a lock
    inumber: the number where the node resides in the table
*/
void lockDestroy(int inumber){
    if (pthread_rwlock_destroy(&inode_table[inumber].lock) != 0){
        error("Error: failed to destroy lock");
    }
}

/* -------------------------------------------------------------------------*/

/*
 * Sleeps for synchronization testing.
 */
void insert_delay(int cycles) {
    for (int i = 0; i < cycles; i++) {}
}


/*
 * Initializes the i-nodes table.
 */
void inode_table_init() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
        lockInit(i);
    }
}

/*
 * Releases the allocated memory for the i-nodes tables.
 */

void inode_table_destroy() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        lockDestroy(i);
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
	    if (inode_table[i].data.dirEntries)
            free(inode_table[i].data.dirEntries);
        }
    }
}

/*
 * Creates a new i-node in the table with the given information.
 * Input:
 *  - nType: the type of the node (file or directory)
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int inode_create(type nType) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        if(pthread_rwlock_trywrlock(&inode_table[inumber].lock)!=0){
            continue;
        }
        else{
            
            if (inode_table[inumber].nodeType == T_NONE){
                inode_table[inumber].nodeType = nType;

                if (nType == T_DIRECTORY) {
                    /* Initializes entry table */
                    inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
                
                    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                        inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                    }
                }
                else {
                    inode_table[inumber].data.fileContents = NULL;
                }
                return inumber;
            }
            unlock(&inode_table[inumber].lock);
            continue;
        }
    }
    return FAIL;
}

/*
 * Deletes the i-node.
 * Input:
 *  - inumber: identifier of the i-node
 * Returns: SUCCESS or FAIL
 */
int inode_delete(int inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);
    
    
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_delete: invalid inumber\n");
        return FAIL;
    } 


    inode_table[inumber].nodeType = T_NONE;

    /* see inode_table_destroy function */
    
    if (inode_table[inumber].data.dirEntries){
        free(inode_table[inumber].data.dirEntries);
    }

    return SUCCESS;
}

/*
 * Copies the contents of the i-node into the arguments.
 * Only the fields referenced by non-null arguments are copied.
 * Input:
 *  - inumber: identifier of the i-node
 *  - nType: pointer to type
 *  - data: pointer to data
 * Returns: SUCCESS or FAIL
 */
int inode_get(int inumber, type *nType, union Data *data) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_get: invalid inumber %d\n", inumber);
        return FAIL;
    }

    if (nType)
        *nType = inode_table[inumber].nodeType;

    if (data)
        *data = inode_table[inumber].data;

    return SUCCESS;
}


/*
 * Resets an entry for a directory.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 * Returns: SUCCESS or FAIL
 */
int dir_reset_entry(int inumber, int sub_inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);
    
    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_reset_entry: can only reset entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < FREE_INODE) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid entry inumber\n");
        return FAIL;
    }

    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == sub_inumber) {
            inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
            inode_table[inumber].data.dirEntries[i].name[0] = '\0';
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Adds an entry to the i-node directory data.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 *  - sub_name: name of the sub i-node entry 
 * Returns: SUCCESS or FAIL
 */
int dir_add_entry(int inumber, int sub_inumber, char *sub_name) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_add_entry: can only add entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < 0) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid entry inumber\n");
        return FAIL;
    }

    if (strlen(sub_name) == 0 ) {
        printf("inode_add_entry: \
               entry name must be non-empty\n");
        return FAIL;
    }
    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == FREE_INODE) {
            inode_table[inumber].data.dirEntries[i].inumber = sub_inumber;
            strcpy(inode_table[inumber].data.dirEntries[i].name, sub_name);
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Prints the i-nodes table.
 * Input:
 *  - inumber: identifier of the i-node
 *  - name: pointer to the name of current file/dir
 */
void inode_print_tree(FILE *fp, int inumber, char *name) {
    if (inode_table[inumber].nodeType == T_FILE) {
        fprintf(fp, "%s\n", name);
        return;
    }

    if (inode_table[inumber].nodeType == T_DIRECTORY) {
        fprintf(fp, "%s\n", name);
        for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
            if (inode_table[inumber].data.dirEntries[i].inumber != FREE_INODE) {
                char path[MAX_FILE_NAME];
                if (snprintf(path, sizeof(path), "%s/%s", name, inode_table[inumber].data.dirEntries[i].name) > sizeof(path)) {
                    fprintf(stderr, "truncation when building full path\n");
                }
                inode_print_tree(fp, inode_table[inumber].data.dirEntries[i].inumber, path);
            }
        }
    }
}
