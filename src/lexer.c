#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mytimeout.h"

#define MAX_HISTORY_SIZE 3

struct commandHistory {
    char commands[MAX_HISTORY_SIZE][200]; // Assuming commands are less than 200 characters
    int count;
};

struct commandHistory history;

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
	// while(1)
	// {
		if (tokens->size > 0) 
		{
			historyCommandList(tokens, input);
			if (hasPipe(tokens)) 
			{
				piping(tokens);
			}
			else 
			{
				if (strcmp(tokens->items[0],"exit") == 0 || strcmp(tokens->items[0],"cd") == 0)
				{
					// running inernal command execution
					internalCommandExecution(tokens);			
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
		printList(tokens);
	// }
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

char * ExternalCommandExec(const tokenlist * tokens, char * filePath)
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

	return 0;
}

char * piping(tokenlist *tokens) 
{
    size_t num_tokens = tokens->size;

    // Find the positions of the pipe tokens ("|")
    size_t pipe_positions[2] = {0};
    int pipe_count = 0; // Use this becasue there could be one or two pipes
    for (size_t i = 0; i < num_tokens; i++) 
	{
        if (strcmp(tokens->items[i], "|") == 0) 
		{
            pipe_positions[pipe_count] = i;
            pipe_count++;
        }
    }
	// pipe has to be more than one. Pipe cannot be located as the first character, and cannot be last character
    if (pipe_count < 1 || pipe_positions[0] == 0 || pipe_positions[pipe_count - 1] == num_tokens - 1) 
	{
        fprintf(stderr, "Invalid input: Missing command(s)\n");
		return 0; 
    }

    // Create an array of tokenlists for commands (There can be two and three commands)
    tokenlist * commands[3] = {NULL};
    for (int i = 0; i <= pipe_count; i++)  // based on the number of new tokens
	{
        commands[i] = new_tokenlist();
    }

    // Populate the tokenlists for commands
    size_t start = 0; // 
    for (int i = 0; i <= pipe_count; i++) 
	{
        size_t end;
		if (i == pipe_count)
		{
			end	= num_tokens;
		}
		else
		{
			end = pipe_positions[i];
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
    for (int i = 0; i < pipe_count; i++) 
	{
        if (pipe(pipefds[i]) == -1) 
		{
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i <= pipe_count; i++) 
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

            if (i < pipe_count) 
			{
                // Redirect standard output to the current pipe
                dup2(pipefds[i][1], STDOUT_FILENO);
                close(pipefds[i][0]);
            }

            // Close all pipe file descriptors
            for (int j = 0; j < pipe_count; j++) 
			{
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            // Execute the command
            char * cmd_path = pathSearch(commands[i]);
            if (cmd_path) 
			{
                ExternalCommandExec(commands[i], cmd_path);
                free(cmd_path);
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
	
    // Parent process
    // Close all pipe file descriptors
    for (int j = 0; j < pipe_count; j++) 
	{
        close(pipefds[j][0]);
        close(pipefds[j][1]);
    }

    // Wait for all child processes to complete
    for (int i = 0; i <= pipe_count; i++) 
	{
        waitpid(pids[i], NULL, 0);
    }

    // Free tokenlists
    for (int i = 0; i <= pipe_count; i++) 
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
		
	}
	return tokens->items[1]; 
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

