CC = gcc
CFLAGS = -c -Wall
LFLAGS = -lsocket

all: server client proxy

server: server.o
	$(CC) -o server server.o $(LFLAGS)

server.o: server.c
	$(CC) $(CFLAGS) server.c

client: client.o
	$(CC) -o client client.o $(LFLAGS)

client.o: client.c
	$(CC) $(CFLAGS) client.c

proxy: proxy.o
	$(CC) -o proxy proxy.o $(LFLAGS)

proxy.o: proxy.c
	$(CC) $(CFLAGS) proxy.c

clean:
	rm -f *.o server client proxy
