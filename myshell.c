//
//  myshell.c
//  Operating Systems - Coursework Part 1: User-space shell
//  OS-CW1
//
//  Created by Gavin Waite on 30/01/2016.
//  The University of Edinburgh
//

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#define BUFFER 625 //(12 args plus the command)
#define MAX_ARGS 25
#define MAX_ARG_LENGTH 50

void ctrlCdetected(int signal);
void resetVariables(char *command, char **args);
int printNewPrompt();
void parseLine(char *inputLine, char *command, char **args);
void executeCommand(char *command, char **args);

sigjmp_buf CtrlCBuffer;
int ctrlCflag = 0;
const char *delim = " \t\n"; // Use spaces or tabs as delimiters between tokens in strsep
int exitCode = 0;

int main(int argc, const char * argv[]) {
    
    // Register a signal handler for Ctrl-C
    // Ctrl-C generates the SIGINT signal, which calls the ctrlCdetected function
    signal(SIGINT, ctrlCdetected);
    
    // Initialise the memory locations for the input parsing
    char command[MAX_ARG_LENGTH];
    // Allocate memory on the main stack for the MAX_ARGS possible arguments in the command
    char *args[MAX_ARGS];
    char inputLine[BUFFER];
    
    // Setup the location for the Ctrl-C buffer to jump to when handled
    while ( sigsetjmp( CtrlCBuffer, 1 ) != 0 );
    
    // Continuous operation loop
    while (1) {
        
        resetVariables(command, args);
        
        if (1 == printNewPrompt()){
            continue;
        }
        
        fgets(inputLine, BUFFER, stdin);
        parseLine(inputLine, command, args);
        executeCommand(command, args);
        
    }
    return 0;
}

void resetVariables(char *command, char **args){
    // Set/reset the variables to zero
    strcpy(command, "");
    for (int i=0; i<MAX_ARGS; i++){
        args[i] = NULL;
    }
}

int printNewPrompt(){
    // Get the current working directory
    char currentDir[1024];
    getcwd(currentDir, sizeof(currentDir));
    
    // If the Ctrl-C signal has been detected then
    // start a new line and prompt
    if (ctrlCflag) {
        printf("\n");
        ctrlCflag = 0;
        signal(SIGINT, ctrlCdetected);
        return 1;
    }
    
    // Wait for input from standard in
    printf("MyShell: %s > ", currentDir);
    return 0;
}

void parseLine(char *inputLine, char *command, char **args){
    
    int counter = 0;
    char *token = NULL;
    char *linePointer;
    linePointer = inputLine;
    
    // Parse the given inputLine into the command and any arguments
    while ((token = strsep(&linePointer, delim)) != NULL) {
        // Safety check for the empty string, due to the nature of strsep()
        if (strcmp("",token) != 0){
            if (counter == 0){
                strcpy(command, token);
                args[counter] = token;
            }
            else {
                args[counter] = token;
            }
            counter++;
            if (counter > (MAX_ARGS)){
                printf("ERROR: More arguments given [%d] than are supported [%d]\n",(counter-1),(MAX_ARGS));
            }
        }
    }
    args[counter] = NULL;
}

void executeCommand(char *command, char **args){
    
    if (strcmp("cd", command) == 0){
        if (args[1] == NULL){
            printf("No directory specified, changing to default HOME\n");
            char *HOME;
            HOME = getenv("HOME");
            exitCode = chdir(HOME);
        }
        else {
            exitCode = chdir(args[1]);
            if (exitCode != 0){
                printf("ERROR: could not find directory\n");
            }
        }
    }
    else if (strcmp("exit", command) == 0) {
        
        printf("exit called: exitCode = %d\n", exitCode);
        
        if (args[1] == NULL){
            exit(exitCode);
        }
        else {
            int intArg0 = atoi(args[1]);
            
            if (intArg0 != 0){
                exit(intArg0);
            }
            else {
                printf("ERROR: Invalid exit code supplied\n");
                exit(0);
            }
            
        }
    }
    else {
        // Fork a new process
        pid_t pid = fork();
        int status;
        
        // If the newly forked child process
        if (pid == 0) {
            exitCode = execvp(*args,args);
            printf("Process %d executed with exitCode %d\n",pid, exitCode);
            if (exitCode == -1){
                printf("ERROR: Unknown command\n");
            }
            exit(exitCode);
        }
        // Must be the original parent process
        else {
            while(wait(&status) != pid);
        }
    }
}

// A function which is called asynchronously when the ctrl-C (SIGINT) signal is detected
void ctrlCdetected(int sig){
    ctrlCflag = 1;
    signal(sig, SIG_IGN);
    siglongjmp(CtrlCBuffer, 1);
}
