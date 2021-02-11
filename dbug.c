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

void *getCommand(char *cmd) {    
    printf(": ");
    fflush(stdout);
    char *buffer;
    size_t inputLength = 0;
    if (getline(&buffer, &inputLength, stdin) != -1) {
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {    
            buffer[len-1] = 0;            
        } 
        cmd = (char *)malloc((strlen(buffer))*sizeof(char));
        strcpy(cmd, buffer);

        // Remove "\n" that getline adds to end of input
        // int len = strlen(cmd);
        // if (len > 0 && cmd[len-1] == '\n') {    
        //     cmd[len-1] = 0;            
        // }        
        // char *temp = (char *)malloc(len * sizeof(char));
        // strcpy(temp, cmd);
        // cmd = temp;       
    } 

    // printf(": ");
	// fflush(stdout);
	// cmd[0] = '\0';
	// fgets(cmd, 2048, stdin);
    // int len = strlen(cmd);
    // if (len > 0 && cmd[len-1] == '\n') {    
    //     cmd[len-1] = '\0';            
    // }    
}

// Get subarray to be used as null terminated array of string arguments for execvp
char **getSubArray(char **array, int end) {
    char **subArr = (char **)malloc((end+1)*sizeof(char *));
    int i;
    for (i = 0; i < end; i++) {
        subArr[i] = array[i];
    }
    subArr[end] = NULL;
    return subArr;
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
            // Note to self: realloc copies the data over to the new memory location for you!!!
            tempArgs = (char **)realloc(args->args, (counter+1)*sizeof(char *));
        }           
        if (tempArgs == NULL) {     // Memory reallocation failed
            break;  
        }        
        args->args = tempArgs;        
        args->args[counter] = (char *)malloc((strlen(token)+1)*sizeof(char));
        if (args->args[counter] == NULL) {
            printf("Malloc failed\n");
            fflush(stdout);
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
        fflush(stdout);
    }
}

