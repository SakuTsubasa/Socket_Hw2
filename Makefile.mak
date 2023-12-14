all: server client

server: code/server.c
	gcc -o server code/server.c -lpthread

client: code/client.c
	gcc -o client code/client.c

clean:
	rm -f server client