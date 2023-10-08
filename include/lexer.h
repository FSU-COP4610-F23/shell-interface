#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>


#define MAX_HISTORY_SIZE 3
#define MAX_BACKGROUND_PROCESSES 10

typedef struct {
    char ** items;
    size_t size;
} tokenlist;

struct commandHistory {
    char commands[MAX_HISTORY_SIZE][200]; // Assuming commands are less than 200 characters
    int count;
};

typedef struct  {
    int jobNumber;
    pid_t pid;
    char commandLine[256];
	int status;
} backgroundProcess;

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
void updateBackgroundProcessStatus();


char * get_input(void);
tokenlist * get_tokens(char *input);
tokenlist * new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);