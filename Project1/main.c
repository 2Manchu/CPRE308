#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define MAX_CMD_LEN 300

int processInput(char input[]) {
    char *command;
    char inputClone[MAX_CMD_LEN];

    //Avoid directly modifying the input
    strcpy(inputClone, input);
    //Extract the command from the input string
    command = strtok(inputClone, " ");

    //---------Built-in commands---------
    if (strcmp(command, "exit\n") == 0) {
        return 0;
    } else if (strcmp(command, "pid\n") == 0) {
        printf("PID: %d\n", getpid());
    } else if (strcmp(command, "ppid\n") == 0) {
        printf("PPID: %d\n", getppid());
    } else if (strcmp(command, "pwd\n") == 0) {
        char *workingDir = NULL;
        workingDir = getwd(workingDir);
        printf("%s\n", workingDir);
    } else if (strcmp(command, "cd") == 0) {
        char *directory = NULL;
        //The rest of the string before newline should be a directory, so grab it
        directory = strtok(NULL, "\n");
        //DEBUG
        //printf("%s\n", directory);
        if (chdir(directory)) {
            printf("Specified file does not exist or is not a directory.\n");
            return 1;
        }
    }

    //---------Exec program---------
    pid_t childPID = fork();
    //Child process
    if (childPID == 0) {
        char *argv[MAX_CMD_LEN] = {NULL};
        int i = 1;

        //First argument of argv is always the executable name
        argv[0] = command;
        //Grab next switch/option/path, etc. out of the input command
        char *next_arg = strtok(NULL, " ");

        //While we don't have the end of the command
        while (strcmp(next_arg, "\0") || strcmp(next_arg, "\n")) {
            //DEBUG
            printf("Adding to argv[%d]: %s", i, next_arg);
            argv[i] = next_arg;
            //Grab the next switch/option/path
            strtok(NULL, " ");
            i++;
        }

        //Execute the executable
        printf("[%d] %s\n", getpid(), argv[0]);
        execvp(command, argv);
    //Parent process
    } else if (childPID > 0) {
        int status;
        //Wait for child to complete
        waitpid(childPID, &status, 0);
        printf("[%d] Exit %d\n", childPID, status);
    }
    //Failed to create child process
    else if (childPID == -1) {
        printf("Failed to create child process.\n");
        return 1;
    }
}

int main(int argc, char *argv[]) {
    char userInput[MAX_CMD_LEN];
    int processInputRet = -1;

    //Main shell loop
    while(1) {
        //Program name is argv[0], check 2nd arg for a -p
        if (strcmp(argv[1], "-p") == 0) {
            //Print the prompt
            printf("%s> ", argv[2]);
        } else {
            printf("308sh> ");
        }

        //Start of our actual user-facing part of the shell that they can interact with
        fgets(userInput, MAX_CMD_LEN, stdin);
        processInputRet = processInput(userInput);
        if (processInputRet == 0) {
            return 0;
        }
    }
}
