#include "usefull_trash.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int compare (const void * a, const void * b)
{
    if ( *(int*)a <  *(int*)b ) return -1;
    else if ( *(int*)a == *(int*)b ) return 0;
    else if ( *(int*)a >  *(int*)b ) return 1;
    return -2;
}

/* this function forks the parent n-1 times,
* parent is the same for every forked process
* return number of child. n - parent */
int fork_n_times(int n)
{
    int i,times=0;
    pid_t fork_key=getpid();
    for(i=0;i<n-1;i++)
    {
        if(getpid()==fork_key)
        {
            times++;
            int fk=fork();
            throwing_fork_errors(fk);
        }
    }
    if(getpid()==fork_key) times++;
    return times;
}
/* this function scans string til the first\n*/

void fork_exec_with_inp_redir2(char *for_exe,int flag, const char * file)
{
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        if(flag){
            int fd=open(file, O_RDWR | O_TRUNC | O_CREAT, S_IWUSR | S_IROTH | S_IWGRP | S_IRUSR);
            dup2(fd,1);
            //close(1); //dont know if we need this
        }
        int df = system(for_exe);
        fflush(NULL);
        exit(df);

    }
    else if (pid < 0)
    {
       throwing_fork_errors(pid);
    }
    else
    {
        while(wait((int*)NULL) != pid);
    }
}

void fork_exec_with_inp_redir(char **for_exe,int flag, const char * file)
{
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        if(flag){
            int fd=open(file, O_RDWR | O_TRUNC | O_CREAT, S_IWUSR | S_IROTH | S_IWGRP | S_IRUSR);
            dup2(fd,1);
            //close(1); //dont know if we need this
        }
        if(execvp(for_exe[0], &for_exe[1])==-1)
        {
            exit(-2);
        }

    }
    else if (pid < 0)
    {
       throwing_fork_errors(pid);
    }
    else
    {
        while(wait((int*)NULL) != pid);
    }
}

void throwing_pipe_errors(int fd)
{
    if(fd==-1)
    {
        printf("Unable to open pipe\n");
        exit(EXIT_FAILURE);
    }

}

void throwing_fork_errors(int fk)
{
    if(fk==-1)
    {
        printf("Unable to fork\n");
        exit(EXIT_FAILURE);
    }
}
