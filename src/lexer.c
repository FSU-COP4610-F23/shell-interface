#include "lexer.h"
#include "mytimeout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/stat.h>

#define MAX_HISTORY_SIZE 3
#define MAX_BACKGROUND_PROCESSES 10
int JOB_NUMBER = 0;

struct commandHistory {
    char commands[MAX_HISTORY_SIZE][200]; // Assuming commands are less than 200 characters
    int count;
};

typedef struct {
    int jobNumber;
    pid_t pid;
    char commandLine[256];
} backgroundProcess;

struct commandHistory history;

// Define a global array to store background process information
backgroundProcess backgroundProcesses[MAX_BACKGROUND_PROCESSES];
int numBackgroundProcesses = 0;

int main()
{
	history.count = 0;
	lexer_parse_token();

	return 0;
}

void lexer_parse_token()
{
	while (1)
	{
		// print out prompt
		printf("\n");
		prompt();
		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */
		char * input = get_input();
		tokenlist * tokens = get_tokens(input);

		executeAllCommands(tokens, input);
		free(input);
		free_tokens(tokens);
	}

}

void executeAllCommands(tokenlist * tokens, char * input)
{

	if (tokens->size > 0) 
	{
		historyCommandList(tokens, input);


		//check if last token is &
		if (strcmp(tokens->items[tokens->size - 1], "&") == 0) {
			//background commands
			tokenlist * tokens2 = {NULL};
			tokens2 = new_tokenlist();
			for (int i = 0; i < tokens->size - 1; i++) //removes & as a token
			{
				add_token(tokens2, tokens->items[i]);
			}
			JOB_NUMBER++;
			BackgroundProcess(tokens2, JOB_NUMBER, hasPipe(tokens), hasRedirector(tokens));
		}
		else if (hasPipe(tokens)) 
		{
			piping(tokens);
		}
		else if (hasRedirector(tokens))
		{
			ioRedirection(tokens);
		}
		else 
		{
			if (strcmp(tokens->items[0],"exit") == 0 || strcmp(tokens->items[0],"cd") == 0 || strcmp(tokens->items[0],"jobs") == 0 )
			{
				// running inernal command execution
				internalCommandExecution(tokens);			
			}
			
			else if (strcmp(tokens->items[0], "./mytimeout") == 0)
			{
				int timeout_seconds = 5; // Example timeout value
				if (tokens->size >= 2) 
				{
        			timeout_seconds = atoi(tokens->items[1]); // Use the specified timeout value
    			}

				int result = execute_with_timeout(tokens, timeout_seconds); // Use the timeout function

				if (result == 0) {
					printf("Command executed successfully.\n");
				} else {
					printf("Command execution failed.\n");
				}

				free_tokens(tokens); // Free the tokenlist when done

			}
			else
			{
				tokens->items[1] = environmentVariables(tokens); // what happens to previous memory in token->items[1]?
				tokens->items[1] = tildeExpansion(tokens);
				tokens->items[0] = pathSearch(tokens);
				ExternalCommandExec(tokens, tokens->items[0]);
			}
		}
	}
	// print tokens
	// printList(tokens);

}

void historyCommandList(tokenlist * tokens, char * input )
{
	// Exclude the "exit" command from being added to history
	if (strcmp(tokens->items[0], "exit") != 0) 
	{
		// Add the command to the history
		if (history.count < MAX_HISTORY_SIZE) 
		{
			strcpy(history.commands[history.count], input);
			history.count++;
		} 
		else 
		{
			// Shift existing commands to make space for the new one
			for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) 
			{
				strcpy(history.commands[i], history.commands[i + 1]);
			}
			strcpy(history.commands[MAX_HISTORY_SIZE - 1], input);
		}
	}
}

bool hasPipe(tokenlist * tokens)
{
	// Check if the input contains a pipe character
	// int has_pipe = 0;
	bool hasPipe = false;
	for (size_t i = 0; i < tokens->size; i++) 
	{
		if (strcmp(tokens->items[i], "|") == 0) 
		{
			// has_pipe = 1;
			hasPipe = true;
			return hasPipe;
			break;
		}
	}
	return false; 
}

bool hasRedirector(tokenlist * tokens)
{
	// Check if the input contains a pipe character
	// int has_pipe = 0;
	bool hasRedirector = false;
	for (size_t i = 0; i < tokens->size; i++) 
	{
		if (strcmp(tokens->items[i], "|") == 0) 
		{
			// has_pipe = 1;
			hasRedirector = true;
			return hasRedirector;
			break;
		}
	}
	return false; 
}

