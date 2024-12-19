CC = gcc
CFLAGS = -std=c99 -Wall -Wpedantic -Werror -o3
OS := $(shell uname)

mkBinDir := $(shell mkdir -p bin)
mkObjDir := $(shell mkdir -p obj)

BIN = ./bin/main.run

MAIN = 	./obj/main.o

OBJS = 	./obj/file.o \
		./obj/object.o \
		./obj/mempool.o \
		./obj/scanner.o \
		./obj/ast.o \
		./obj/ast_node.o \
		./obj/color.o \
		./obj/grammar.o \
		./obj/parser.o \

default: reset $(BIN)
ifeq ($(OS),Darwin) 
	$(BIN)
else 
	valgrind --track-origins=yes --leak-check=full $(BIN)  
endif

reset: clear $(BIN)

run: reset $(BIN)
	$(BIN)

$(BIN): $(OBJS) $(MAIN)
	$(CC) $(CFLAGS) $(OBJS) $(MAIN) -o $(BIN)

./obj/%.o: ./src/%.c ./src/%.h
	$(CC) $(CFLAGS) -c $< -o $@

./obj/main.o: ./src/main.c
	$(CC) $(CFLAGS) -c ./src/main.c -o ./obj/main.o
	
clear: clear-bin clear-obj

clear-bin:
	-rm ./bin/*

clear-obj:
	-rm ./obj/*
