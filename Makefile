# Makefile, versao 2
# Sistemas Operativos, DEI/IST/ULisboa 2020-21, Changed by Miguel & Gustavo

# -----------------------------------------------------------------------------------------------------------------------
# | The Makefile now compiles every necessary file for execution and places the executable files in the main directory. |
# | It also cleans the txt files created by the program in the 'outputs' folder. Added for conveniency purposes.		|
# -----------------------------------------------------------------------------------------------------------------------

CC   = gcc
LD   = gcc
CFLAGS = -pthread -Wall -std=gnu99 -I../ 
LDFLAGS=-lm -lpthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs tecnicofs-client

tecnicofs: fs/state.o fs/operations.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs fs/state.o fs/operations.o main.o

fs/state.o: fs/state.c fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o fs/operations.o -c fs/operations.c

main.o: main.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o main.o -c main.c

client/tecnicofs-client-api.o: client/tecnicofs-client-api.c client/tecnicofs-client-api.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) -o client/tecnicofs-client-api.o -c client/tecnicofs-client-api.c

client/tecnicofs-client.o: client/tecnicofs-client.c tecnicofs-api-constants.h client/tecnicofs-client-api.h
	$(CC) $(CFLAGS) -o client/tecnicofs-client.o -c client/tecnicofs-client.c

tecnicofs-client: client/tecnicofs-client-api.o client/tecnicofs-client.o
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-client client/tecnicofs-client-api.o client/tecnicofs-client.o

clean:
	@echo Cleaning...
	rm -f fs/*.o *.o tecnicofs client/*.o tecnicofs-client outputs/*.txt

run: tecnicofs
	./tecnicofs
