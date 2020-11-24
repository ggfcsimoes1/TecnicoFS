#ifndef API_H
#define API_H

#include "tecnicofs-api-constants.h"

int tfsCreate(char *line);
int tfsDelete(char *line);
int tfsLookup(char *line);
int tfsMove(char *line);
int tfsMount(char* serverName);
int tfsUnmount();

void sendToSocket(char* command);

#endif /* CLIENT_H */
