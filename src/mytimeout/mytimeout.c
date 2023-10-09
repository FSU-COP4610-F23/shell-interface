#define _POSIX_C_SOURCE 200809L // Define _POSIX_C_SOURCE before including any headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // Include this header for kill and SIGTERM
#include "lexer.h" 

// int main(int argc, char *argv[]) {
//     if (argc < 3) {
//         fprintf(stderr, "Usage: %s [seconds] [command] [command-args]\n", argv[0]);
//         return 1;
//     }

//     int timeout_seconds = atoi(argv[1]);
//     if (timeout_seconds <= 0) {
//         fprintf(stderr, "Invalid timeout value\n");
//         return 1;
//     }

//     pid_t child_pid = fork();

//     if (child_pid == -1) {
//         perror("fork");
//         return 1;
//     }

//     if (child_pid == 0) {
//         // This code runs in the child process
//         char *cmd = argv[2];
//         char **cmd_args = &argv[2];

//         execvp(cmd, cmd_args);
//         perror("execvp"); // If execvp fails, print an error
//         exit(1);
//     } else {
//         // This code runs in the parent process
//         sleep(timeout_seconds);

//         // Try to terminate the child process gracefully
//         int status;
//         if (waitpid(child_pid, &status, WNOHANG) == 0) {
//             // Child process is still running; send a termination signal
//             kill(child_pid, SIGTERM);
//         }

//         // Wait for the child process to finish
//         waitpid(child_pid, &status, 0);

//         if (WIFEXITED(status)) {
//             return WEXITSTATUS(status);
//         } else {
//             fprintf(stderr, "Child process terminated abnormally\n");
//             return 1;
//         }
//     }

//     return 0;
// }


int execute_with_timeout(tokenlist *tokens, int timeout_seconds) {
    if (tokens->size < 2) {
        fprintf(stderr, "Usage: %s [seconds] [command] [command-args]\n", tokens->items[0]);
        return 1;
    }

    timeout_seconds = atoi(tokens->items[1]);
    if (timeout_seconds <= 0) {
        fprintf(stderr, "Invalid timeout value\n");
        return 1;
    }

    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        // This code runs in the child process
        char *cmd = tokens->items[2];
        char **cmd_args = &(tokens->items[2]);

        execvp(cmd, cmd_args);
        perror("execvp"); // If execvp fails, print an error
        exit(1);
    } else {
        // This code runs in the parent process
        sleep(timeout_seconds);

        // Try to terminate the child process gracefully
        int status;
        if (waitpid(child_pid, &status, WNOHANG) == 0) {
            // Child process is still running; send a termination signal
            kill(child_pid, SIGTERM);
        }

        // Wait for the child process to finish
        waitpid(child_pid, &status, 0);

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            fprintf(stderr, "Child process terminated abnormally\n");
            return 1;
        }
    }
                                       
    return 0;
}