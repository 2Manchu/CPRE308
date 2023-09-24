#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_LEN 300
#define MAX_ARG_LEN 100
#define MAX_BG_PROCESSES 50

pid_t backgroundProcesses[MAX_BG_PROCESSES];
char backgroundProcNames[MAX_BG_PROCESSES][MAX_ARG_LEN];
int numBackgroundProcesses = 0;

char *currentArg;
char *execArgs[MAX_CMD_LEN] = {NULL};

void freeArgMemory(int numExecArgs) {
    //Only need to do a less than here since the
    for (int i = 0; i <= numExecArgs; i++) {
        free(execArgs[i]);
        execArgs[i] = NULL;
    }
}

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
    int numFinishedProcesses = 0;

    int background = 0;
    int currentChar;

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

    //Parse the input for executable/command name and arguments
    int quoteSeen = 0;
    int execArgsIndex = 0;
    int currentArgIndex = 0;
    currentArg = (char*) malloc(MAX_ARG_LEN * sizeof(char));
    for(currentChar = 0; currentChar < strlen(input); currentChar++) {
        //Check for space and inside/outside of quotes
        //This portion is space-delimited but only if outside of quotes
        if ((input[currentChar] == 32 || input[currentChar] == 10) && quoteSeen == 0) {
            //Append a null character to the end of the argument
            currentArg[currentArgIndex] = '\0';
            //Copy the current built argument to the arguments array
            execArgs[execArgsIndex] = currentArg;

            //Reset the character index for the current argument
            currentArgIndex = 0;

            //We should count this as the start of the next argument if we are not in quotes

            //New memory address so the old one doesn't get overwritten, only if not newline
            if (input[currentChar] != 10) {
                currentArg = (char *) malloc(MAX_ARG_LEN * sizeof(char));
                execArgsIndex++;
            }
            continue;
        //Ignore spaces if we are inside a quotation
        } else if (input[currentChar] == 32 && quoteSeen == 1) {
            currentArg[currentArgIndex] = input[currentChar];
            currentArgIndex++;
            continue;
        }
        //If we see a " mark
        if (input[currentChar] == 34 && !quoteSeen) {
            quoteSeen = 1;
            continue;
        } else if (input[currentChar] == 34 && quoteSeen) {
            quoteSeen = 0;
            continue;
        }

        currentArg[currentArgIndex] = input[currentChar];
        currentArgIndex++;
    }

    //---------Built-in commands---------
    if (strcmp(execArgs[0], "exit") == 0) {
        freeArgMemory(execArgsIndex);
        return 127;
    } else if (strcmp(execArgs[0], "pid") == 0) {
        printf("PID: %d\n", getpid());
        freeArgMemory(execArgsIndex);
        return 0;
    } else if (strcmp(execArgs[0], "ppid") == 0) {
        printf("PPID: %d\n", getppid());
        freeArgMemory(execArgsIndex);
        return 0;
    } else if (strcmp(execArgs[0], "pwd") == 0) {
        char *workingDir = NULL;
        workingDir = getwd(workingDir);
        printf("%s\n", workingDir);
        freeArgMemory(execArgsIndex);
        return 0;
    } else if (strcmp(execArgs[0], "cd") == 0) {
        //Second argument should be a directory
        if (chdir(execArgs[1])) {
            printf("Specified file does not exist or is not a directory.\n");
            freeArgMemory(execArgsIndex);
            return 1;
        }
        freeArgMemory(execArgsIndex);
        return 0;
    }

    if (strcmp(execArgs[execArgsIndex], "&") == 0) {
        execArgs[execArgsIndex] = NULL;
        background = 1;
    }
    //We didn't have a built-in command, so execute the program specified by execArgs[0] with args execArgs
    execProgram(execArgs[0], execArgs, background);
    freeArgMemory(execArgsIndex);
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
