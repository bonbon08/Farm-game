CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 $(shell pkg-config --cflags raylib)
LDFLAGS := $(shell pkg-config --libs raylib) -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
BIN := farmi

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c src/farm.h
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)
