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

typedef struct DataItem
{
    int key;
    char *data;
} DataItem;

DataItem *hashArray[SIZE];
DataItem *dummyItem;
DataItem *item;

int hashCode(char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % SIZE;
}

void insert(char *key, char *data)

{
    DataItem *item = (DataItem *)malloc(sizeof(struct DataItem));
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
DataItem *search(char *key)
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

int cd(char *pth)
{
    char path[SIZE];
    strcpy(path, pth);

    char cwd[SIZE];
    if (pth[0] != '/')
    {
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, "/");
        strcat(cwd, path);
        chdir(cwd);
    }
    else
    {
        chdir(pth);
    }
    return 0;
}
int existRedirection(char *arg[100])
{
    int i = 0;
    while(arg){
        printf("%s", *arg);
        *arg++;
    }
    return 0;
}

void loop_pipe(char *argv[10][SIZE], int numberOfPipes)
{
    int status;
    pid_t pid;
    int i = 0;

    int prev_pipe, pipefds[2];
    prev_pipe = STDIN_FILENO;

    for (i = 0; i < numberOfPipes; i++)
    {
        if (pipe(pipefds) < 0)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0) /*Child proccess*/
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

            execvp(argv[i][0], argv[i]);
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
    execvp(argv[i][0], argv[i]);
}

void handler(int sig)
{
    printf("You typed Control-C!\n");
}

int main()
{
    char directory[SIZE];
    char prompt[SIZE] = "hello:";
    struct sigaction sa;
    sa.sa_handler = &handler;
    sa.sa_flags = SA_RESTART;

    dummyItem = (struct DataItem *)malloc(sizeof(struct DataItem));
    dummyItem->key = -1;
    dummyItem->data = NULL;

    char command[1024], last_command[1024] = "";
    char *token;
    char *outfile;

    int i, fd, amper, redirect, retid, status, error, piping = 0, argc1;
    char *argv[10][100];
    int last_command_flag = 0, number_of_pipes;

    sigaction(SIGINT, &sa, NULL);

    while (1)
    {
        number_of_pipes = 0;
        printf("%s ", prompt);
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
        // insertCommand(argv[number_of_pipes]);
        // printCmd(head);
        /* Is command empty */
        if (argv[0][0] == NULL)
        {
            clearerr(stdin);
            continue;
        }
        if (existRedirection(*argv))
        {
            printf("Exist");
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
            prompt[0] = '\0';
            strcpy(prompt, argv[0][2]);
            int i = 3;
            while (argv[0][i])
            {
                strcat(prompt, " ");
                strcat(prompt, argv[0][i]);
                i++;
            }
            prompt[strlen(prompt)] = '\0';
            clearerr(stdin);
            continue;
        }
        if (!strcmp(argv[0][0], "cd") && argv[0][1])
        {
            int i = 2;
            strcpy(directory, argv[0][1]);
            while (argv[0][i])
            {
                strcat(directory, " ");
                strcat(directory, argv[0][i]);
                i++;
            }
            cd(directory);
            clearerr(stdin);
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

        if (argc1 > 1 && (!strcmp(argv[number_of_pipes][argc1 - 2], ">") || !strcmp(argv[number_of_pipes][argc1 - 2], ">>") || !strcmp(argv[number_of_pipes][argc1 - 2], "2>")))
        {
            if (!strcmp(argv[number_of_pipes][argc1 - 2], ">"))
            {
                redirect = 1;
                error = 0;
            }
            else if (!strcmp(argv[number_of_pipes][argc1 - 2], ">>"))
            {
                redirect = 2;
                error = 0;
            }
            else
            {
                redirect = 1;
                error = 1;
            }
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
        }
        else
        {
            redirect = 0;
            error = 0;
        }

        if (argc1 > 1 && argv[number_of_pipes][argc1 - 2] && !strcmp(argv[number_of_pipes][argc1 - 2], "<"))
        {
            argv[0][argc1 - 2] = NULL;
            outfile = argv[0][argc1 - 1];
            redirect = 3;
            error = 0;
        }
        if (number_of_pipes == 0)
        {
            if (strcmp(argv[number_of_pipes][0], "echo") == 0 && strcmp(argv[number_of_pipes][1], "$?") == 0)
            {
                printf("%d\n", retid);
                clearerr(stdin);
                continue;
            }
            if (strcmp(argv[number_of_pipes][0], "echo") == 0 && argv[number_of_pipes][1][0] == '$')
            {
                item = search(argv[0][1]);
                if (item)
                {
                    printf("%s\n", item->data);
                }
                else
                {
                    printf("error: No such argument\n");
                }
                clearerr(stdin);
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

            if (redirect == 3)
            {
                fd = open(outfile, O_RDONLY, 0777);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            else if (redirect == 2)
            {
                fd = open(outfile, O_APPEND | O_CREAT | O_WRONLY, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            /* redirection of IO ? */
            else if (redirect == 1 && !error)
            {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            else if (redirect == 1 && error)
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

            else if (number_of_pipes > 0)

                loop_pipe(argv, number_of_pipes);
        }
        if (amper == 0)
            retid = wait(&status);
    }
    /* parent continues here */
}
