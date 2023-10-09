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

int JOB_NUMBER = 0;

struct commandHistory history;

// Define a global array to store background process information
backgroundProcess backgroundProcesses[MAX_BACKGROUND_PROCESSES];
int activeBackgroundProcesses = 0;

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
		char *input = get_input();
		tokenlist *tokens = get_tokens(input);

		executeAllCommands(tokens, input);
		free(input);
		free_tokens(tokens);
	}
}

void executeAllCommands(tokenlist *tokens, char *input)
{

	if (tokens->size > 0)
	{
		historyCommandList(tokens, input);

		// check if last token is &
		if (strcmp(tokens->items[tokens->size - 1], "&") == 0)
		{
			// background commands
			tokenlist *tokens2 = {NULL};
			tokens2 = new_tokenlist();
			for (int i = 0; i < tokens->size - 1; i++) // removes & as a token
			{
				add_token(tokens2, tokens->items[i]);
			}
			JOB_NUMBER++;
			BackgroundProcess(tokens2, JOB_NUMBER, 
			hasPipe(tokens), 
			hasRedirector(tokens));
		}
		else if (hasPipe(tokens))
		{
			piping(tokens);
		}
		else
		{
			if (strcmp(tokens->items[0], "exit") == 0 
			|| strcmp(tokens->items[0], "cd") == 0 
			|| strcmp(tokens->items[0], "jobs") == 0)
			{
				// running inernal command execution
				internalCommandExecution(tokens);
			}
			else
			{
				tokens->items[1] = environmentVariables(tokens);
				tokens->items[1] = tildeExpansion(tokens);
				tokens->items[0] = pathSearch(tokens);
				ExternalCommandExec(tokens, tokens->items[0]);
			}
		}
	}
	// print tokens
	// printList(tokens);
}

void historyCommandList(tokenlist *tokens, char *input)
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

bool hasPipe(tokenlist *tokens)
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

bool hasRedirector(tokenlist *tokens)
{
	// Check if the input contains a pipe character
	// int has_pipe = 0;
	bool hasRedirector = false;
	for (size_t i = 0; i < tokens->size; i++)
	{
		if (strcmp(tokens->items[i], "<") == 0 
		   || strcmp(tokens->items[i], ">") == 0)
		{
			// has_pipe = 1;
			hasRedirector = true;
			return hasRedirector;
		}
	}
	return false;
}

void printList(tokenlist *tokens)
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

char *environmentVariables(tokenlist *tokens)
{
	// get environmental variable
	for (int i = 0; i < tokens->size; i++)
	{
		if (tokens->items[i][0] == '$')
		{
			 // put each character into tokenItems
			char tokenItems[20] = "";
			strcat(tokenItems, &tokens->items[i][1]);
			char *expand = malloc(sizeof(char) * strlen(getenv(tokenItems)));
			strcpy(expand, getenv(tokenItems));

			return expand;
		}
	}

	return tokens->items[1];
}

char *tildeExpansion(tokenlist *tokens)
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
				char *expand = malloc(sizeof(char) * strlen(getenv("HOME")));
				strcpy(expand, getenv("HOME"));
				return expand;
			}

			char *expand2 = malloc(sizeof(char) * (strlen(getenv("HOME"))
			                    + strlen(tokenItems)) + 1);
			strcpy(expand2, getenv("HOME"));
			strcat(expand2, tokenItems);
			return expand2;
		}
	}
	return tokens->items[1];
}

