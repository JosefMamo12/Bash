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

char *substr(const char *src, int m, int n)
{
    // get the length of the destination string
    int len = n - m;

    // allocate (len + 1) chars for destination (+1 for extra null character)
    char *dest = (char *)malloc(sizeof(char) * (len + 1));

    // extracts characters between m'th and n'th index from source string
    // and copy them into the destination string
    for (int i = m; i < n && (*(src + i) != '\0'); i++)
    {
        *dest = *(src + i);
        dest++;
    }

    // null-terminate the destination string
    *dest = '\0';

    // return the destination string
    return dest - len;
}

struct DataItem
{
    int key;
    char *data;
};

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

void loop_pipe(char *cmd[10][10])
{
    int p[2];
    pid_t pid;
    int fd_in = 0;

    while (*cmd != NULL)
    {
        pipe(p);
        if ((pid = fork()) == -1)
        {
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            dup2(fd_in, 0);
            if (*(cmd + 1) != NULL)
                dup2(p[1], 1);
            close(p[0]);
            execvp(*(cmd)[0], *cmd);
            exit(EXIT_FAILURE);
        }
        else
        {
            wait(NULL);
            close(p[1]);
            fd_in = p[0];
            cmd++;
        }
    }
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
    char *argv[10][10], *argv1[10];
    int last_command_flag = 0, number_of_pipes = 0;

    sigaction(SIGINT, &sa, NULL);

    while (1)
    {
        printf("hello: ");
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
            argv[number_of_pipes][i] = token;
            token = strtok(NULL, " ");
            i++;

            if (token != NULL && !strcmp(token, "|"))
            {
                number_of_pipes++;
                i = 0;
                continue;
            }
            argc1 = i;
        }
        argv[number_of_pipes][i] = NULL;
        /* Is command empty */
        if (argv[0][0] == NULL)
            continue;

        if (number_of_pipes == 0)
        {
            argc1 = i;
            argv[number_of_pipes][argc1] = NULL;
        }

        if (strcmp(argv[0][0], "quit") == 0)
        {
            exit(0);
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

            if (number_of_pipes > 0)
            {

                loop_pipe(argv);
            }
        }
        if (amper == 0)
            retid = wait(&status);
    }
    /* parent continues here */
}
