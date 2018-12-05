/* 
 * mysh.c
 * Trey Oehmler
 * CS315 Assignment 5 Fall 2018
 */

#include "mysh.h"

int
main(int argc, char *argv[])
{
    char *input;
	char **args;

	printf("tosh> ");
	input = get_input();
	args = parse_args(input);

	//printf("input was: %s\n", input);
	//printf("args were: %s\n", *(args+1));

	execute_command(args);
	return 0;
}

static int execute_command(char **args)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) // child process
	{
		if (execvp(args[0], args) < 0)
		{
			perror("execv error");
		}
	}
	else if (pid < 0) // error forking 
	{
		perror("execute_command error forking");
	}
	else // parent process 
	{
		do 
		{
			waitpid(pid, &status, WUNTRACED);
		} 
		while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}


static char **parse_args(char *input)
{
	char **args = malloc(sizeof(char *) * ARG_SIZE);

	if (!args) // check for allocation error
	{
		fprintf(stderr, "malloc error\n");
		return NULL;
	}

	int position = 0;
	char *token = strtok(input, " ");
	
	while (token != NULL)
	{
		size_t length = strlen(token);
		if (token[length - 1] == '\n')
		{
			token[length - 1] = '\0';
		}
		args[position] = token;
		position ++;
		token = strtok(NULL, " ");
	}

	args[position] = NULL;
	return args;
}

static char *get_input(void)
{	
	size_t input_size = INPUT_BYTES;
	char *line_buf = malloc(sizeof(char *) * input_size);

	if (!line_buf) // check for allocation error
	{
		fprintf(stderr, "malloc error\n");
		return NULL;
	}

	ssize_t bytes_read = getline(&line_buf, &input_size, stdin);

	if (bytes_read < 0)
	{
		printf("getline error \n");
		return NULL;
	}

	return line_buf;
}
