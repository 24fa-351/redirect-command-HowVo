#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *findAbsolutePath(char *cmd)
{
    char *path = getenv("PATH");
    if (!path)
        return NULL;

    char *pathCopy = strdup(path); // Copy and modify this string so we don't mess up the $PATH variable
    char *dir;

    for (dir = strtok(pathCopy, ":"); dir; dir = strtok(NULL, ":"))
    {
        char fullPath[200];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir, cmd);
        if (access(fullPath, X_OK) == 0)
        {
            free(pathCopy);
            return strdup(fullPath); // Return the first found path
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Not enough arguments! Usage: ./redir <inp> <cmd> <out>");
        exit(EXIT_FAILURE);
    }

    char *inp = argv[1];
    char *out = argv[3];

    char **newargv = (char **)malloc(sizeof(char *) * (1 + argc - 2));
    for (int i = 2; i < argc; i++)
    {
        newargv[i - 2] = argv[i];
    }
    newargv[argc - 2] = NULL;
    char *commandPath = findAbsolutePath(newargv[0]);
    printf("Absolute path %s\n", commandPath);
    printf("newargv %s %s %s\n", newargv[0], newargv[1], newargv[2]);

    pid_t childPid = fork();

    if (childPid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    else if (childPid == 0) // in child process
    {
        int fdIn, fdOut;

        if (strcmp(inp, '-') != 0)
        {
            fdIn = open(inp, O_RDONLY, 0644);

            if (fdIn == -1)
            {
                perror("Can't open input file");
                exit(EXIT_FAILURE);
            }

            if (dup2(fdIn, STDIN_FILENO) == -1)
            {
                perror("Can't redirect stdin");
                exit(EXIT_FAILURE);
            }
            close(fdIn);
        }

        if (strcmp(out, "-") != 0)
        {
            fdOut = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fdOut == -1)
            {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fdOut, STDOUT_FILENO) == -1)
            {
                perror("Error redirecting stdout");
                exit(EXIT_FAILURE);
            }
            close(fdOut);
        }

        char *commandPath = findAbsolutePath(newargv[0]);

        if (!commandPath)
        {
            fprintf(stderr, "Command not found: %s\n", newargv[0]);
            exit(EXIT_FAILURE);
        }
        execve(commandPath, newargv, NULL);

        wait(NULL);
        return 0;
    }
}