int getCmdType(char *cmd) {
    int cmdLength = strlen(cmd);
    
    if (cmdLength == 0) {
        return 0;
    } else if (strcmp(cmd, "exit") == 0) {
        return 1;
    } else if (strncmp(cmd, "cd", 2) == 0) {    // Check if command starts with "cd" 
        if (cmdLength == 2) {
            return 2;
        } else if (cmdLength > 2 && cmd[2] == ' ') {    // Check if there is a space after "cd" when length of command is greater than 2
            return 2;
        }
    } else if (strcmp(cmd, "status") == 0) {
        return 3;
    } else if (cmd[cmdLength-1] != '&') {
        return 4;
    } else if (cmd[cmdLength-1] == '&' && cmd[cmdLength-2] == ' ') {
        return 5;
    } else if (cmd[0] == '#') {
        return 6;
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
    char *path;
    if (args->length > 1) {
        path = args->args[1];    // Argument that comes after "cd" is path of directory to open
    } else if (args->length == 1) {     // If no arguments other than "cd" were given, change to HOME directory
        path = getenv("HOME");
    }
    int result = chdir(path);
    if (result == -1) {
        printf("Error changing to directory\n");
        fflush(stdout);
    } else {
        char cwd[2048];
        getcwd(cwd, sizeof(cwd));
        printf("Currerent working directory is now: %s\n", cwd);
        fflush(stdout);
    }     

    freeArgs(args);
}

void printStatus(int status) {
    printf("exit value %d\n", status);
    fflush(stdout);
}

// Uses code from "Exploration: Processes and I/O"
int redirect(char* symbol, char *filename) {
    if ((strcmp(symbol, ">") == 0)) {
        int targetFD = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (targetFD == -1) {
            perror("open()");
            return -1;
        }
        // Use dup2 to point FD 1, i.e., standard output to targetFD
        int result = dup2(targetFD, 1);
        if (result == -1) {
            perror("dup2"); 
            return -1; 
        }
    } else if ((strcmp(symbol, "<") == 0)) {
        // Open source file
        int sourceFD = open(filename, O_RDONLY);
        if (sourceFD == -1) { 
            perror("source open()"); 
            return -1;
        }
        int result = dup2(sourceFD, 0);
        if (result == -1) { 
            perror("source dup2()"); 
            return -1;
        }
    }

    return 0;
}

// Handle creation of foreground processes
void handleFg(char *cmd, int *statusPtr) {
    struct Args *args;
    args = getArgs(cmd);
    // Uses code from "Exploration: Process API – Creating and Terminating Processes"
    int childStatus;
    pid_t childPID = -5;
    // If fork is successful, the value of childPID will be 0 in the child, and the pid of the child in the parent
	childPID = fork();
    switch (childPID) {
		case -1:
            // Code in this branch will be exected by the parent when fork() fails and the creation of child process fails as well
			perror("fork() failed!");	
            exit(1);
			break;
		case 0:
            // childPID is 0. This means the child will execute the code in this branch
            if (args->length > 1) {
                int delimiter = -1;
                // Check for redirection arguments if there is more than one argument
                int i;
                for (i = 1; i < args->length; i++) {
                    // The '>' symbol is used for output (STDOUT)
                    // The '<' symbol is used for input (STDIN)
                    if ((strcmp(args->args[i], "<") == 0) || (strcmp(args->args[i], ">") == 0)) {
                        // Look for filename argument
                        if ((i+1) < args->length) {
                            if (delimiter == -1) {
                                delimiter = i;
                            }
                            // Call redirection function
                            int redirected = redirect(args->args[i], args->args[i+1]);    
                            if (redirected == -1) {
                                *statusPtr = 1;
                            }         
                        } 
                    }            
                }

                // If redirection commands were found in original array of args, get rid of them before passing args to exec function
                char **newArgs;
                if (delimiter != -1) {
                    newArgs = getSubArray(args->args, delimiter);
                } else {
                    newArgs = args->args;
                }                        
                execvp(newArgs[0], newArgs);                                                           
            } else if (args->length == 1) {
                printf("calling execvp\n");   
                fflush(stdout);   
                execvp(args->args[0], args->args);          
                printf("execvp failed\n");   
                fflush(stdout);   
            }  

            // execvp only returns if error, so we know that the following code will only be executed if an error occurred            
            perror("Error executing command");
            exit(1);
			break;
		default:
            // childPID is the pid of the child. This means the parent will execute the code in this branch
            childPID = waitpid(childPID, &childStatus, 0);
            printf("Parent has finished waiting for child with pid of %d\n", childPID);
            fflush(stdout);
            // If error occurred then print message and change exit status to 1
            if (WIFEXITED(childStatus) == 0) {
                *statusPtr = 1;
                printStatus(*statusPtr);                
            }

            printf("actual exit status %d\n", WEXITSTATUS(childStatus));
            fflush(stdout);

            freeArgs(args);
			break;
	}   
}

// Handle creation of background processes
void handleBg() {

}

int main() {
    char *cmd = NULL;
    int *statusPtr, status;
    status = 0;
    statusPtr = &status;
    int cmdType = -1;
    while(1) {
        if (cmd != NULL) {
            free(cmd);
        }
        getCommand(cmd);       
        cmdType = getCmdType(cmd);
        if (cmdType == 1) {     // exit
            break;
        } else if (cmdType == 2) {      // cd
            handleCd(cmd);
        } else if (cmdType == 3) {      // status
            printStatus(status);
        } else if (cmdType == 4) {      // run in foreground 
            printf("Running foreground process\n");
            fflush(stdout);
            handleFg(cmd, statusPtr);
        } else if (cmdType == 5) {      // run in background
            printf("Running background process\n");
            fflush(stdout);
            handleBg(cmd);
        } else if (cmdType == 6) {      // comment
            // Do nothing            
        } else if (cmdType == -1) {     // invalid
            printf("Command %s not recognized\n", cmd);
            fflush(stdout);
        } else if (cmdType == 0) {      // blank line
            // Do nothing
        }
    }
    return 0;
}