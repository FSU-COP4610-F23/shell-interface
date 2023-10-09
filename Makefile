SRC := src
OBJ := obj
BIN := bin
SHELL_EXECUTABLE := shell
MYTIMEOUT_EXECUTABLE := mytimeout

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
INCS := -Iinclude/

CC := gcc
CFLAGS := -g -Wall -std=c99 $(INCS)

all: $(BIN)/$(SHELL_EXECUTABLE) $(BIN)/$(MYTIMEOUT_EXECUTABLE)

$(BIN)/$(SHELL_EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN)/$(MYTIMEOUT_EXECUTABLE): $(OBJ)/mytimeout/mytimeout.o
	$(CC) $(CFLAGS) $^ -o $@
	cp $(BIN)/$(MYTIMEOUT_EXECUTABLE) ./

$(OBJ)/mytimeout/mytimeout.o: $(SRC)/mytimeout/mytimeout.c
	@mkdir -p $(OBJ)/mytimeout
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(OBJ) $(BIN)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)/$(SHELL_EXECUTABLE)
	$<

clean:
	rm -rf $(OBJ)/*.o $(BIN)/$(SHELL_EXECUTABLE) $(MYTIMEOUT_EXECUTABLE) $(OBJ) $(BIN)

.PHONY: run clean all
