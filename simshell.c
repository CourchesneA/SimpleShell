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
    int i;
    //int hcom;   //history command
    int rdloc;
    int pipeloc;
    //now initializing pointer to jobs array to share between processes
    int jarr[15];
    int *jarrp;
    jarrp = jarr;
    int jarrc=0;
    int *jarrcp;
    jarrcp = &jarrc;

    for(i=0;i<15;i++){  //initialize jobs array with -1
        jarr[i] = -1;
    }
    int exitval;
    while(1) {
        //hcom = 0;
        bg = 0;
        rdloc = -1;
        pipeloc = -1;
        args[0] = NULL;
        int cnt = getcmd("\n>> ", args, &bg);
        exitval = 0;

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

        if (strcmp(args[0], "jobs") == 0){       //jobs feature
            int ans;
            for(i = 0; i<jarrc; i++){
                if(jarr[i] == -1)
                    continue;
                //printf("Waitpid for %d\n",jarr[i]);
                ans = waitpid(jarr[i],&status,WNOHANG);   //"Ping" a process
                if( ans > 0 ){   //pid has terminated, remove it
                    jarr[i] = -1;
                    for(j=i; j<14;j++){
                        jarr[j]=jarr[j+1];
                    }
                    jarrc--;
                }else if( ans == 0 ){  //pid is running, print it
                    printf("%d. PID = %d\n",i+1,jarrp[i]);
                }else if( ans < 0){  //process does not exists
                    //printf("Should not happen: pid was %d\n",jarr[i]);
                    perror("Waitpid error\n");
                }
            }
            continue;
        }

        if (strcmp(args[0], "fg") == 0 ){   //fg feature
            int jnum = 1;
            if( cnt > 1 ){
                jnum = atoi(args[1]);
            }
            waitpid(jarr[jnum-1],&status,0);
        }

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
            //hcom = 1;
            int k = atoi(args[0]+1);
            int m=0;
            char* w;
            if (k>counter || k<0 || k<counter-10){
                printf("No command found in history");
                continue;
            }
            while( *(w = &hist[(k-1)%10][m]) != NULL ){    //while there is still tokens
                printf("%s ",w);
                args[m++] = w;        //make the args token point to the token in hist
            }
            printf("\n");
        }

        //Scan the token for redirect or pipe characters
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
                    *(jarrp+(*jarrcp)++)=getpid();
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
                    //printf("ccpid : %d\n",ccpid);
                    waitpid(ccpid,&status,0);
                }

            }else if(execvp(*args, args) < 0) {
                printf("Execution failed: %s\n",args[0]);
                exitval =-1;
            }
            //remove this process from the jobs
            /*
            printf("Looking for process in %d processes\n",*jarrcp);
            for(i=0; i<*jarrcp;i++){
                if(*(jarrp+i) == getpid()){
                    printf("Removing process:%d\n",getpid());
                    *(jarrp+i)=-1;
                    (*jarrcp)--;
                    break;
                }
            } */   
            exit(exitval);
        }else{
            //parent process execution
           
            printf("Adding process: %d\n",pid);
            *(jarrp+(*jarrcp)++)=pid;

           if(bg == 0){
               //printf("parent waiting\n");
               waitpid(pid,&status,0);
               //process has terminated, remove it from the job list

               for(i=0;i<15;i++){
                   if(jarr[i]==pid){
                       printf("Removing process %d\n", pid);
                       jarr[i] = -1;
                       for(j=i; j<14;j++){
                           jarr[j]=jarr[j+1];
                       }
                   }
               }
               jarrc--;
           }
        }
  
        //Set args val to 0
        for (i=0;i<20;i++){
            args[i] = 0;
        }

        counter++;
    }
}
