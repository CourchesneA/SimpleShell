#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>
#include <fcntl.h>
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
    char hist[10][20][30];
    int counter = 0;
    //int hcom;
    int rdloc;
    int pipeloc;
    while(1) {
        //hcom = 0;
        bg = 0;
        rdloc = -1;
        pipeloc = -1;
        args[0] = NULL;
        int cnt = getcmd("\n>> ", args, &bg);

        //HISTORY
        //Copy the content of args in the history at hist[counter%10], overwriting existing content.
        //Run command # using the counter variable, cmd stored at counter%10 (Circular-like array)
        int j;
        for(j=0;j<cnt;j++){
            if(strlen(args[j])>=30){
                printf("Argument should not be more than 30 chars");
                exit(-1);
            }    
            strcpy(&hist[counter%10][j][0],args[j]);
            //printf("%s\n",&hist[counter%10][j][0]);    
        }

        //check empty query
        if (!args[0])
            continue;

        //Built-in features
        if (strcmp(args[0], "exit") == 0)       //exit feature
            exit(0);

        if(strcmp(args[0], "pwd") == 0){        //pwd feature
            char buff[30];
            printf("%s\n",getcwd(buff,30));
            continue;   //skip execution since its done
        }

        if(strcmp(args[0], "cd") == 0){         //cd feature
            if(cnt > 2){
                printf("Too many arguments\n");
                continue;
            }
            if(chdir(args[1])==-1){
                printf("Error, could not change directory to %s\n",args[1]);
            }else
                printf("Changing dir for %s\n",args[1]);
            continue;        //skip execution since its done
        }

        //HISTORY
        //if args has only one string (cnt==1) and it start with !, take the following number k
        //and load hist[(k-1)%10] in args to be executed
        if(cnt == 1 && args[0][0] == 33){
            int k = atoi(args[0]+1);
            int m=0;
            char* w;
            if (k>counter || k<0 || k<counter-10){
                printf("No command found in history");
                continue;
            }
            while( *(w = &hist[(k-1)%10][m]) != NULL ){    //while there is still tokens
                args[m++] = w;        //make the args token point to the token in hist
            }
            printf("\n");
        }

        //Scan the token for redirect or pipe characters
        int i;
        for(i=0;i<cnt;i++){
            if(*args[i]==62){
                rdloc = i;
                break;      //we will only handle simple cases (one redirection or pipe)
            }
            if(*args[i]==124){
                pipeloc = i;
                break;      //we will only handle simple cases (one redirection or pipe)
            }
        }
        //Forking for process execution
        if ((pid = fork()) < 0) {   //create child process
            printf("Forking failed, exiting");
            exit(-1);
        }else if (pid == 0 ) {
            //child process execution

            //Check if there is a redirection char (rdloc != -1)
            if(rdloc != -1){
                char* lbuf[20];
                for(i=0;i<rdloc;i++){   //create LHS buffer
                    lbuf[i]=args[i];
                    printf("T: %s\n",lbuf[i]);
                }
                //for output redirection we dont have to save STDOUT because its the child process
                //and it will die soon after
                close(1);
                open(args[rdloc+1],O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                //printf("T: %s\n",args[rdloc+1]);
                execvp(*lbuf,lbuf);
            }else if(pipeloc != -1){    //found a pipe character
                char* lbuf[20];
                char* rbuf[20];
                for(i=0;i<pipeloc;i++){
                    lbuf[i] = args[i];      //create LHS buffer
                }
                for(i=pipeloc+1;i<cnt;i++){
                    rbuf[i-(pipeloc+1)] = args[i];      //create RHS buffer
                }
                
                //Here another fork create the pipe for both commands
                int pfd[2];
                pipe(pfd);
                pid_t ccpid;
                if((ccpid=fork())==0){
                    //child-child process
                    close(pfd[1]);
                    close(0);   //close stdin
                    dup(pfd[0]);    //copy read-end of pipe to input file descriptor
                    //printf("Child-Child evaluating: %s\nwith input from pipe \n",*rbuf);
                    printf("cc exec result : %i\n",execvp(*rbuf,rbuf));
                    /*
                    if(execvp(*rbuf,rbuf) < 0){
                        printf("Execution of child-child failed\n");
                        exit(-1);
                    }*/
                }else{
                    //child process
                    close(pfd[0]);
                    close(1);   //close stdout
                    dup(pfd[1]); //copy write-end of the pipe to output file descriptor
                    printf("Result = %d\n",execvp(*lbuf,lbuf));
                    /*
                    if(execvp(*lbuf,lbuf) < 0){
                        //printf("Execution of child failed\n");
                        exit(-1);
                    }*/
                    printf("ccpid : %d\n",ccpid);
                    waitpid(ccpid,&status,0);
                }

            }else if(execvp(*args, args) < 0) {
                printf("Execution failed: %s\n",args[0]);
                exit(-1);
            }
            exit(0);
        }else{
            //parent process execution
           if(bg == 0){
               //printf("parent waiting\n");
               waitpid(pid,&status,0);
           }
        }
  
        //Set args val to 0
        for (i=0;i<20;i++){
            args[i] = 0;
        }

        counter++;
    }
}
