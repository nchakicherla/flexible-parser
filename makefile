CC = gcc
# -O3 was previously written "-o3", which gcc parses as an output-file flag and
# silently discards, so every build so far was unoptimized.
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -O3 -Iexternal/linenoise
OS := $(shell uname)

mkBinDir := $(shell mkdir -p bin)
mkObjDir := $(shell mkdir -p obj)
mkDbgDir := $(shell mkdir -p debug)

BIN = ./bin/main.run

MAIN = 	./obj/main.o

OBJS = 	./obj/file.o \
		./obj/object.o \
		./obj/mempool.o \
		./obj/registry.o \
		./obj/scanner.o \
		./obj/ast.o \
		./obj/ast_node.o \
		./obj/color.o \
		./obj/grammar.o \
		./obj/parser.o \
		./obj/interp.o \
		./obj/repl.o \
		./obj/linenoise.o \

all: reset $(BIN)
ifeq ($(OS),Darwin)
	$(BIN)
else
	valgrind --track-origins=yes --leak-check=full $(BIN)
endif

reset: clean $(BIN)

run: $(BIN)
	$(BIN)

# The .def files feed the enums and tables through the preprocessor, so a change
# there has to rebuild everything.
DEFS = ./src/token.def ./src/syntax.def

$(BIN): $(OBJS) $(MAIN)
	$(CC) $(CFLAGS) $(OBJS) $(MAIN) -o $(BIN)

./obj/%.o: ./src/%.c ./src/%.h $(DEFS)
	$(CC) $(CFLAGS) -c $< -o $@

./obj/main.o: ./src/main.c $(DEFS)
	$(CC) $(CFLAGS) -c ./src/main.c -o ./obj/main.o

# Vendored third party (see THIRD_PARTY_LICENSES/). linenoise uses termios and
# ioctl calls that -Wpedantic rejects, and needs _GNU_SOURCE on some platforms.
./obj/linenoise.o: ./external/linenoise/linenoise.c ./external/linenoise/linenoise.h
	$(CC) $(filter-out -Wpedantic,$(CFLAGS)) -D_GNU_SOURCE -c $< -o $@

clean: clear-bin clear-obj

clear-bin:
	-rm -f ./bin/*

clear-obj:
	-rm -f ./obj/*
