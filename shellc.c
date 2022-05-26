#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 100

typedef struct Command
{
    char **arguments;
    struct Command *next;
} cmdLine;

struct DataItem
{
    int key;
    char *data;
};
struct Command *head = NULL;
struct Command *curr;

void printCmd(cmdLine *dummy)
{
    int i = 0;
    while (dummy)
    {
        while (dummy->arguments[i])
        {
            printf("%s ", dummy->arguments[i++]);
            fflush(stdout);
        }
        i = 0;
        printf("-> ");
        fflush(stdout);

        dummy = dummy->next;
    }
    printf("null\n");
}

void insertCommand(char **arg)
{
    int i = 0;
    struct Command *cmd = (struct Command *)malloc(sizeof(struct Command));
    cmd->arguments = (char **)malloc(sizeof(arg));
    while (*arg)
    {
        cmd->arguments[i] = (char *)malloc(strlen(*arg));
        memcpy(cmd->arguments[i++], *arg, strlen(*arg));
        arg++;
    }
    if (!head)
    {
        cmd->next = NULL;
        head = cmd;
    }
    else
    {
        curr = head;
        while (curr->next)
        {
            curr = curr->next;
        }
        cmd->next = NULL;
        curr->next = cmd;
    }
}

struct DataItem *hashArray[SIZE];
struct DataItem *dummyItem;
struct DataItem *item;

int hashCode(char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % SIZE;
}
int cd (char *pth){
    char path[SIZE];
    strcpy(path,pth);

    char cwd[SIZE];
    if(pth[0] != '/'){
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,path);
        chdir(cwd);
    }else{
        chdir(pth);
    }
    return 0;
}

