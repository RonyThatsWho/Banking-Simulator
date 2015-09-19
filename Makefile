CC=gcc
CFLAGS= -Wall -Werror -Wextra -pedantic -g -O0
CLIENT_OBJ = client.o
SERVER_OBJ = server.o server.h
LIBS = -lpthread

all: client server

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

server: $(SERVER_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
        
.PHONY: clean

clean:
	rm -f client server *.o core 