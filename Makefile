# SRC := src
# OBJ := obj
# BIN := bin
# EXECUTABLE:= shell

# SRCS := $(wildcard $(SRC)/*.c)
# OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
# INCS := -Iinclude/
# DIRS := $(OBJ)/ $(BIN)/
# EXEC := $(BIN)/$(EXECUTABLE)

# CC := gcc
# CFLAGS := -g -Wall -std=c99 $(INCS)
# LDFLAGS :=

# all: $(EXEC)

# $(EXEC): $(OBJS)
# 	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# $(OBJ)/%.o: $(SRC)/%.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# run: $(EXEC)
# 	$(EXEC)

# clean:
# 	rm $(OBJ)/*.o $(EXEC)

# $(shell mkdir -p $(DIRS))

# .PHONY: run clean all


SRC := src
OBJ := obj
BIN := bin
EXECUTABLE := shell
MYTIMEOUT_EXECUTABLE := mytimeout

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
INCS := -Iinclude/
DIRS := $(OBJ)/ $(BIN)/
EXEC := $(BIN)/$(EXECUTABLE)
MYTIMEOUT_EXEC := $(BIN)/$(MYTIMEOUT_EXECUTABLE)

CC := gcc
CFLAGS := -g -Wall -std=c99 $(INCS)
LDFLAGS :=

all: $(EXEC) $(MYTIMEOUT_EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

$(MYTIMEOUT_EXEC): $(OBJ)/mytimeout.o $(OBJ)/lexer.o
	$(CC) $(CFLAGS) $^ -o $(MYTIMEOUT_EXEC)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(EXEC)
	$(EXEC)

clean:
	rm -f $(OBJ)/*.o $(EXEC) $(MYTIMEOUT_EXEC)

$(shell mkdir -p $(DIRS))

.PHONY: run clean all