void printList(tokenlist * tokens)
{
	// print tokens
	for (int i = 0; i < tokens->size; i++)
	{
		printf("token %d: (%s)\n", i, tokens->items[i]);
	}
	// print output
	for (int i = 0; i < tokens->size; i++)
	{
		printf("%s ", tokens->items[i]);
	}

}

void prompt()
{
	printf("%s@%s:%s>", getenv("USER"), getenv("MACHINE"), getenv("PWD"));
}

char * environmentVariables(tokenlist *tokens)
{
	// get environmental variable
	for (int i = 0; i < tokens->size; i++)
	{
		if (tokens->items[i][0] == '$')
		{
			char tokenItems[20] = "";
			strcat(tokenItems, &tokens->items[i][1]); // put each character into tokenItems
			char * expand = malloc(sizeof(char) * strlen(getenv(tokenItems)));
			strcpy(expand, getenv(tokenItems));
			// strcpy(tokens->items[i], getenv(tokenItems)); // tokens->items[i] = getenv(tokenItems)
			// free_tokens(tokens);
			return expand;
		}
	}

	return tokens->items[1];
}

char * tildeExpansion(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
	{
		if (tokens->items[i][0] == '~')
		{
			char tokenItems[50] = "";
			if (tokens->items[i][1] == '/')
			{

				strcat(tokenItems, &tokens->items[i][1]);
			}
			else
			{
				char * expand = malloc(sizeof(char) * strlen(getenv("HOME")));
				strcpy(expand, getenv("HOME"));
				return expand;
			}

			char * expand2= malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen(tokenItems)) + 1);
			strcpy(expand2, getenv("HOME"));
			strcat(expand2, tokenItems);
			return expand2;
		}
	}
	return tokens->items[1];
}

char * pathSearch(tokenlist * tokens)
{
   char * fullPath = malloc(sizeof(char) * strlen(getenv("PATH")));
   strcpy(fullPath, getenv("PATH"));
   const char s[2] = ":";
   char * token;
   char * filePath;
   /* get the first token */
   token = strtok(fullPath, s);
   bool check = false;
   /* walk through other tokens */
   while( token != NULL ) 
   {

		if (!check) 
		{
			char * tempFilePath = malloc(sizeof(char) * strlen(token) + strlen(tokens->items[0]) + 2);
			//save strlen(token)
			strcpy(tempFilePath, token);
			strcat(tempFilePath, "/");
			strcat(tempFilePath, tokens->items [0]); //incecrement ls into first token
			if (fopen(tempFilePath, "r") != NULL)
			{
				check = true;
				filePath = tempFilePath; //filePath ptr points to tempFilePath memory address
			}
			else {
				free(tempFilePath);
			}
		}
		token = strtok(NULL, s);
   }

	if (!check)
	{
		printf("Command not found\n");
		return 0; 
	}
	else
		return filePath; 
}

