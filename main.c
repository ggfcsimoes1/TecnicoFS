/*TECNICOFS - Gustavo Simoes 95588 / Miguel Prazeres 95649*/


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "fs/operations.h"
#include "fs/state.h"

#define INDIM 30
#define OUTDIM 512
#define SNAME "/tmp/server"

#define TRUE 1
#define FALSE !TRUE
#define FAIL -1
#define SUCCESS 0
#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define MAX_PARAMETERS 5

int numberThreads = 0;
int sockfd;
pthread_mutex_t mutexGlobal = PTHREAD_MUTEX_INITIALIZER; /* Initializing the locks used for syncing threads*/
struct timeval itime, ftime;

int fsChangingCommands = 0;
pthread_cond_t canPrint = PTHREAD_COND_INITIALIZER;
pthread_mutex_t printLock = PTHREAD_MUTEX_INITIALIZER;


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

FILE* openFile(const char* filePath, const char *mode){
    FILE *fp = fopen(filePath, mode);
    if(!fp){
        error("Error: invalid file path");
    }
    return fp;
}

int getNumThreads(int argc, char* numT){
    int numThreads = atoi(numT);
    if (argc != 3){
        error("Error: invalid parameters amount");  
    }
    if(!numThreads || numThreads <= 0){         /* If the conversion is invalid / thread number is negative ... */
        error("Error: invalid thread number");
    }
    return numThreads;
}

void globalLock(){
    if (pthread_mutex_lock(&mutexGlobal) != 0){
        error("Error: mishandled global lock");
    }  
}

void globalUnlock(){
    if (pthread_mutex_unlock(&mutexGlobal) != 0){
        error("Error: mishandled global lock");
    }  
}

/*--------------------------------------------------------------------------------*/

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    globalUnlock();
    exit(EXIT_FAILURE);
}

int setSockAddrUn(struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, SNAME);

  return SUN_LEN(addr);
}

int serverMount(char * serverpath){
  struct sockaddr_un server_addr;
  socklen_t addrlen;

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    perror("server: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(serverpath);

  addrlen = setSockAddrUn (&server_addr);
  if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
    perror("server: bind error");
    exit(EXIT_FAILURE);
  }
  
  return 0;
}


int applyCommands(char * command){

    pthread_rwlock_t * iNumberBuffer[INODE_TABLE_SIZE];  /*Buffer used to keep track of the inode locks, during syncronization*/
    int numLocks = 0;  /*index of "iNumberBuffer"*/
    int operationSuccessful = SUCCESS;

    globalLock();

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
                    fsChangingCommands++;
                    if(create(name, T_FILE,iNumberBuffer,&numLocks) == FAIL)
                        operationSuccessful = FAIL;
                    fsChangingCommands--;
                    pthread_cond_signal(&canPrint);
                    break;
                case 'd':
                    printf("Create directory: %s\n", name);
                    fsChangingCommands++;
                    if(create(name, T_DIRECTORY,iNumberBuffer,&numLocks) == FAIL)
                        operationSuccessful = FAIL;
                    fsChangingCommands--;
                    pthread_cond_signal(&canPrint);
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
            else{
                printf("Search: %s not found\n", name);
                operationSuccessful = FAIL;
            }     
            break;
        case 'd':
            printf("Delete: %s\n", name);
            fsChangingCommands++;
            if(delete(name,iNumberBuffer,&numLocks) == FAIL)
                operationSuccessful = FAIL;
            fsChangingCommands--;
            pthread_cond_signal(&canPrint);
            break;

        case 'm':
            printf("Move: %s %s\n", name, name2);
            fsChangingCommands++;
            if(move(name, name2, iNumberBuffer, &numLocks) == FAIL)
                operationSuccessful = FAIL;
            fsChangingCommands--;
            pthread_cond_signal(&canPrint);
            break;

        case 'p':
            pthread_mutex_lock(&printLock);
            while (fsChangingCommands != 0)
                pthread_cond_wait(&canPrint, &printLock);
            
            print_tecnicofs_tree(openFile(name, "w"));
            pthread_mutex_unlock(&printLock);
            break;
            
            printf("Print filesystem to: %s \n", name);
            
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    unlockAll(&numLocks, iNumberBuffer);
    return operationSuccessful;
}

void *serverApplyCommands(){
    
    while (TRUE) {
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c;
        socklen_t addrlen;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        if (applyCommands(in_buffer) == 0){
            c = sprintf(out_buffer, "s");
        }
        else{
            c = sprintf(out_buffer, "f");
        }
        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
    }
}
    

/*--------------------------------------------------------------------------------*/

void createThread(int numberThreads, pthread_t tid[]){
    for (int i=0; i<numberThreads; i++){    /* Creating the threads */
        if(pthread_create(&tid[i], NULL, serverApplyCommands , NULL)!=0){
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
    /*SNAME -> argv[2]*/
   if (serverMount(argv[2]) == 0)
      printf("Mounted! (socket = %s)\n", argv[2]);
    else {
      fprintf(stderr, "Unable to mount socket: %s\n", argv[2]);
      exit(EXIT_FAILURE);
    }
    
    numberThreads = getNumThreads(argc, argv[1]);
    pthread_t tid[numberThreads];  /*Declaring the thread pool*/ 
  
    init_fs();   /*init filesystem*/ 
    startTimer();    /*Starting the timer...*/ 
    
    createThread(numberThreads, tid);
    
    finishThread(numberThreads, tid);
    
    stopTimer();     /*Stopping the timer...*/ 
    getExecTime();

        
    destroy_fs();    /*release allocated memory*/ 
    exit(EXIT_SUCCESS);
}
