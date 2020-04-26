all: server client

server: server.o
	gcc server.o -o server -g

server.o: server.c
	gcc server.c -c -g

client: client.o
	gcc client.o -o client -g

client.o: client.c
	gcc client.c -c -g

clean:
	rm *.o server client
