# SRC := src
# OBJ := obj
# BIN := bin
# EXECUTABLE := shell
# MYTIMEOUT_EXECUTABLE := mytimeout

# SRCS := $(wildcard $(SRC)/*.c)
# OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
# INCS := -Iinclude/
# DIRS := $(OBJ)/ $(BIN)/
# EXEC := $(BIN)/$(EXECUTABLE)
# MYTIMEOUT_EXEC := $(BIN)/$(MYTIMEOUT_EXECUTABLE)

# CC := gcc
# CFLAGS := -g -Wall -std=c99 $(INCS)
# LDFLAGS :=

# all: $(EXEC) $(MYTIMEOUT_EXEC)

# $(EXEC): $(OBJS)
# 	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# $(MYTIMEOUT_EXEC): $(OBJ)/mytimeout/mytimeout.o $(OBJ)/lexer.o
# 	$(CC) $(CFLAGS) $^ -o $(MYTIMEOUT_EXEC)

# $(OBJ)/%.o: $(SRC)/%.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# run: $(EXEC)
# 	$(EXEC)

# clean:
# 	rm -f $(OBJ)/*.o $(EXEC) $(MYTIMEOUT_EXEC)

# $(shell mkdir -p $(DIRS))

# .PHONY: run clean all





# SRC := src
# OBJ := obj
# BIN := bin
# SHELL_EXECUTABLE := shell
# MYTIMEOUT_EXECUTABLE := mytimeout

# SRCS := $(wildcard $(SRC)/*.c)
# OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
# INCS := -Iinclude/

# CC := gcc
# CFLAGS := -g -Wall -std=c99 $(INCS)

# all: $(BIN)/$(SHELL_EXECUTABLE) $(BIN)/$(MYTIMEOUT_EXECUTABLE)

# $(BIN)/$(SHELL_EXECUTABLE): $(OBJ)/lexer.o
# 	$(CC) $(CFLAGS) $^ -o $@

# $(BIN)/$(MYTIMEOUT_EXECUTABLE): $(OBJ)/mytimeout/mytimeout.o
# 	$(CC) $(CFLAGS) $^ -o $@

# $(OBJ)/mytimeout/mytimeout.o: $(SRC)/mytimeout/mytimeout.c
# 	@mkdir -p $(OBJ)/mytimeout
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(OBJ)/lexer.o: $(SRC)/lexer.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# run: $(BIN)/$(SHELL_EXECUTABLE)
# 	$<

# # run-mytimeout: $(BIN)/$(MYTIMEOUT_EXECUTABLE)
# # 	$<

# clean:
# 	rm -f $(OBJ)/*.o $(BIN)/$(SHELL_EXECUTABLE) $(BIN)/$(MYTIMEOUT_EXECUTABLE)

# .PHONY: run-shell run-mytimeout clean all


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

all: $(BIN)/$(SHELL_EXECUTABLE) $(MYTIMEOUT_EXECUTABLE)

$(BIN)/$(SHELL_EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(MYTIMEOUT_EXECUTABLE): $(OBJ)/mytimeout/mytimeout.o
	$(CC) $(CFLAGS) $^ -o $@


$(OBJ)/mytimeout/mytimeout.o: $(SRC)/mytimeout/mytimeout.c
	@mkdir -p $(OBJ)/mytimeout
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)/$(SHELL_EXECUTABLE)
	$<

clean:
	rm -f $(OBJ)/*.o $(BIN)/$(SHELL_EXECUTABLE) $(MYTIMEOUT_EXECUTABLE)  # Remove from root directory

.PHONY: run clean all
