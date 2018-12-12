/* 
 * mysh.c
 * Trey Oehmler
 * CS315 Assignment 5 Fall 2018
 */

#include "mysh.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
	// Run loop to input commands
	tosh_loop();

	// TODO: shutdown / cleanup

	return 0;
}


void tosh_loop(void) 
{
	/* executes loop to read & execute commands */

	char *input;
	char **args, **progs;
	int status = 1;

	prog_count = 0;

	while (status) {
		int out_fd, in_fd;
		printf("tosh> ");
		input = get_input();
		
		// check for EOF from getline()
		if (!input) {
			status = 0;
			printf("\n");
			break;
		}
	
		progs = parse_progs(input); // parse_progs() sets prog_count

		

		if (prog_count < 1) {
			printf("parse_progs() error \n"); // malloc() err w/in parse_progs()
		} else if (prog_count == 1) {

			//check for both output and input redirection
			args = parse_args(progs[0]);
			out_fd = parse_output(args);
			in_fd = parse_input(args);
			status = execute_command(args, in_fd, out_fd);
			free(args);

		} else { // need to use pipes
		
			/* setting up pipes: 
			 * pipe_fd[0] = read end of progs[1]
			 * pipe_fd[1] = write_end of progs[0] ... etc */
			int pipe_count = prog_count - 1;			
			for (int i = 0; i < pipe_count; i ++) {
				pipe(pipe_fd + (i * 2));
			}

			/* parse each program into args :
			 * set in / out fd from pipe_fd[] or by parsing
			 * execute each program (break on status <= 0) */
			int current = 0;

			while (status && (current < prog_count)) {
				printf("program count: %d\n", prog_count);
				args = parse_args(progs[current]);

				print_args(args);
				// assume non-contradictory redirection 
				if (current == 0) {
					// first prog
					in_fd = parse_input(args);
					out_fd = pipe_fd[1];
				} else if (current == (prog_count - 1)) {
					// last prog
					in_fd = pipe_fd[(2 * current) - 2];
					out_fd = parse_output(args);
					printf("last\n");
				} else {
					// input & ouput redirection w/ pipes
					in_fd = pipe_fd[(2 * current) - 2];
					out_fd = pipe_fd[(2 * current) + 1];
				}
				
				status = execute_command(args, in_fd, out_fd);
				current ++;
				free(args);
			}
		} 
		free(input);
		free(progs);
		prog_count = 0;
	}
}

void close_pipes() 
{
	/* closes all open pipes */
	int pipe_count = prog_count - 1;
	for (int i = 0; i < (2 * pipe_count); i ++) {
		printf("close: %d\n", pipe_fd[i]);
		close(pipe_fd[i]);
	}
}

void print_progs(char **progs) 
{
	for (int i = 0; progs[i]; i++) {
		printf("prog %d: %s", i, progs[i]);
	}
}

void print_args(char **args) 
{
	for (int i = 0; args[i]; i++) {
		printf("arg %d: %s\n", i, args[i]);
	}
}

char *get_input(void)
{	
	/* read input from stdin using getline() */

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
		return NULL;
	}
	return line_buf;
}

char **parse_progs(char *input) 
{
	/* splits input into progs based on pipes */

	char **progs = malloc(sizeof(char *) * MAX_PROG_SIZE);

	if (!progs) {
		fprintf(stderr, "malloc error\n");
		return NULL;
	}

	int position = 0;
	char *token = strtok(input, "|");
	while (token) {
		progs[position] = token;
		position ++;
		prog_count ++;
		token = strtok(NULL, "|");
	}

	progs[position] = NULL;
	return progs;
}	

char **parse_args(char *input)
{
	/* split arguments based on delimiters */

	char **args = malloc(sizeof(char *) * ARG_SIZE);

	if (!args) // check for allocation error
	{
		fprintf(stderr, "malloc error\n");
		return NULL;
	}

	int position = 0;
	char *token = strtok(input, " \n");
	
	while (token != NULL)
	{
		size_t length = strlen(token);
		if (token[length - 1] == '\n')
		{
			token[length - 1] = '\0';
		}
		args[position] = token;
		position ++;
		token = strtok(NULL, " \n");
	}

	args[position] = NULL;
	return args;
}

