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

    int i, fd, amper, redirect, retid, status, error,piping = 0, argc1, pipe_buffer[2];
    char *argv[10], *argv1[10];
    int last_command_flag = 0;

    sigaction(SIGINT,&sa, NULL);

    while (1)
    {
        printf("hello: ");
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        if(strcmp(command, "!!")){
            memcpy(last_command,command,1024);
            last_command_flag = 0;
        }else
        {
            memcpy(command,last_command,1024);
            last_command_flag = 1;
        }

        /* parse command line */
        i = 0;
        token = strtok(command, " ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok(NULL, " ");
            i++;
            if(token != NULL && !strcmp(token,"|")){
                piping = 1;
                break;

            }
        }
        argv[i] = NULL;
        argc1 = i;

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        if (piping)
        {
            i = 0;
            while (token != NULL)
            {
                token = strtok(NULL," ");
                argv1[i] = token;
                i++;
            }
        argv1[i] = NULL;    
        }
        if (strcmp(argv[0], "quit") == 0)
        {
            exit(0);
        }
        /* Does command line end with & */
        if (!strcmp(argv[argc1 - 1], "&"))
        {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else
            amper = 0;

        if (!strcmp(argv[argc1 - 2], ">"))
        {
            redirect = 1;
            error = 0;
            argv[argc1 - 2] = NULL;
            outfile = argv[argc1 - 1];
        }
        else
        {
            redirect = 0;
            error = 0;
        }

        if (!strcmp(argv[argc1- 2], "2>"))
        {
            redirect = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            error = 1;
        }
        else
        {
            redirect = 0;
            error = 0;
        }
        if (strcmp(argv[0], "echo") == 0 && strcmp(argv[1], "$?") == 0)
        {

            printf("%d\n", retid);
            continue;
        }
        if (strcmp(argv[0], "echo") == 0 && argv[1][0] == '$')
        {
            item = search(argv[1]);
            printf("%s\n", item->data);
            continue;
        }
        if (argv[0][0] == '$' && strlen(argv[0]) > 1 && strcmp(argv[1], "=") == 0)
        {
            insert(argv[0], argv[2]);
        }
     
        if (last_command_flag == 1)
        {
            printf("%s\n", last_command);
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
            if(piping){
                pipe(pipe_buffer);
                if(fork() == 0){
                    close(STDOUT_FILENO);
                    dup(pipe_buffer[1]);
                    close(pipe_buffer[1]);
                    close(pipe_buffer[0]);
                    execvp(argv[0], argv);
                }
                close(STDIN_FILENO);
                dup(pipe_buffer[0]);
                close(pipe_buffer[0]);
                close(pipe_buffer[1]);
                execvp(argv1[0],argv1);
            }
            else{

            execvp(argv[0], argv);
            }
        }
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
    }
}
