#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
    //
    // This code is given for illustration purposes. You need not include or follow this
    // strictly. Feel free to writer better or bug free code. This example code block does not
    // worry about deallocating memory. You need to ensure memory is allocated and deallocated
    // properly so that your shell works without leaking memory.


int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);
    
    if (length <= 0) {
        exit(-1);
    }
 
    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;
    
    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }

    return i;
}

int main(void)
{
    char *args[20];
    int bg;
    pid_t pid;
    int status;
    char hist[10][20][20];
    int counter = 0;
    while(1) {
        bg = 0;
        args[0] = NULL;
        int cnt = getcmd("\n>> ", args, &bg);
        //Copy the content of args in the history at hist[counter%10], overwriting existing content.
        //Run command # using the counter variable, cmd stored at counter%10 (Circular-like array)
        int j;
        for(j=0;j<cnt;j++){
            if(strlen(args[j])>=20){
                printf("Argument should not be more than 20 chars");
                exit(-1);
            }    
            strcpy(&hist[counter%10][j][0],args[j]);
            printf("%s\n",&hist[counter%10][j][0]);    
        }

        //check empty query
        if (!args[0])
            continue;

        if (strcmp(args[0], "exit") == 0)
            exit(0);

        //if args has only one string (cnt==1) and it start with !, take the following number k
        //and load hist[k%10] in args to be executed

        if ((pid = fork()) < 0) {   //create child process
            printf("Forking failed, exiting");
            exit(-1);
        }else if (pid == 0 ) {
            //child process execution
            if(execvp(*args, args) < 0) {
                printf("Execution failed, exiting");
                exit(-1);
            }
            exit(0);
        }else{
            //parent process execution
           if(bg == 0)
              wait(&status); 
        }
  
        //Set args val to 0
        int i;
        for (i=0;i<20;i++){
            args[i] = 0;
        }

        counter++;
        /* the steps can be..:
        (1) fork a child process using fork()
        (2) the child process will invoke execvp()
        (3) if background is not specified, the parent will wait,
        otherwise parent starts the next command... */
    }
}
