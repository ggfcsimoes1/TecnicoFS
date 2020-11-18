#include "operations.h"
#include "state.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>



/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	
	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
	
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	unlock(&inode_table[root].lock);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
			return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 *  - iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, pthread_rwlock_t *iNumberBuffer[], int *numLocks){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;
	strcpy(name_copy, name);
	
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = lookup_cd(parent_name, iNumberBuffer, numLocks);
	
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		return FAIL;
	}
	addToBuffer(child_inumber, iNumberBuffer, numLocks); /*Adding child's lock to the buffer, only if it's creation was successful*/

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}
	return SUCCESS;
}

/*
 * Move: changes a node's path.
 * Input:
 *  - name1: path of node's origin
 * 	- name2: path of node's destiny
 *  - iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns: SUCCESS or FAIL
 */
int move(char *name1, char *name2,pthread_rwlock_t *iNumberBuffer[], int *numLocks){
	int parent_inumber1, parent_inumber2, child_inumber1, child_inumber2;
	char *parent_name1, *child_name1, *parent_name2, *child_name2, name_copy1[MAX_FILE_NAME], name_copy2[MAX_FILE_NAME];
	
	type pType1, pType2, cType1;
	union Data pdata1, pdata2, cdata1; 

	strcpy(name_copy1, name1);
	strcpy(name_copy2, name2);

	split_parent_child_from_path(name_copy1, &parent_name1, &child_name1);
	split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);

	/*The execution of "lookup_m" is dependant on the path's order, to make sure that inodes are locked in 
	alphabetic order, avoiding dead locks*/

	if (strcmp(name_copy1,name_copy2) >= 0){
		
		parent_inumber2 = lookup_m(name_copy2, iNumberBuffer, numLocks);
		parent_inumber1 = lookup_m(name_copy1, iNumberBuffer, numLocks);
	}
	else{
		parent_inumber1 = lookup_m(name_copy1, iNumberBuffer, numLocks);
		parent_inumber2 = lookup_m(name_copy2, iNumberBuffer, numLocks);
	}
	
	
	if (parent_inumber1 == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",  /*Basic verifications of initial path*/
		        name1, parent_name1);
		return FAIL;
	}
	
	inode_get(parent_inumber1, &pType1, &pdata1);

	if(pType1 != T_DIRECTORY){
		printf("failed to move %s, parent %s is not a dir\n",
		        name1, parent_name1);
		return FAIL;
	}

	child_inumber1 = lookup_sub_node(child_name1, pdata1.dirEntries);

	if (child_inumber1 == FAIL) {
		printf("could not move %s, does not exist in dir %s\n",
		       name1, parent_name1);
		return FAIL;
	}

	
	if (parent_inumber2 == FAIL) {
		printf("failed to move %s, invalid parent dir %s\n",   /*Basic verifications of the final path*/
		        name2, parent_name2);
		return FAIL;
	}
	
	inode_get(parent_inumber2, &pType2, &pdata2);

	if(pType2 != T_DIRECTORY){
		printf("failed to move %s, parent %s is not a dir\n",
		        name2, parent_name2);
		return FAIL;
	}

	child_inumber2 = lookup_sub_node(child_name2, pdata2.dirEntries);

	if (child_inumber2 != FAIL || child_inumber1 == parent_inumber2) {  /*Making sure the origin isn't the destiny*/
		printf("could not move %s, already exists in dir %s\n",
		       name2, parent_name2);
		return FAIL;
	}

	inode_get(child_inumber1, &cType1, &cdata1);

	if (dir_add_entry(parent_inumber2, child_inumber1, child_name1) == FAIL) {   /*Adding inode to new location*/
		printf("could not add entry %s in dir %s\n",
		       child_name1, parent_name2);
		return FAIL;
	}

	if (dir_reset_entry(parent_inumber1, child_inumber1) == FAIL) {   /*Deleting inode from origins*/
		printf("failed to delete %s from dir %s\n",
		       child_name1, parent_name1);
		return FAIL;
	}

	return SUCCESS;

}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * 	- iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns: SUCCESS or FAIL
 */
int delete(char *name, pthread_rwlock_t *iNumberBuffer[],int * numLocks){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);	
	
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup_cd(parent_name, iNumberBuffer, numLocks);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		return FAIL;
	} 
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * 	- iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, pthread_rwlock_t *iNumberBuffer[], int *numLocks) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	lock(&inode_table[current_inumber].lock,'r'); /*locking root*/
	addToBuffer(current_inumber, iNumberBuffer, numLocks);
	
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim,&saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		lock(&inode_table[current_inumber].lock,'r'); /*locking every subnode*/
		addToBuffer(current_inumber, iNumberBuffer, numLocks);
		
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
	}
	
	return current_inumber;
}

/*
 * Lookup for a given path ( used in "create" and "delete").
 * Input:
 *  - name: path of node
 * 	- iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup_cd(char *name, pthread_rwlock_t *iNumberBuffer[], int * numLocks) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/*If the create/delete occours in the root*/
	if(!strcmp(name,"")){        
		lock(&inode_table[current_inumber].lock,'w');
		addToBuffer(current_inumber, iNumberBuffer, numLocks);
	}
	else{
		lock(&inode_table[current_inumber].lock,'r');
		addToBuffer(current_inumber, iNumberBuffer, numLocks);
	}
	
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim,&saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		if(path[strlen(path)-1]=='\0'){
			lock(&inode_table[current_inumber].lock,'w');          /*lock the parent inode for "write"*/
			addToBuffer(current_inumber, iNumberBuffer, numLocks);
			inode_get(current_inumber, &nType, &data);
			break;
		}
		
 		lock(&inode_table[current_inumber].lock,'r');     /*lock all the subnodes for "read"*/
    	addToBuffer(current_inumber, iNumberBuffer, numLocks);
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
	}
	return current_inumber;
}

/*
 * Lookup for a given path (used in "move").
 * Input:
 *  - name: path of node
 * 	- iNumberBuffer[]: array of pointers to pthread_rwlock_t
 *  - numLocks: index of the array above
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup_m(char *name, pthread_rwlock_t *iNumberBuffer[], int * numLocks) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/*Since the order in which the locks are acquired may vary, trylocks are used in order to prevent deadlocking*/
	if(!strcmp(name,"") && pthread_rwlock_trywrlock(&inode_table[current_inumber].lock) == 0 ){
		addToBuffer(current_inumber, iNumberBuffer, numLocks);
	}
	else if(pthread_rwlock_tryrdlock(&inode_table[current_inumber].lock) == 0){
		addToBuffer(current_inumber, iNumberBuffer, numLocks);
	}
	
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim,&saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		if(path[strlen(path)-1]=='\0'){
			if(pthread_rwlock_trywrlock(&inode_table[current_inumber].lock) == 0 ){
				addToBuffer(current_inumber, iNumberBuffer, numLocks);
			}
			inode_get(current_inumber, &nType, &data);
			break;
		}
		
		if(pthread_rwlock_tryrdlock(&inode_table[current_inumber].lock) == 0){
			addToBuffer(current_inumber, iNumberBuffer, numLocks);
		}
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
	}
	return current_inumber;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
	fclose(fp);
}
