BIN = ./bin
SRC = ./src
LIB = ./lib
INCLUDE = ./include
CC = gcc
P1 = server
P2 = client
P3 = bytes
L1 = eckelsjd
L2 = proxy_parse
CFLAGS = -g -L $(LIB) -I $(INCLUDE) -l $(L1) -l $(L2)

all: $(BIN)/$(P1) $(BIN)/$(P2) $(BIN)/$(P3)

$(BIN)/$(P1): $(BIN)/$(P1).o $(LIB)/lib$(L1).so $(LIB)/lib$(L2).so
	$(CC) $(BIN)/$(P1).o -o $(BIN)/$(P1) $(CFLAGS)

$(BIN)/$(P1).o: $(SRC)/$(P1).c
	$(CC) $(SRC)/$(P1).c -c -o $(BIN)/$(P1).o $(CFLAGS)

$(BIN)/$(P2): $(BIN)/$(P2).o $(LIB)/lib$(L1).so $(LIB)/lib$(L2).so
	$(CC) $(BIN)/$(P2).o -o $(BIN)/$(P2) $(CFLAGS)

$(BIN)/$(P2).o: $(SRC)/$(P2).c 
	$(CC) $(SRC)/$(P2).c -c -o $(BIN)/$(P2).o $(CFLAGS)

$(LIB)/lib$(L1).so: $(SRC)/lib$(L1).c
	$(CC) $(SRC)/lib$(L1).c -shared -o $(LIB)/lib$(L1).so -c

$(LIB)/lib$(L2).so: $(SRC)/lib$(L2).c
	$(CC) $(SRC)/lib$(L2).c -shared -o $(LIB)/lib$(L2).so -c -I $(INCLUDE)

$(BIN)/$(P3): $(SRC)/ByteGenerator.c
	$(CC) $(SRC)/ByteGenerator.c -o $(BIN)/$(P3)

clean:
	rm $(BIN)/* $(LIB)/*