void ioRedirection(tokenlist *tokens)
{
    int input_redirection = -1;  // File descriptor for input redirection
    int output_redirection = -1; // File descriptor for output redirection

    // Check for input redirection
    for (int i = 0; i < tokens->size; i++)
    {
        if (strcmp(tokens->items[i], "<") == 0)
        {
            input_redirection = i; // Store the index of the "<" token
            break;
        }
    }

    // Check for output redirection
    for (int i = 0; i < tokens->size; i++)
    {
        if (strcmp(tokens->items[i], ">") == 0)
        {
            output_redirection = i; // Store the index of the ">" token
            break;
        }
    }

    if (input_redirection != -1)
    {
        // Handle input redirection
        if (input_redirection + 1 < tokens->size)
        {
            char *input_file = tokens->items[input_redirection + 1];
            // Open the input file for reading and replace standard input
			close(0);
            int fd = open(input_file, O_RDONLY);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            // dup2(fd, STDIN_FILENO);
            // close(fd);

            // Remove the redirection tokens and the input file from the token list
            for (int i = input_redirection; i < tokens->size - 2; i++)
            {
                tokens->items[i] = tokens->items[i + 2];
            }
            tokens->size -= 2;
        }
        else
        {
            fprintf(stderr, "Error: Missing input file for redirection.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (output_redirection != -1)
    {
        // Handle output redirection
        if (output_redirection + 1 < tokens->size)
        {
            char *output_file = tokens->items[output_redirection + 1];
            // Open the output file for writing and replace standard output'
			close(1);
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);

            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            // dup2(fd, STDOUT_FILENO);
            // close(fd);

            // Remove the redirection tokens and the output file from the token list
            for (int i = output_redirection; i < tokens->size - 2; i++)
            {
                tokens->items[i] = tokens->items[i + 2];
            }
            tokens->size -= 2;
        }
        else
        {
            fprintf(stderr, "Error: Missing output file for redirection.\n");
            exit(EXIT_FAILURE);
        }
    }
}

char * ExternalCommandExec(tokenlist * tokens, char * filePath)
{
	pid_t pid;
	int status;
	pid = fork();
	
	if (pid == 0) // do only if child process
	{
		int has_redirector = 0;
		for (size_t i = 0; i < tokens->size; i++) 
		{
			if (strcmp(tokens->items[i], "<") == 0 || strcmp(tokens->items[i], ">") == 0) 
			{
				has_redirector = 1;
				break;
			}
		}
		if (has_redirector) 
		{
			// Execute piped commands
			ioRedirection(tokens);
		}

		// have to call redirection in here
		char *argv[tokens->size + 1];
		for (int i = 0; i < tokens->size; i++)
		{
			argv[i] = tokens->items[i];
		}
		argv[tokens->size] = NULL;
		execv(filePath, argv);
	}

	waitpid(pid, &status, 0); //wait for child process to finish	

	return 0;
}


char * piping(tokenlist *tokens) 
{
    size_t numTokens = tokens->size;

    // Find the positions of the pipe tokens ("|")
    size_t pipePositions[2] = {0};
    int pipeCount = 0; // Use this becasue there could be one or two pipes
    for (size_t i = 0; i < numTokens; i++) 
	{
        if (strcmp(tokens->items[i], "|") == 0) 
		{
            pipePositions[pipeCount] = i;
            pipeCount++;
        }
    }
	// pipe has to be more than one. Pipe cannot be located as the first character, and cannot be last character
    if (pipeCount < 1 || pipePositions[0] == 0 || pipePositions[pipeCount - 1] == numTokens - 1) 
	{
        fprintf(stderr, "Invalid input: Missing command(s)\n");
		return 0; 
    }

    // Create an array of tokenlists for commands (There can be two and three commands)
    tokenlist * commands[3] = {NULL};
    for (int i = 0; i <= pipeCount; i++)  // based on the number of new tokens
	{
        commands[i] = new_tokenlist();
    }

    // Populate the tokenlists for commands
    size_t start = 0; // 
    for (int i = 0; i <= pipeCount; i++) 
	{
        size_t end;
		if (i == pipeCount)
		{
			end	= numTokens;
		}
		else
		{
			end = pipePositions[i];
		}
        for (size_t j = start; j < end; j++) 
		{
            add_token(commands[i], tokens->items[j]);
        }
        start = end + 1;
    }

	// pipr file decriptors initalization. 
    int pipefds[2][2]; 
    pid_t pids[3]; 

    // Create pipes
    for (int i = 0; i < pipeCount; i++) 
	{
        if (pipe(pipefds[i]) == -1) 
		{
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i <= pipeCount; i++) 
	{
        pids[i] = fork();
        if (pids[i] == -1) 
		{
            perror("fork");
            exit(EXIT_FAILURE);
     	}

        if (pids[i] == 0) 
		{
            // Child process
            if (i > 0) 
			{
                // Redirect standard input from the previous pipe
                dup2(pipefds[i - 1][0], STDIN_FILENO);
                close(pipefds[i - 1][0]);
            }

            if (i < pipeCount) 
			{
                // Redirect standard output to the current pipe
                dup2(pipefds[i][1], STDOUT_FILENO);
                close(pipefds[i][0]);
            }

            // Close all pipe file descriptors
            for (int j = 0; j < pipeCount; j++) 
			{
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            // Execute the command
            char * commandPath = pathSearch(commands[i]);
            if (commandPath) 
			{
                ExternalCommandExec(commands[i], commandPath);
                free(commandPath);
            } 
			else 
			{
                fprintf(stderr, "Command not found: %s\n", commands[i]->items[0]);
                exit(EXIT_FAILURE);
            }

            // Child process should exit after executing the command
            exit(EXIT_SUCCESS);
        }
    }
	
	/**
	 * 
	 * Close all pipe file descriptors
	*/
	// Parent process
    for (int j = 0; j < pipeCount; j++) 
	{
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }
    // Wait for all child processes to complete
    for (int i = 0; i <= pipeCount; i++) 
	{
        waitpid(pids[i], NULL, 0);
    }
    // Free tokenlists
    for (int i = 0; i <= pipeCount; i++) 
	{
        free_tokens(commands[i]);
    }

	return 0; 
}

char * internalCommandExecution(tokenlist * tokens)
{
	if (strcmp(tokens->items[0], "exit") == 0)
	{
		// creare a fuunction that lists active backgroun processes
		// Display the last three valid commands
        
		int start; 

		if (history.count > 3)
		{
			start = history.count - 3; // if we pass three commands then start from 1 again
		}
		else if(history.count < 3 && history.count > 0)
		{
			printf("Last valid commands:\n");
			start = history.count - 1;
		}
		else if(history.count == 3)
		{
			printf("Last three valid commands:\n");
			start = 0;  //start is 0
		}
		else
		{
			printf("There are no valid commands\n");
		}
        for (int i = start; i < history.count; i++) 
		{
            printf("%s\n", history.commands[i]); 
        }

		exit(0);
		printf("User wants to exit");
	}

	else if(strcmp(tokens->items[0], "cd") == 0)
	{
		// if there are no arguments supplied
		if (tokens->size == 1 )
		{	
			tokens->items[1] = getenv("HOME");
			add_token(tokens, tokens->items[1]);
			chdir(tokens->items[1]);
		}
		else if (tokens->size > 2) //if there are more than one argumenr
		{
			printf("Error: More than one argument is present\n");
		}
		else if (access(tokens->items[1], F_OK)==-1)
		{
			printf("Error: target does not exist\n");
		}
		chdir(tokens->items[1]);
	}
	else if (strcmp(tokens->items[0], "jobs") == 0)
	{
		printf("Output a list of active processes\n");
		listActiveBackgroundProcesses();
	}

	return tokens->items[1]; 
}

// void BackgroundProcess(tokenlist* tokens, int JOB_NUMBER, bool has_pipe, bool has_redirector)
// {
// 	if (has_pipe) {
// 		int status;
// 		pid_t PID = fork();
// 		if (PID == 0)
// 		{
// 			printf("We go in here\n");
// 			printf("\n[ %d ] [ %d ]\n", JOB_NUMBER, getpid()); // we want it print this out and go back to prompt while it runs in background
// 			piping(tokens);
// 			printf("[ %d ] + done [ ", JOB_NUMBER);
// 			// printList(tokens);
// 			printf("]\n");
// 			exit(0);
// 		}
// 		waitpid(PID, &status, WNOHANG);
// 	}
// 	else if (has_redirector) // this i sused in t
// 	{
// 		int status;
// 		pid_t PID = fork();
// 		if (PID != 0)
// 		{
// 			printf("[%d] [%d]\n", JOB_NUMBER, PID);
// 			char* cmdLine = commandLine(tokens);
//             storeBackgroundProcessInfo(JOB_NUMBER, PID, cmdLine);
//             free(cmdLine);
// 			piping(tokens);
// 			printf("[ %d ] + done [ ", JOB_NUMBER);
// 			// printList(tokens);
// 			printf("]\n");
// 			exit(0);
// 		}
// 		waitpid(PID, &status, WNOHANG);
// 	}
// 	else {
// 		int status;
// 		pid_t PID = fork();
		
// 		char* cmdLine = commandLine(tokens);
//         storeBackgroundProcessInfo(JOB_NUMBER, getpid(), cmdLine);
//         free(cmdLine);

// 		if (PID == 0) // do only if child process
// 		{
// 			printf("\n[ %d ] [ %d ]\n", JOB_NUMBER, getpid());
// 			int status2;
// 			pid_t pid2 = fork();
// 			if (pid2 == 0) //child^2		
// 			{	
// 				char *argv[tokens->size + 1];
// 				for (int i = 0; i < tokens->size; i++)
// 				{
// 					argv[i] = tokens->items[i];
// 				}
// 				argv[tokens->size] = NULL;

// 				storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
// 				execv(pathSearch(tokens), argv);
// 			}
// 			waitpid(pid2, &status2, 0);
// 			if (WIFEXITED(status2)) {
// 				printf("[ %d ] + done [ ", JOB_NUMBER);
// 				// printList(tokens);
// 				printf("]\n");
// 			}
// 		}
// 		waitpid(PID, &status, WNOHANG); //don't wait for child process to finish
// 	}
// }


void BackgroundProcess(tokenlist* tokens, int JOB_NUMBER, bool has_pipe, bool has_redirector)
{
    if (has_pipe) 
	{
        // int status;
        pid_t PID = fork();
        if (PID == 0) {
            piping(tokens);
            exit(0);
        }

		storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
		free(commandLine(tokens));

        printf("[ %d ] [ %d ]\n", JOB_NUMBER, PID);

    }
    else if (has_redirector) {
        // int status;
        pid_t PID = fork();
        if (PID == 0) {
            // char * cmdLine = commandLine(tokens);
            // storeBackgroundProcessInfo(JOB_NUMBER, PID, cmdLine);
            // free(cmdLine);
            // piping(tokens);
			// storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
			// free(commandLine(tokens));
            // printf("[%d] [%d]\n", JOB_NUMBER, PID);
            // waitpid(PID, &status, 0);
            // printf("[ %d ] + done %s\n", JOB_NUMBER, cmdLine);
			ioRedirection(tokens);
            exit(0);
        }
		storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
		free(commandLine(tokens));

        printf("[ %d ] [ %d ]\n", JOB_NUMBER, PID);
    }
    else {
        // int status;
        pid_t PID = fork();

        // storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
        // free(commandLine(tokens));

        if (PID == 0) 
		{
			
            int status2;
            pid_t pid2 = fork();
            if (pid2 == 0) {
                char* argv[tokens->size + 1];
                for (int i = 0; i < tokens->size; i++) {
                    argv[i] = tokens->items[i];
                }
                argv[tokens->size] = NULL;

                execv(pathSearch(tokens), argv);

            }

			
            waitpid(pid2, &status2, 0);
			// pid_t result = waitpid(pid2, &status, WNOHANG);
			// printf("\nThis is result: %d\n", result);

            if (WIFEXITED(status2)) 
			{
                printf("\n[ %d ] + done %s\n", JOB_NUMBER, commandLine(tokens));
            }
			exit(0);
        }

		

        printf("[ %d ] [ %d ]\n", JOB_NUMBER, getpid());

		storeBackgroundProcessInfo(JOB_NUMBER, getpid(), commandLine(tokens));
		free(commandLine(tokens));
		// waitpid(PID, &status, 0);
    }

    // Return to the prompt without waiting for child processes.
}

// Function to store the PID and command line of a background process
void storeBackgroundProcessInfo(int jobNumber, pid_t pid, const char *commandLine)
{
	if (numBackgroundProcesses < MAX_BACKGROUND_PROCESSES) {
        backgroundProcesses[numBackgroundProcesses].jobNumber = jobNumber;
        backgroundProcesses[numBackgroundProcesses].pid = pid;
        strncpy(backgroundProcesses[numBackgroundProcesses].commandLine, commandLine, sizeof(backgroundProcesses[numBackgroundProcesses].commandLine));
        numBackgroundProcesses++;
    } else {
        fprintf(stderr, "Maximum number of background processes reached.\n");
    }
}

// Function to list active background processes
void listActiveBackgroundProcesses() {
    if (numBackgroundProcesses == 0) {
        printf("No active background processes.\n");
    } else {
        printf("Active background processes:\n");
        for (int i = 0; i < numBackgroundProcesses; i++) {
            printf("[%d]+ [%d] running %s\n", backgroundProcesses[i].jobNumber, backgroundProcesses[i].pid, backgroundProcesses[i].commandLine);
        }
    }
}

// Function to concatenate elements of the tokenlist into a single string
char* commandLine(tokenlist* tokens) 
{
    int totalLength = 0;
    for (int i = 0; i < tokens->size; i++) {
        totalLength += strlen(tokens->items[i]) + 1; // +1 for space or null terminator
    }

    char* cmdline = (char*)malloc(totalLength);
    if (cmdline == NULL) {
        perror("malloc");
        exit(1);
    }

    strcpy(cmdline, tokens->items[0]);
    for (int i = 1; i < tokens->size; i++) {
        strcat(cmdline, " ");
        strcat(cmdline, tokens->items[i]);
    }

    return cmdline;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;
	char line[5];
	while (fgets(line, 5, stdin) != NULL)
	{
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;
		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;
		if (newln != NULL)
			break;
	}
	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;
	return buffer;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input);
	tokenlist *tokens = new_tokenlist();
	char *tok = strtok(buf, " ");
	while (tok != NULL)
	{
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}
	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
