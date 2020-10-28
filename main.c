/*TECNICOFS - Gustavo Simoes 95588 / Miguel Prazeres 95649*/


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define TRUE 1
#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MAX_PARAMETERS 5

struct timeval itime, ftime;

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0; 

pthread_mutex_t mutex_global = PTHREAD_MUTEX_INITIALIZER; /* Initializing the locks used for syncing threads*/


/*--------------------------------------------------------------------------------*/

void error(const char* errorMessage){
    printf("%s\n",errorMessage);
    exit(EXIT_FAILURE);
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

/*--------------------------------------------------------------------------------*/


int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputfile){

    char line[MAX_INPUT_SIZE];
    
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), inputfile)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

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
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void* applyCommands(){

    pthread_rwlock_t iNumberBuffer[INODE_TABLE_SIZE];
    int numLocks = 0;
    int copy;
    
    
    while (TRUE){

        globalLock();

        if (numberCommands <= 0){
            globalUnlock();
            break;
        }

        const char* command = removeCommand();

        globalUnlock();     /*  Unlocking... */

        if (command == NULL){
            continue;
        }


        char token, type;
        char name[MAX_INPUT_SIZE];
       

        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

         
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
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }

        copy=numLocks;
        for(int i=0; i < copy; i++){
            unlock_sync(&iNumberBuffer[i]);
            numLocks--;
        }         
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
    
    pthread_t tid[numberThreads];   /* Declaring the thread pool */
    numberThreads = getNumThreads(argc, argv[3]);
    FILE *inputfile = openFile(argv[1], "r");
    FILE *outputfile = openFile(argv[2], "w");
  
    init_fs();  /* init filesystem */

    processInput(inputfile);    /* process input and print tree */
    fclose(inputfile);

    startTimer();   /* Starting the timer... */

    createThread(numberThreads, tid);
    finishThread(numberThreads, tid);
  
    stopTimer();    /* Stopping the timer... */
    getExecTime();

    print_tecnicofs_tree(outputfile); /* print input and close the output file...*/

    destroy_fs();   /* release allocated memory */
    exit(EXIT_SUCCESS);
}
