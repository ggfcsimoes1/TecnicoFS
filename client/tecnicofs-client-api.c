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
#define CLNAME "/tmp/client" /* basename for the socket */

char clientPath[MAX_BUFFER_SIZE];
char * servername;
int sockfd;
socklen_t clilen;
struct sockaddr_un client_addr;

/*
 * Sets the client socket adress
 * Input:
 *  - path: pointer to the client path
 *  - addr: pointer to the server socket adress
 */
int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/*
 * Sends a certain string to the socket adress (In this case, sends a command to the server)
 * Input:
 *  - command: string to be sent
 */
void sendToSocket(char* command){
  struct sockaddr_un serv_addr;
  socklen_t servlen = setSockAddrUn(servername, &serv_addr);
  if (sendto(sockfd, command , strlen(command)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 
}

/*
 * Receives the result of the operation from the server
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int receiveFromSocket(){
  int res=0;
  if (recvfrom(sockfd, &res, sizeof(res), 0, 0, 0) < 0){ /*receiving an int that indicates whether the operation was successful or not*/
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }
  return res; /*return the result (0 or -1, depending on the validity of the operation)*/
}

/*
 * Sends a request to create a node in the server
 * 
 * Input:
 *  -filename: the name of the node
 *  -nodetype: the type of node ('f', 'd')
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsCreate(char *filename, char nodeType) {
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"c %s %c",filename,nodeType); /*building a command*/

  sendToSocket(outbuffer);  
  return receiveFromSocket(); /*receiving the result of the operation*/
}

/*
 * Sends a request to delete a node in the server
 * 
 * Input:
 *  -path: the name of the node to be deleted
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsDelete(char *path) {
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"d %s",path);
  
  sendToSocket(outbuffer);
  return receiveFromSocket();
}

/*
 * Sends a request to lookup a node in the server
 * 
 * Input:
 *  -path: the name of the node to be found
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsLookup(char *path){
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"l %s",path);

  sendToSocket(outbuffer);
  return receiveFromSocket();
}


/*
 * Sends a request to move a node in the server
 * 
 * Input:
 *  -from: starting location of the node
 *  -to: final location of the node
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsMove(char *from, char *to){
  char outbuffer[MAX_BUFFER_SIZE];
  sprintf(outbuffer,"m %s %s",from,to);

  sendToSocket(outbuffer);
  return receiveFromSocket();
}

/*
 * Sends a request to print the state of the server
 * 
 * Input:
 *  -filename: the name of file to be written in.
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsPrint(char *filename) {
  sendToSocket(filename);
  return receiveFromSocket();
}

/*
 * Mounts the client socket
 * 
 * Input:
 *  -sockPath: the path of the client socket.
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsMount(char * sockPath) {
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }
  sprintf(clientPath, "%s-%d", CLNAME, getpid()); /*building a unique client path*/
  unlink(clientPath);
  clilen = setSockAddrUn(clientPath,&client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  }  
  servername=sockPath;
  return 0;
}

/*
 * Unmounts the client socket
 * 
 * Output:
 *  - the result (SUCESS/FAIL)
 */
int tfsUnmount() {
  if((close(sockfd) && unlink((clientPath)))==0)
    return 0;  
  return -1;
}