char *pathSearch(tokenlist *tokens)
{
	char *fullPath = malloc(sizeof(char) * strlen(getenv("PATH")));
	strcpy(fullPath, getenv("PATH"));
	const char s[2] = ":";
	char *token;
	char *filePath;
	/* get the first token */
	token = strtok(fullPath, s);
	bool check = false;
	/* walk through other tokens */
	while (token != NULL)
	{

		if (!check)
		{
			char *tempFilePath = malloc(sizeof(char) * strlen(token) + 
			                  strlen(tokens->items[0]) + 2);
			// save strlen(token)
			strcpy(tempFilePath, token);
			strcat(tempFilePath, "/");
			strcat(tempFilePath, tokens->items[0]); // incecrement
			if (fopen(tempFilePath, "r") != NULL)
			{
				check = true;
				filePath = tempFilePath; 
			}
			else
			{
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
	int input_redirection = -1;	
	int output_redirection = -1; 

	int tokenSize = tokens->size;
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
		if (input_redirection + 1 < tokenSize)
		{
			char *input_file = tokens->items[input_redirection + 1];

			if (access(input_file, F_OK) == -1)
			{
				int fd = open(input_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if (fd == -1)
				{
					perror("open");
					exit(EXIT_FAILURE);
				}
				close(fd); 
			}

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

			// Remove the redirection tokens and the input file
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
		if (output_redirection + 1 < tokenSize)
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

			// Remove the redirection tokens 
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

char *ExternalCommandExec(tokenlist *tokens, char *filePath)
{
	pid_t pid;
	int status;
	pid = fork();

	if (pid == 0) // do only if child process
	{
		int has_redirector = 0;
		for (size_t i = 0; i < tokens->size; i++)
		{
			if (strcmp(tokens->items[i], "<") == 0 
			|| strcmp(tokens->items[i], ">") == 0)
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

	waitpid(pid, &status, 0); // wait for child process to finish

	return 0;
}

char *piping(tokenlist *tokens)
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
	// pipe has to be more than one.
	if (pipeCount < 1 || pipePositions[0] == 0 
	  || pipePositions[pipeCount - 1] == numTokens - 1)
	{
		fprintf(stderr, "Invalid input: Missing command(s)\n");
		return 0;
	}

	// Create an array of tokenlists for commands
	tokenlist *commands[3] = {NULL};
	for (int i = 0; i <= pipeCount; i++) // based on the number of new tokens
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
			end = numTokens;
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
			char *commandPath = pathSearch(commands[i]);
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

char *internalCommandExecution(tokenlist *tokens)
{
	if (strcmp(tokens->items[0], "exit") == 0)
	{
		int start;

		if (history.count > 3)
		{
			start = history.count - 3; 
		}
		else if (history.count < 3 && history.count > 0)
		{
			printf("Last valid commands:\n");
			start = history.count - 1;
		}
		else if (history.count == 3)
		{
			printf("Last three valid commands:\n");
			start = 0; // start is 0
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

	else if (strcmp(tokens->items[0], "cd") == 0)
	{
		// if there are no arguments supplied
		if (tokens->size == 1)
		{
			tokens->items[1] = getenv("HOME");
			add_token(tokens, tokens->items[1]);
			chdir(tokens->items[1]);
		}
		else if (strcmp(tokens->items[1], "~") == 0)
		{
			tokens->items[1] = tildeExpansion(tokens);
			chdir(tokens->items[1]);
		}
		else if (tokens->size > 2) // if is are more than one argument
		{
			printf("Error: More than one argument is present\n");
		}
		else if (access(tokens->items[1], F_OK) == -1)
		{
			printf("Error: target does not exist\n");
		}
		else
		{
			chdir(tokens->items[1]);
		}
	}
	else if (strcmp(tokens->items[0], "jobs") == 0)
	{
		listActiveBackgroundProcesses();
	}

	return tokens->items[1];
}

void BackgroundProcess(tokenlist *tokens, 
                       int JOB_NUMBER, 
                       bool has_pipe, 
	                     bool has_redirector)
{
	if (has_pipe)
	{
		// int status;
		pid_t PID = fork();
		if (PID == 0)
		{
			printf("\n[ %d ] [ %d ]\n", JOB_NUMBER, getpid());		
			piping(tokens);
			printf("\n[ %d ] + done %s\n", JOB_NUMBER, commandLine(tokens));
			exit(0);
		}

		storeBackgroundProcessInfo(JOB_NUMBER, PID, commandLine(tokens));
		free(commandLine(tokens));

		
	}
	else if (has_redirector)
	{
		// int status;
		pid_t PID = fork();
		if (PID == 0)
		{
			printf("\n[ %d ] [ %d ]\n", JOB_NUMBER, getpid());
			sleep(10);
			pid_t pid2 = fork();
			if (pid2 == 0) {
				ioRedirection(tokens);
				char *argv[tokens->size + 1];
				for (int i = 0; i < tokens->size; i++)
				{
					argv[i] = tokens->items[i];
				}
				argv[tokens->size] = NULL;
				execv(pathSearch(tokens), argv);
				}
			printf("\n[ %d ] + done %s\n", JOB_NUMBER, commandLine(tokens));
			exit(0);
		}
		storeBackgroundProcessInfo(JOB_NUMBER, PID, commandLine(tokens));		
		free(commandLine(tokens));
	}
	else
	{
		// int status;
		pid_t PID = fork();

		if (PID == 0)
		{
			printf("\n[ %d ] [ %d ]\n", JOB_NUMBER, getpid());
			int status2;
			pid_t pid2 = fork();
			if (pid2 == 0)
			{
				char *argv[tokens->size + 1];
				for (int i = 0; i < tokens->size; i++)
				{
					argv[i] = tokens->items[i];
				}
				argv[tokens->size] = NULL;

				execv(pathSearch(tokens), argv);
			}

			waitpid(pid2, &status2, 0);

			if (WIFEXITED(status2))
			{
				printf("\n[ %d ] + done %s\n", JOB_NUMBER, commandLine(tokens));
			}
			exit(0);
		}
		storeBackgroundProcessInfo(JOB_NUMBER, PID, commandLine(tokens));
		free(commandLine(tokens));

	}

}

// Function to store the PID and command line 
void storeBackgroundProcessInfo(int jobNumber, 
                                pid_t pid, 
                                const char *commandLine)
{
	if (activeBackgroundProcesses < MAX_BACKGROUND_PROCESSES)
	{
		backgroundProcesses[JOB_NUMBER - 1].jobNumber = jobNumber;
		backgroundProcesses[JOB_NUMBER - 1].pid = pid;
		strncpy(backgroundProcesses[JOB_NUMBER - 1].commandLine, 
		          commandLine, 
		         sizeof(backgroundProcesses[JOB_NUMBER - 1].commandLine));
		 // Initially set as running
		backgroundProcesses[JOB_NUMBER - 1].status = 1;
		activeBackgroundProcesses++;
	}
	else
	{
		fprintf(stderr, 
		"Maximum number of active background processes reached.\n");
	}
}

// Function to update the status of background processes
void updateBackgroundProcessStatus()
{
	int status;
	pid_t pid;

	for (int i = 0; i < JOB_NUMBER; i++)
	{
		pid = waitpid(backgroundProcesses[i].pid, &status, WNOHANG);
		if (pid == -1 && backgroundProcesses[i].status != 0)
		{
			// Error handling if waitpid fails
			perror("waitpid");
		}
		else if (pid == 0)
		{
			// Process is still running
			backgroundProcesses[i].status = 1; 
		}
		else
		{
			// Process has completed
			backgroundProcesses[i].status = 0;
			activeBackgroundProcesses--;
		}
	}
}

// Function to list active background processes
void listActiveBackgroundProcesses()
{
	updateBackgroundProcessStatus();
	if (activeBackgroundProcesses == 0)
	{
		printf("No active background processes.\n");
	}
	else
	{
		printf("Active background processes:\n");
		for (int i = 0; i < JOB_NUMBER; i++)
		{
			if (backgroundProcesses[i].status == 1)
				printf("[%d] + [%d] running %s\n", 
				backgroundProcesses[i].jobNumber, 
				backgroundProcesses[i].pid, 
				backgroundProcesses[i].commandLine);
		}
	}
}

// Function to concatenate elements of the tokenlist 
char *commandLine(tokenlist *tokens)
{
	int totalLength = 0;
	for (int i = 0; i < tokens->size; i++)
	{
		 // +1 for space or null terminator
		totalLength += strlen(tokens->items[i]) + 1;
	}

	char *cmdline = (char *)malloc(totalLength);
	if (cmdline == NULL)
	{
		perror("malloc");
		exit(1);
	}

	strcpy(cmdline, tokens->items[0]);
	for (int i = 1; i < tokens->size; i++)
	{
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

	tokens->items = (char **)realloc(tokens->items,
									 (i + 2) * sizeof(char *));
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
