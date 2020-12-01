#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#include <pthread.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_INPUT_SIZE 100
#define CLNAME "/tmp/client"

char * servername;
int sockfd;
socklen_t clilen;
struct sockaddr_un client_addr;


int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

void sendToSocket(char* command){
  struct sockaddr_un serv_addr;
  socklen_t servlen = setSockAddrUn(servername, &serv_addr);
  if (sendto(sockfd, command , strlen(command)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 
}

void receiveFromSocket(char *buffer){
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }
}

int tfsCreate(char *filename, char nodeType) {
  char inbuffer[MAX_BUFFER_SIZE];
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"c %s %c",filename,nodeType);

  sendToSocket(outbuffer);
  receiveFromSocket(inbuffer);
  
  if(!strcmp(inbuffer,"s")){
    return 0;
  }
  return -1;
}

int tfsDelete(char *path) {
  char inbuffer[MAX_BUFFER_SIZE];
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"d %s",path);
  
  sendToSocket(outbuffer);
  receiveFromSocket(inbuffer);
  
  if(!strcmp(inbuffer,"s")){
    return 0;
  }
  return -1;
}

int tfsLookup(char *path){
  char inbuffer[MAX_BUFFER_SIZE];
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"l %s",path);

  sendToSocket(outbuffer);
  receiveFromSocket(inbuffer);
  
  if(!strcmp(inbuffer,"s")){
    return 0;
  }
  return -1;
}

int tfsMove(char *from, char *to){
  char inbuffer[MAX_BUFFER_SIZE];
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"m %s %s",from,to);

  sendToSocket(outbuffer);
  receiveFromSocket(inbuffer);
  
  if(!strcmp(inbuffer,"s")){
    return 0;
  }
  return -1;
}

int tfsPrint(char *command) {
  char inbuffer[MAX_BUFFER_SIZE];

  sendToSocket(command);
  receiveFromSocket(inbuffer);
  
  if(!strcmp(inbuffer,"s")){
    return 0;
  }
  return -1;
}

int tfsMount(char * sockPath) {
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(CLNAME);
  clilen = setSockAddrUn (CLNAME,&client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  }  

  servername=sockPath;

  return 0;
}

int tfsUnmount() {
  
  if((close(sockfd) && unlink(CLNAME)) ==0)
    return 0;  
  return -1;
}
