#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main()
{
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
		tokenlist * tokens = get_tokens(input);
		char *filePath;

		tokens->items[1] = environmentVariables(tokens); // what happens to previous memory in token->items[1]?
		tokens->items[1] = tildeExpansion(tokens);
		filePath = pathSearch(tokens);
		ExternalCommandExec(tokens, filePath);

		// printList(tokens);

		free(input);
		free_tokens(tokens);
	}
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

// https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm

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
   while( token != NULL ) {

	if (!check) {
		char * tempFilePath = malloc(sizeof(char) * strlen(token) + strlen(tokens->items[0]) + 2);
		//save strlen(token)
		strcpy(tempFilePath, token);
		strcat(tempFilePath, "/");
		strcat(tempFilePath, tokens->items[0]); //incecrement ls into first token
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

void ExternalCommandExec(const tokenlist * tokens, char * filePath)
{
	pid_t pid;
	int status;
	pid = fork();
	
	if (pid == 0) // do only if child process
	{
		char *argv[tokens->size + 1];
		for (int i = 0; i < tokens->size; i++)
		{
			argv[i] = tokens->items[i];
		}
		argv[tokens->size] = NULL;
		execv(filePath, argv);
	}

	waitpid(pid, &status, 0); //wait for child process to finish	

}

void ioRedirection(tokenlist *tokens)
{
	
	size_t num_tokens = tokens->size;
	size_t redirector_positions[2] = {0};
	int redirectorIn_count = 0;		// for <
	int redirectorOut_count = 0;	// for >
	int redirector_count = 0;

	for(size_t i = 0; i< num_tokens; i++)
	{
		if(strcmp(tokens->items[i], ">") == 0)
		{
			redirector_positions[redirector_count] = i;
			redirector_count++;
			redirectorOut_count++;

			/*
			redirector_positions[redirectorIn_count] = i;
			redirectorIn_count++;
			*/
		}
		/*
		else if(strcmp(tokens->items[i], ">") == 0)
		{
			redirector_positions[redirectorOut_count] = i;
			redirectorOut_count++;
		}
		*/
	}

	// Line 217 - Creating an array of tokenlists for commands 

	// QUICK TEST 1:05 PM

	
	tokenlist *command[1] = {NULL};
	for(int i=0; i <= redirector_count; i++)	// took out redirector_positons[1]
	{
		command[i] = new_tokenlist();
	}

	// Populate the tokenlist for commands

	size_t start = 0;
	for(int i = 0; i <= redirector_count; i++)
	{
		size_t end;
			if(i == redirector_count)
			{
				end = num_tokens;
			}
			else
			{
				end = redirector_positions[i];
			}
		for(size_t j = start; j < end; j++)
			{
				add_token(command[i], tokens->items[j]);
			}
		start = end + 1;
	}


	// Jasmine Pipe stuff 
	

	// pipe file descriptors initialization
	//int pipefds[2][2];
	pid_t pids[3];

	// Create pipes
	/*
	for (int i = 0; i < redirector_count; i++)
	{
		if(pipe(pipefds[i]) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	}

	*/
	
	// Le Fork
	for(int i = 0; i <= redirector_count; i++)
	{
		pids[i] = fork();
		if (pids[i] == -1)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		

	if(pids[i] == 0)
	{
		// if child process
		if(i > 0)
		{

			if(redirectorOut_count == 1 && redirectorIn_count == 0) 
			{
			int outfd = dup(STDOUT_FILENO);
			close(STDOUT_FILENO);
			outfd = open("output.txt", O_RDWR | O_CREAT);
			dup2(outfd,STDIN_FILENO);
			close(outfd);
			printf("Redirected output to test,txt\n");		// test line
			}
			
		}

		char * cmd_path = pathSearch(command[i]); 
		if(cmd_path)
		{
		ExternalCommandExec(command[i], cmd_path);
		free(cmd_path);
		}
		else
		{
		fprintf(stderr, "Command not found: %s\n", command[i]->items[0]);
		exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);

	}
	}
	
	for(int i = 0; i <= redirector_count; i++)
	{
		waitpid(pids[i], NULL, 0);
	}

	for(int i = 0; i <= redirector_count; i++)
	{
		free_tokens(command[i]);
	}
}

	// < and > stuff from the other day

	 /*for(int i = 0; i <= redirector_count; i++)
	{
		if(redirectorOut_count == 1 && redirectorIn_count == 0)							// not sure if this overwrites
		{
			int outfd = dup(STDOUT_FILENO);
			close(STDOUT_FILENO);
			int fd = open("text.txt", O_RDWR | O_CREAT);
			dup2(outfd,STDIN_FILENO);
			close(outfd);

			printf("Redirected output to test,txt\n");		// test line
		}
		else if(redirectorIn_count = 1 && redirectorOut_count == 0)
		{
			char str[50];
			int infd = dup(STDIN_FILENO);
			close(STDIN_FILENO);
			fgets(str, 60, stdin);			// maybe slap in stdin_fileno here??
			if(!open())
			{
				printf("File does not exist or is not a regular file\n");
			}
			dup2(infd, STDIN_FILENO);
			printf("Redirected input %s", str);

		}
	}

	*/

	



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