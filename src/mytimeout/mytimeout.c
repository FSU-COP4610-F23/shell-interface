#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include "mytimeout.h"

volatile sig_atomic_t timeout_flag = 0;

void handle_timeout(int signum) {
    timeout_flag = 1;
}

int main(int argc, char *argv[]) 
{

    create_timeout(argc, argv);
    return 0;
}


void create_timeout(int argc, char *argv[]) 
{
    printf("We go in here\n");
    if (argc < 2) 
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGALRM, handle_timeout);

    // Set a timeout of 5 seconds (adjust as needed) //
    alarm(argc);

    pid_t child_pid = fork(); // fork 

    if (child_pid == -1) 
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) // if in the child process
    {
        char *cmd = argv[1]; // Get the command from arguments

        // Perform a PATH search to find the full path of the command
        char *path = getenv("PATH");
        char *token = strtok(path, ":");
        while (token != NULL) {
            char command_path[256]; // Adjust the size as needed
            snprintf(command_path, sizeof(command_path), "%s/%s", token, cmd);

            if (access(command_path, X_OK) == 0) {
                // Execute the command with arguments
                execv(command_path, argv + 1);
                perror("execv failed");
                exit(EXIT_FAILURE);
            }

            token = strtok(NULL, ":");
        }

        // If the command is not found in PATH
        fprintf(stderr, "Command not found: %s\n", cmd);
        exit(EXIT_FAILURE);
    } 
    else 
    {
        // Parent process
        printf("Waiting for child process...\n");
        int status;
        waitpid(child_pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Child process exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal %d\n", WTERMSIG(status));
        }

        if (timeout_flag) {
            printf("Timeout occurred. Killing child process.\n");
            kill(child_pid, SIGKILL); // Send SIGKILL to child process
        }
    }

    return 0;

}