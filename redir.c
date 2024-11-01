#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void parseCmd(char *cmd, char **args)
{ // Parse the command into an array of arguments
    char *token = strtok(cmd, " ");
    int i = 0;
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

char *findAbsolutePath(char *cmd) // Find the absolute path of the command
{
    if (cmd[0] == '/')
        return strdup(cmd);
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
    free(pathCopy);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Not enough arguments! Usage: ./redir <inp> <cmd> <out>");
        exit(EXIT_FAILURE);
    }

    char *inp = argv[1];
    char *cmd = argv[2];
    char *out = argv[3];

    char **args = (char **)malloc(sizeof(char *) * (1 + argc - 2));
    parseCmd(cmd, args);
    char *commandPath = findAbsolutePath(args[0]);

    if (!commandPath)
    {
        fprintf(stderr, "Command not found %s\n", args[0]);
        exit(EXIT_FAILURE);
    }

    pid_t childPid = fork();

    if (childPid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    else if (childPid == 0) // In child process
    {
        if (strcmp(inp, "-") != 0)
        {
            int fdin = open(inp, O_RDONLY, 0644);
            dup2(fdin, STDIN_FILENO);
            close(fdin);
        }

        if (strcmp(out, "-") != 0)
        {
            int fdout = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fdout, STDOUT_FILENO);
            close(fdout);
        }
        execve(commandPath, args, NULL);
    }

    wait(NULL);
    return 0;
}