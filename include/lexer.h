#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char ** items;
    size_t size;
} tokenlist;

void environmentVariables(tokenlist * tokens);
void tildeExpansion(tokenlist * tokens);
// void printList(tokenlist * tokens); 
void prompt();
void lexer_parse_token(); 

char * get_input(void);
tokenlist * get_tokens(char *input);
tokenlist * new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
