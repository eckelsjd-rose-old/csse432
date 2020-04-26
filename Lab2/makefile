BIN = ./bin
SRC = ./src
CC = gcc
CFLAGS = -g

all: $(BIN)/server $(BIN)/client $(BIN)/bytes

$(BIN)/server: $(BIN)/server.o
	$(CC) $(BIN)/server.o -o $(BIN)/server $(CFLAGS)

$(BIN)/server.o: $(SRC)/server.c
	$(CC) $(SRC)/server.c -c -o $(BIN)/server.o $(CFLAGS)

$(BIN)/client: $(BIN)/client.o
	$(CC) $(BIN)/client.o -o $(BIN)/client $(CFLAGS)

$(BIN)/client.o: $(SRC)/client.c
	$(CC) $(SRC)/client.c -c -o $(BIN)/client.o $(CFLAGS)

$(BIN)/bytes: $(SRC)/ByteGenerator.c
	$(CC) $(SRC)/ByteGenerator.c -o $(BIN)/bytes $(CFLAGS)

clean:
	rm $(BIN)/*