int parse_output(char **args) 
{
	/* checks for output redirection
	 * opens file and returns fd 
	 * returns -1 on error or if no output redirection*/
	
	int current = 0;
	char *cmp = ">";

	while (args[current]) {
		if (strcmp(args[current], cmp) == 0) {
			// should check to make sure args[current+1] != NULL
			int fd = open(args[current + 1], O_WRONLY | O_CREAT, 0644);
			if (fd < 0) {
				perror("open");
			}
			args[current] = NULL;
			return fd;
			//dup2(fd, 1); // make stdout got to the file
			//close(fd);
		} else if (!args[current + 1] && args[current + 2]) {
			current += 2;
		} else {
			current ++;
		}
	}
	return -1;
}

int parse_input(char **args)
{
	/* checks for input redirection 
	 * sets ptr to < / > operator to NULL 
	 * opens file for input, returns fd 
	 * returns -1 on error or if no input redirection */

	int current = 0;
	char *cmp = "<";

	while (args[current]) {
		if (strcmp(args[current], cmp) == 0) {
			// should check to make sure args[current+1] != NULL 
			int fd = open(args[current + 1], O_RDONLY, 0644);
			if (fd < 0) {
				perror("open: file doesn't exist");
			}
			args[current] = NULL;
			return fd;
		} else if (!args[current + 1] && args[current + 2]) {
			current += 2;
		} else {
			current ++;
		}
	}
	return -1;
}

int execute_command(char **args, int in_fd, int out_fd)
{
	/* checks if command is a built-in
	 * launchs new process otherwise
	 * retuns 1 on sucess, 0 on err / termination */

	if (!args[0]) // empty command
	{
		return tosh_help(args);
	}
	for (int i = 0; i < builtin_size(); i++)
	{
		if ((strcmp(args[0], builtin_str[i])) == 0)
		{
			return (*builtin_func[i])(args);
		}
	}
	return launch_process(args, in_fd, out_fd);
}

int launch_process(char **args, int in_fd, int out_fd)
{
	// NEED TO CLEAR ERROR 

	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) // child process
	{
		if (in_fd > -1) {
			dup2(in_fd, 0); // replace stdin w in_file
		}
		if (out_fd > -1) {
			dup2(out_fd, 1); // replace stdout w out_file
		}
		close_pipes();
		if (execvp(args[0], args) < 0){
			perror("execv error");
		}
	}
	else if (pid < 0) // error forking 
	{
		perror("execute_command error forking");
	}
	else // parent process 
	{
		close_pipes();
		do 
		{	
			
			waitpid(pid, &status, WUNTRACED);
		} 
		while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

/* ---------------- BUILTINS ---------------- */

int tosh_pwd(char **args)
{
	/* prints current working directory */

	char cwd[256];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("getcwd error()");
	} 
	else 
	{
		printf("%s\n", cwd);
	}
	return 1;
}


int tosh_cd(char **args)
{ 
	/* changes cwd bassed on path passed to cd 
	 * assumes a path is given, otherwise no action */

	if (args[1]) // consider adding message to say argument needed?
	{
		if (chdir(args[1]) < 0)
		{
			perror("tosh");
		}
	}
	return 1;
}


int tosh_help(char **args)
{
	/* prints built in functions */
	printf("------------------------------------------------\n");
	printf("tosh | Trey Oehmler | Middlebury CS315 Fall 2018\n");
	printf("------------------------------------------------\n");
	printf("The following functions are built in:\n");
	
	for (int i = 0; i < builtin_size(); i++)
	{
		printf(" - %s()\n", builtin_str[i]);
	}

	printf("\n");
	return 1;
}


int tosh_exit(char **args)
{
	return 0;
}


int builtin_size(void)
{
	return sizeof(builtin_str) / sizeof(char *);
}
