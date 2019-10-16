#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
char* getLine(FILE* fp)
{
    char c;
    int n = 0;
    char* s = NULL;
    while( (c = fgetc(fp)) != '\n')
    {
        s = (char*)realloc(s,n+1);
        if(s == NULL)
        {
            free(s);
            return NULL;
        }
        else
        {
            s[n++] = c;
        }
    }
    if(n > 0)
    {
        s[n] = '\0';
    }
    return s;
}
char** convertCommand(char* command)
{
    char** args = (char**)malloc(sizeof(char*));
    if(args != NULL)
    {
        char* temp = strdup(command);
        char *pch = strtok(temp," \n");
        int i=0;
        while(pch != NULL)
        {
            args[i++] = pch;
            pch = strtok(NULL," \n");
            args = (char**)realloc(args,(i+1)*sizeof(char*));
        }
    }
    return args;
}

int findSpecial(char** args, char* c)
{
    int i=0;
    while(args[i]!= NULL)
    {
        if(strcmp(args[i],c)==0)
        {
            return i;
        }
        i++;
    }
    return -1;
}

//excuecvp pipe, REF: https://stackoverflow.com/questions/33912024/shell-program-with-pipes-in-c
int execpipe (char ** argv1, char ** argv2) {
    int fds[2];
    pipe(fds);
    int i;
    pid_t pid = fork();
    if (pid == -1) { //error
        char *error = strerror(errno);
        printf("error fork!!\n");
        return 1;
    }
    if (pid == 0) { // child process
        close(fds[1]);
        dup2(fds[0], 0);
       //close(fds[0]);
        execvp(argv2[0], argv2); // run command AFTER pipe character in userinput
    } else { // parent process
        close(fds[0]);
        dup2(fds[1], 1);
       //close(fds[1]);
        execvp(argv1[0], argv1); // run command BEFORE pipe character in userinput
    }
};


void process(char** args)
{
    int p = findSpecial(args,"&");
    
    int pipfd[2];
    pid_t id = fork();
    if(id < 0)
    {
        fprintf(stderr,"Can't create fork()\n");
    }
    else if(id == 0)
    {
        // comand has &
        if(p != -1)
        {
            args[p] = NULL;
        }
        // comand has > or < 
        int p_file;
        if((p_file = findSpecial(args,"<")) != -1)
        {
            char* input_file = args[p_file+1];
            args[p_file] = NULL;
            dup2(open(input_file, O_RDWR|O_CREAT,0777),0);
        }
        else if((p_file = findSpecial(args,">")) != -1)
        {
            char* output_file = args[p_file+1];
            args[p_file] = NULL;
            dup2(open(output_file, O_RDWR|O_CREAT,0777),1);
        }

        // command has |
        int pipe_flag =findSpecial(args,"|");
        if(pipe_flag!=-1)
        {
            char** args1 = args;
            args[pipe_flag] = NULL;
            char** args2 = args + pipe_flag + 1;
            execpipe(args1,args2);
        }
        
        if(execvp(args[0], args) < 0)
        {
            fprintf(stderr,"Invalid command\n");
        }
    }
    else
    {
        if(p == -1)
        {
            int status;
            waitpid(id, &status,0);
        }
    }
}

int main()
{
    int should_run = 1;
    char* command = NULL;
    char* history = NULL;
    char** args = NULL;
    while(should_run)
    {
        fprintf(stdout,"osh>");
        fflush(stdout);
        //read user input
        command = getLine(stdin);
        if(command != NULL)
        {
            // attack user input
            args = convertCommand(command);
            if(args != NULL)
            {
                //exit
                if(strcmp(args[0],"exit") == 0)
                {
                    kill(0, SIGTERM);
                    should_run = 0;
                }
                //history
                else if(strcmp(args[0],"!!") == 0)
                {
                    if(history == NULL)
                    {
                        fprintf(stderr,"No commands in history.\n");
                    }
                    else
                    {
                        process(convertCommand(history));
                    }
                    continue;
                }
                else
                {
                    process(args);
                }
                free(args);
            }
            history = strdup(command);
            free(command);
        }
    }
    return 0;
}