void loop_pipe(cmdLine *commands, int numberOfPipes)
{
    int status;
    pid_t pid;
    int i = 0;

    int prev_pipe, pipefds[2];
    prev_pipe = STDIN_FILENO;

    for (i = 0; i < numberOfPipes; i++)
    {
        if (i > 0)
        {
            
            commands = commands->next;
        }
        if (pipe(pipefds) < 0)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0)
        {
            if (prev_pipe != STDIN_FILENO)
            {
                if (dup2(prev_pipe, STDIN_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe);
            }
            dup2(pipefds[1], STDOUT_FILENO);
            close(pipefds[1]);

            execvp(*commands->arguments, commands->arguments);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        close(prev_pipe);
        close(pipefds[1]);
        prev_pipe = pipefds[0];
    }
    if (prev_pipe != STDIN_FILENO)
    {
        dup2(prev_pipe, STDIN_FILENO);
        close(prev_pipe);
    }
    execvp(*commands->arguments, commands->arguments);
}

void insert(char *key, char *data)

{
    struct DataItem *item = (struct DataItem *)malloc(sizeof(struct DataItem));
    item->key = hashCode(key);
    item->data = (char *)malloc(sizeof(strlen(data)));
    memcpy(item->data, data, strlen(data));

    int hashIndex = item->key;

    while (hashArray[hashIndex] != NULL && hashArray[hashIndex]->key != -1)
    {
        ++hashIndex;
        hashIndex %= SIZE;
    }
    hashArray[hashIndex] = item;
}
struct DataItem *search(char *key)
{
    int hashIndex = hashCode(key);

    while (hashArray[hashIndex] != NULL)
    {
        if (hashArray[hashIndex]->key == hashIndex)
        {
            return hashArray[hashIndex];
        }
        ++hashIndex;
        hashIndex %= SIZE;
    }
    return NULL;
}

void handler(int sig)
{
    printf("You typed Control-C!\n");
}

int main()
{
    char promopt[SIZE] = "hello:";
    struct sigaction sa;
    sa.sa_handler = &handler;
    sa.sa_flags = SA_RESTART;

    dummyItem = (struct DataItem *)malloc(sizeof(struct DataItem));
    dummyItem->key = -1;
    dummyItem->data = NULL;

    char command[1024], last_command[1024];
    char *token;
    char *outfile;

    int i, fd, amper, redirect, retid, status, error, piping = 0, argc1;
    char *argv[10][10];
    int last_command_flag = 0, number_of_pipes;

    sigaction(SIGINT, &sa, NULL);

    while (1)
    {
        number_of_pipes = 0;
        printf("%s: ", promopt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        if (strcmp(command, "!!"))
        {
            memcpy(last_command, command, 1024);
            last_command_flag = 0;
        }
        else
        {
            memcpy(command, last_command, 1024);
            last_command_flag = 1;
        }

        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL)
        {
            if (token != NULL && !strcmp(token, "|"))
            {
                argv[number_of_pipes][i] = NULL;
                insertCommand(*argv);
                number_of_pipes++;
                i = 0;
                token = strtok(NULL, " ");
                continue;
            }

            argv[number_of_pipes][i] = token;
            token = strtok(NULL, " ");
            i++;
            argc1 = i;
        }

        argv[number_of_pipes][i] = NULL;
        insertCommand(argv[number_of_pipes]);
        // printCmd(head);
        /* Is command empty */
        if (argv[0][0] == NULL)
        {
            continue;
        }

        if (number_of_pipes == 0)
        {
            argc1 = i;
            argv[number_of_pipes][argc1] = NULL;
        }

        if (strcmp(argv[0][0], "quit") == 0)
        {
            exit(0);
        }
        if (!strcmp(argv[0][0], "prompt") && !strcmp(argv[0][1], "=") && argv[0][2])
        {
            promopt[0] = '\0';
            strcpy(promopt,argv[0][2]);
            int i = 3;
            while (argv[0][i])
            {
                strcat(promopt, " ");
                strcat(promopt, argv[0][i]);
                i++;
            }
            promopt[strlen(promopt)] = '\0';
            continue;
        }
        if(!strcmp(argv[0][0], "cd") && argv[0][1]){
            cd(argv[0][1]);
            continue;
        }
        /* Does command line end with & */
        if (!strcmp(argv[number_of_pipes][argc1 - 1], "&"))
        {
            amper = 1;
            argv[0][argc1 - 1] = NULL;
        }
        else
            amper = 0;

        if (argc1 > 1 && !strcmp(argv[number_of_pipes][argc1 - 2], ">"))
        {
            redirect = 1;
            error = 0;
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
        }
        else
        {
            redirect = 0;
            error = 0;
        }

        if (argc1 > 2 && !strcmp(argv[number_of_pipes][argc1 - 2], "2>"))
        {
            redirect = 1;
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
            error = 1;
        }
        else
        {
            redirect = 0;
            error = 0;
        }
        if (number_of_pipes == 0)
        {
            if (strcmp(argv[number_of_pipes][0], "echo") == 0 && strcmp(argv[number_of_pipes][1], "$?") == 0)
            {
                printf("%d\n", retid);
                continue;
            }
            if (strcmp(argv[number_of_pipes][0], "echo") == 0 && argv[number_of_pipes][1][0] == '$')
            {
                item = search(argv[0][1]);
                printf("%s\n", item->data);
                continue;
            }
            if (argv[number_of_pipes][0][0] == '$' && strlen(argv[number_of_pipes][0]) > 1 && strcmp(argv[number_of_pipes][1], "=") == 0)
            {
                insert(argv[0][0], argv[0][2]);
            }

            if (last_command_flag == 1)
            {
                printf("%s\n", last_command);
            }
        }
        /* for commands not part of the shell command language */
        if (fork() == 0)
        {
            /* redirection of IO ? */
            if (redirect && !error)
            {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (redirect && error)
            {
                fd = creat(outfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
            }
            if (number_of_pipes == 0)
            {
                execvp(argv[0][0], argv[0]);
            }

            else if (number_of_pipes > 0){
                loop_pipe(head, number_of_pipes);
            }
        }
        if (amper == 0)
            retid = wait(&status);
    }
    /* parent continues here */
}
