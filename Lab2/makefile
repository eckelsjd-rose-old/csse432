all: ./server/server ./client/client

server: ./server/server.o
	gcc ./server/server.o -o ./server/server -g

server.o: ./server/server.c
	gcc ./server/server.c -c -g -o ./server/server.o

client: ./client/client.o
	gcc ./client/client.o -o ./client/client -g

client.o: ./client/client.c
	gcc ./client/client.c -c -g -o ./client/client.o

clean:
	rm ./client/*.o ./server/*.o ./server/server ./client/client
