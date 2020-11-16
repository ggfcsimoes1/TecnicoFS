/*TECNICOFS - Gustavo Simoes 95588 / Miguel Prazeres 95649*/


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "fs/operations.h"
#include "fs/state.h"

#define TRUE 1
#define FALSE !TRUE
#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define MAX_PARAMETERS 5

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueueR = 0; 
int headQueueI = 0;
int done_insert = 0;
int done_apply = 0;

pthread_mutex_t mutex_global = PTHREAD_MUTEX_INITIALIZER; /* Initializing the locks used for syncing threads*/
struct timeval itime, ftime;
pthread_cond_t canAdd, canGrab;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

/*--------------------------------------------------------------------------------*/

void startTimer(){
    gettimeofday(&itime ,NULL);
}

void stopTimer(){
    gettimeofday(&ftime ,NULL);
}

void getExecTime(){ /* Function that calculates the program's run time*/
    double milliseconds = (double) (ftime.tv_usec - itime.tv_usec) / 1000000; /* tv_usec/tv_sec are ints, so a cast is required in order to get an accurate reading */
    double seconds = (double) (ftime.tv_sec - itime.tv_sec);        
    printf("TecnicoFS completed in %.4f seconds.\n", seconds + milliseconds);
}

int getNumThreads(int argc, char* numT){
    int numThreads = atoi(numT);
    if (argc != 4){
        error("Error: invalid parameters amount");  
    }
    if(!numThreads || numThreads <= 0){         /* If the conversion is invalid / thread number is negative ... */
        error("Error: invalid thread number");
    }
    return numThreads;
}

FILE* openFile(const char* filePath, const char *mode){
    FILE *fp = fopen(filePath, mode);
    if(!fp){
        error("Error: invalid file path");
    }
    return fp;
}

void globalLock(){
    if (pthread_mutex_lock(&mutex_global) != 0){
        error("Error: mishandled global lock");
    }  
}

void globalUnlock(){
    if (pthread_mutex_unlock(&mutex_global) != 0){
        error("Error: mishandled global lock");
    }  
}

/*--------------------------------------------------------------------------------*/

int insertCommand(char* data) {  
    while(numberCommands == MAX_COMMANDS) {
        pthread_cond_wait(&canAdd,&mutex_global);
    }
    numberCommands++;
    strcpy(inputCommands[headQueueI++], data);
    if(headQueueI==MAX_COMMANDS) {
        headQueueI = 0;
    }
    pthread_cond_signal(&canGrab); 
    return 1;
}

void removeCommand(char ** command) {    
    while(numberCommands == 0){
        pthread_cond_wait(&canGrab,&mutex_global);
    }
    numberCommands--;
    *command = inputCommands[headQueueR++];    
    if(headQueueR==MAX_COMMANDS){
        headQueueR = 0;
    }   
    pthread_rwlock_wrlock(&rwlock);
    if(done_insert && numberCommands==0){
        done_apply=1;
        pthread_rwlock_unlock(&rwlock);
        return;
    }
    pthread_rwlock_unlock(&rwlock);  
    pthread_cond_signal(&canAdd);
    
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    globalUnlock();
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputfile){

    char line[MAX_INPUT_SIZE];
    
    /* break loop with ^Z or ^D */
    
    while (fgets(line, sizeof(line)/sizeof(char), inputfile)) {
        
        globalLock();
        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens;

        sscanf(line, "%c", &token);
        if(token == 'm'){
            char name2[MAX_INPUT_SIZE];
            numTokens = sscanf(line, "%c %s %s", &token, name, name2);
        }else{
            numTokens = sscanf(line, "%c %s %c", &token, name, &type);
        }
        
        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'm':
                 if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
        globalUnlock();
    }
    //pthread_rwlock_wrlock(&rwlock);
    globalLock();
    done_insert=1;
    pthread_cond_broadcast(&canAdd);
    globalUnlock();
    //pthread_rwlock_unlock(&rwlock);
}

void* applyCommands(){

    pthread_rwlock_t * iNumberBuffer[INODE_TABLE_SIZE];
    int numLocks = 0;
    
    while (TRUE){

        globalLock();

        char* command=NULL;
        removeCommand(&command);

        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        char name2[MAX_INPUT_SIZE];
        int numTokens;

        sscanf(command, "%c", &token);
        if(token == 'm'){
            numTokens = sscanf(command, "%c %s %s", &token, name, name2);  
        }else{
            numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        }

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        globalUnlock();
     
        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f': 
                        printf("Create file: %s\n", name);
                        create(name, T_FILE,iNumberBuffer,&numLocks);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY,iNumberBuffer,&numLocks);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name,iNumberBuffer,&numLocks);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);     
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name,iNumberBuffer,&numLocks);
                break;

            case 'm':
                printf("Move: %s %s\n", name, name2);
                move(name, name2, iNumberBuffer, &numLocks);
                break;
                
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        
        unlockAll(&numLocks, iNumberBuffer);
        
        pthread_rwlock_rdlock(&rwlock);
        if(done_apply){
            pthread_rwlock_unlock(&rwlock);
            break;
        }
        pthread_rwlock_unlock(&rwlock);
        
    }
    return 0;
}

/*--------------------------------------------------------------------------------*/

void createThread(int numberThreads, pthread_t tid[]){
    for (int i=0; i<numberThreads; i++){    /* Creating the threads */
        if(pthread_create(&tid[i], NULL, applyCommands, NULL)!=0){
            error("Error: thread_create");                                                         /* Calling in case the creation goes wrong*/
        }       
    }
}

void finishThread(int numberThreads, pthread_t tid[]){
    for (int i=0; i<numberThreads; i++){    /* Terminating the threads */ 
        if(pthread_join(tid[i], NULL) != 0){
            error("Error: thread_join");
        }   
    }
}

/*--------------------------------------------------------------------------------*/

int main(int argc, char* argv[]){
    
       /* Declaring the thread pool */
    numberThreads = getNumThreads(argc, argv[3]);
    pthread_t tid[numberThreads];
    FILE *inputfile = openFile(argv[1], "r");
    FILE *outputfile = openFile(argv[2], "w");
    pthread_cond_init(&canAdd, NULL);
    pthread_cond_init(&canGrab, NULL);
  
    init_fs();  /* init filesystem */
    startTimer();   /* Starting the timer... */
    
    
    createThread(numberThreads, tid);
    processInput(inputfile);    /* process input and print tree */
    fclose(inputfile);
    
    finishThread(numberThreads, tid);
    
  
    stopTimer();    /* Stopping the timer... */
    getExecTime();

    print_tecnicofs_tree(outputfile); /* print input and close the output file...*/
    pthread_cond_destroy (&canAdd);
    pthread_cond_destroy (&canGrab);
    destroy_fs();   /* release allocated memory */
    exit(EXIT_SUCCESS);
}
