#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_LEN 300
#define MAX_BG_PROCESSES 50
#define MAX_BINARY_NAME 100

pid_t backgroundProcesses[MAX_BG_PROCESSES];
char backgroundProcNames[MAX_BG_PROCESSES][MAX_BINARY_NAME];
int numBackgroundProcesses = 0;

int execProgram(char *executable, char *argv[], int background) {
    ///Fork the child process
    pid_t childPID = fork();

    if (childPID == 0) {
        printf("[%d] %s\n", getpid(), executable);
        //Execute the executable
        if (execvp(executable, argv) == -1) {
            printf("Cannot exec %s: No such file or directory.\n", executable);
            exit(1);
        }
    //Parent process
    } else if (childPID > 0) {
        //We busy wait on the process to complete if this is not a background process
        if (!background) {
            int status = -1;
            //Wait for child to complete
            waitpid(childPID, &status, 0);
            printf("[%d] %s Exit %d\n", childPID, executable, status);
            return 0;
        } else {
            backgroundProcesses[numBackgroundProcesses] = childPID;
            strcpy(backgroundProcNames[numBackgroundProcesses], executable);
            numBackgroundProcesses++;

            //TODO: Non janky fix for proc details printing after prompt??
            usleep(100000);
            return 0;
        }
    }
    //Failed to create child process
    else {
        printf("Failed to create child process.\n");
        return 1;
    }
}

int processInput(char input[]) {
    char *command;
    char inputClone[MAX_CMD_LEN] = {"0"};
    int numFinishedProcesses = 0;

    //First up, check to see if background processes have completed and output their exit codes
    for (int i = 0; i < numBackgroundProcesses; i++) {
        int exitStatus = -1;
        waitpid(backgroundProcesses[i], &exitStatus, WNOHANG);
        //If our background process has exited, print its info
        if (exitStatus >= 0) {
            printf("[%d] %s Exit %d\n", backgroundProcesses[i], backgroundProcNames[i], exitStatus);
            numFinishedProcesses++;
        }
    }
    numBackgroundProcesses -= numFinishedProcesses;

    //Avoid directly modifying the input
    //FIXME: Necessary?
    strcpy(inputClone, input);
    //Extract the executable from the input string
    command = strtok(inputClone, " ");

    //---------Built-in commands---------
    if (strcmp(command, "exit\n") == 0) {
        return 127;
    } else if (strcmp(command, "pid\n") == 0) {
        printf("PID: %d\n", getpid());
        return 0;
    } else if (strcmp(command, "ppid\n") == 0) {
        printf("PPID: %d\n", getppid());
        return 0;
    } else if (strcmp(command, "pwd\n") == 0) {
        char *workingDir = NULL;
        workingDir = getwd(workingDir);
        printf("%s\n", workingDir);
        return 0;
    } else if (strcmp(command, "cd") == 0) {
        char *directory = NULL;
        //The rest of the string before newline should be a directory, so grab it
        directory = strtok(NULL, "\n");
        if (chdir(directory)) {
            printf("Specified file does not exist or is not a directory.\n");
            return 1;
        }
        return 0;
    }

    //---------Extract program arguments and call exec routine---------
    char *argv[MAX_CMD_LEN] = {NULL};
    char *next_arg;
    int i = 0, background = 0;

    //First argument is always the executable name
    argv[0] = command;

    //Get program arguments
    while (1) {
        i++;
        //Grab next switch/option/path, etc. out of the input executable
        next_arg = strtok(NULL, " ");
        //Check to see if we've hit the end of the argument string (or there were no switches set to begin with)
        if (next_arg == NULL) {
            break;
        }

        //FIXME: DEBUG
        //printf("Adding to argv[%d]: %s\n", i, next_arg);
        argv[i] = next_arg;
    }
    int lastArgLen = strlen(argv[i-1]);
    argv[i-1][lastArgLen - 1] = '\0';

    //Check to see if user wants to run process in background, set if & exists as the last arg
    if (strcmp(argv[i-1], "&") == 0) {
        background = 1;
        //Delete the & from the args vector
        argv[i-1] = NULL;
    }

    //Execute the program specified by argv[0] with args argv
    execProgram(argv[0], argv, background);
}

int main(int argc, char *argv[]) {
    char userInput[MAX_CMD_LEN];
    int processInputRet;

    //Main shell loop
    while(1) {
        //Program name is argv[0], check 2nd arg for a -p
        if (argc > 1 && strcmp(argv[1], "-p") == 0) {
            //Print the prompt
            printf("%s> ", argv[2]);
        } else {
            printf("308sh> ");
        }

        //Start of our actual user-facing part of the shell that they can interact with
        fgets(userInput, MAX_CMD_LEN, stdin);

        processInputRet = processInput(userInput);
        if (processInputRet == 127) {
            return 0;
        }
    }
}
