#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>    


struct Args {
    int length;
    char **args;    
};

char *getCommand(char *cmd) {
    printf(": ");
    size_t inputLength = 0;
    if (getline(&cmd, &inputLength, stdin) != -1) {
        // Remove "\n" that getline adds to end of input
        int len = strlen(cmd);
        if (len > 0 && cmd[len-1] == '\n') {    
            cmd[len-1] = 0;            
        }        
    } 
    return cmd;
}

struct Args *getArgs(char *cmd) {   
    struct Args *args = (struct Args*)malloc(sizeof(struct Args));
    args->args = (char **)malloc(1*sizeof(char *));    
    char **tempArgs; 
    char *token, *tracker;      // tracker is used by strtok_r to keep track of the remaining portion of the string being tokenized   
    token = strtok_r(cmd, " ", &tracker);
    int counter = 0;
    while (token != NULL) {        
        if (counter == 0) {
            tempArgs = args->args;
        } else {
            // Note to self: realloc copies the data over to the new memory location, you do NOT have to do this yourself!!!
            tempArgs = (char **)realloc(args->args, (counter+1)*sizeof(char *));
        }           
        if (tempArgs == NULL) {     // Memory reallocation failed
            break;  
        }        
        args->args = tempArgs;        
        args->args[counter] = (char *)malloc((strlen(token)+1)*sizeof(char));
        if (args->args[counter] == NULL) {
            printf("Malloc failed\n");
            exit(1);
        }
        strcpy(args->args[counter], token);
        counter++;
        token = strtok_r(NULL, " ", &tracker);
    }     
    args->length = counter;    

    return args;   
}

void printArgs(struct Args *args) {
    int i;
    for (i = 0; i < args->length; i++) {
        printf("%s\n", args->args[i]);
    }
}

int getCmdType(char *cmd) {
    int cmdLength = strlen(cmd);
    
    if (strcmp(cmd, "exit") == 0) {
        return 1;
    } else if (strncmp(cmd, "cd", 2) == 0) {    // Check if command starts with "cd" 
        if (cmdLength == 2) {
            return 2;
        } else if (cmdLength > 2 && cmd[2] == ' ') {    // Check if there is a space after "cd" when length of command is greater than 2
            return 2;
        }
    } else if (strcmp(cmd, "status") == 0) {
        return 3;
    }

    return -1;
}

void freeArgs(struct Args *args) {
    int length = args->length;
    int i;
    for (i = 0; i < length; i++) {
        free(args->args[i]);
    }
    free(args->args);
    free(args);
}

void handleCd(char *cmd) {
    struct Args *args;
    args = getArgs(cmd);
    printArgs(args);
    freeArgs(args);

}

void handleStatus(char *cmd) {

}

void handleFg() {

}

void handleBg() {

}

void handleComment() {

}

int main() {
    char *cmd;
    int cmdType = -1;
    while(1) {
        cmd = getCommand(cmd);

        // Command types reference:
        // Built-in commands:
        // exit -> 1    cd -> 2     status -> 3
        // Foreground commands -> 4
        // Background commands -> 5
        // Comments -> 6
        cmdType = getCmdType(cmd);
        if (cmdType == 1) {            
            break;
        } else if (cmdType == 2) {
            printf("cd entered...\n");
            handleCd(cmd);
        } else if (cmdType == 3) {
            printf("status entered...\n");
            handleStatus(cmd);
        } else if (cmdType == 4) {
            handleFg(cmd);
        } else if (cmdType == 5) {
            handleBg(cmd);
        } else if (cmdType == 6) {
            handleComment();
        } else if (cmdType == -1) {            
            printf("Command %s not recognized\n", cmd);
        } 
    }
    return 0;
}