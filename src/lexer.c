#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
		tokenlist *tokens = get_tokens(input);
		char *filePath;

		tokens->items[1] = environmentVariables(tokens); // what happens to previous memory in token->items[1]?
		tokens->items[1] = tildeExpansion(tokens);
		filePath = pathSearch(tokens);
		ExternalCommandExec(tokens, filePath);

		//printList(tokens);

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
		strcat(tempFilePath, tokens->items[0]);
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
		printf("Command not found\n"); 
	//else
	//  printf("This is correct path: %s\n", filePath);

	return filePath; //I tink this what u return
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


// void lexer_parse_token()
// {
// 	while (1)
// 	{
// 		// print out prompt
// 		printf("\n");
// 		prompt();

// 		/* input contains the whole command
// 		 * tokens contains substrings from input split by spaces
// 		 */
// 		char *input = get_input();
// 		printf("whole input: %s\n", input);

// 		tokenlist *tokens = get_tokens(input);

// 		for (int i = 0; i < tokens->size; i++) {
// 			printf("token %d: (%s)\n", i, tokens->items[i]);
// 		}

// 		environmentVariables(tokens);
// 		tildeExpansion(tokens);

// 		free(input);
// 		free_tokens(tokens);
// 	}
// }



// void environmentVariables(tokenlist * tokens)
// {
// 	for (int i = 0; i < tokens->size; i++)
// 	{
// 		// printf("this should give me echo: %s\n", tokens->items[i]);
// 		// printf("%c",tokens->items[i][0]);
// 		// int charCount = 0;
// 		if (tokens->items[i][0] == '$')
// 		{
// 			// printf("%s", tokens->items[i--]);
// 			char tokenItems[100] = "";
// 			strcat(tokenItems, &tokens->items[i][1]);

// 			printf("%s", getenv(tokenItems));
// 		}
// 		else if (tokens->items[i][0] != '~' && tokens->items[i][0] != '$')
// 		{
// 			printf("%s ", tokens->items[i]);
// 		}
// 	}
// }

// void tildeExpansion(tokenlist * tokens)
// {
// 	for (int i = 0; i < tokens->size; i++)
// 	{
// 		if (tokens->items[i][0] == '~')
// 		{
// 			bool check = false;
// 			char tokenItems[100] = "";
// 			if (tokens->items[i][1] == '/')
// 			{
// 				check = true;
// 				// printf("this is &token->items:%s\n", &tokens->items[i][1]);
// 				strcat(tokenItems, &tokens->items[i][1]);
// 				// printf("#3 starts here: %s", tokenItems);
// 			}

// 			printf("%s", getenv("HOME"));
// 			// strcpy(tokens->items[i], getenv("HOME"));

// 			if (check)
// 			{
// 				// strcat(tokens->items[i], tokenItems);
// 				printf("%s", tokenItems);
// 				// printf("%s", tokens->items[i]);
// 			}
// 		}
// 	}
// }
