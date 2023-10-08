#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    char ** items;
    size_t size;
} tokenlist;

char * environmentVariables(tokenlist * tokens);
char * tildeExpansion(tokenlist * tokens);
char * pathSearch(tokenlist * tokens);
char * ExternalCommandExec(tokenlist * tokens, char * filePath);
void ioRedirection(tokenlist * tokens);
char *  piping(tokenlist * tokens);
char * internalCommandExecution(tokenlist * tokens);
void printList(tokenlist * tokens); 
void prompt();
void lexer_parse_token(); 
void historyCommandList(tokenlist * tokens, char * input);
bool hasPipe(tokenlist * tokens);
bool hasRedirector(tokenlist * tokens);
void executeAllCommands(tokenlist * tokens, char * input);
char* commandLine(tokenlist* tokens);
void checkBackgroundProcesses();

void BackgroundProcess(tokenlist * tokens, int JOB_NUMBER, bool has_pipe, bool has_redirector);
void storeBackgroundProcessInfo(int jobNumber, pid_t pid, const char *commandLine);
void listActiveBackgroundProcesses();



char * get_input(void);
tokenlist * get_tokens(char *input);
tokenlist * new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);