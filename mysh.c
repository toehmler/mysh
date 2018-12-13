/* 
 * mysh.c
 * Trey Oehmler
 * CS315 Assignment 5 Fall 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT_BYTES  4096
#define ARG_SIZE     11
#define MAX_PROGS    100

void tosh_loop(void);
char *get_input(void);
char **parse_progs(char *input);
int count_progs(char **progs);
char **parse_args(char *prog);
int parse_io(char **args, int flag);
int is_builtin(char **args);
int builtin_size(void);
int tosh_cd(char **args);
int tosh_pwd(char **args);
int tosh_help(char **args);
int tosh_exit(char **args);

char *builtin_str[] = {
	"help",
	"cd",
	"pwd",
	"exit"
};

int (*builtin_func[]) (char **) = {
     &tosh_help,
     &tosh_cd,
     &tosh_pwd,
     &tosh_exit
};

int main(int argc, char *argv[])
{
	tosh_loop();
	return 0;
}

void tosh_loop(void) 
{
	/* executes loop to read & execute commands */
	int status = 1;
	while (status) {

		int out_fd, in_fd, prog_count, pid, p_status;
		char *input;
		char **args, **progs;
		
        // tosh: trey oehmler shell
		printf("tosh> ");
		input = get_input();
		
		// check for EOF from getline()
		if (!input) {
			status = 0;
			printf("\n");
			break;
		}
	
		progs = parse_progs(input); // init progs
		if (!progs) { // malloc error within parse_progs()
			continue; // goto next loop iteration
		}
		prog_count = count_progs(progs); 

		// save in / out
		int tmp_in = dup(0);
		int tmp_out = dup(1);
		
		// init args for first program 
		args = parse_args(progs[0]);
		
		// check for input redirection
		in_fd = parse_io(args, 0);
		if (in_fd < 0) {
			in_fd = dup(tmp_in);
		}

        // loop through programs & execute
		for (int i = 0; i < prog_count; i ++) {
			// redirect input
			dup2(in_fd, 0);
			close(in_fd);
			// init args if needed
			if (i > 0) { 
				args = parse_args(progs[i]);
			}
			// only checks output redir for last prog
			if (i == (prog_count - 1)) {
				out_fd = parse_io(args, 1);
				if (out_fd < 0) {
					out_fd = dup(tmp_out);
				}
			} else {
				// create a pipe
				int fd_pipe[2];
				pipe(fd_pipe);
				out_fd = fd_pipe[1];
				in_fd = fd_pipe[0];
			}
			// redirect output
			dup2(out_fd, 1);
			close(out_fd);
			// check if builtin, call func if so
			int builtin_index = is_builtin(args);
			if (builtin_index > -1) {
				status = (*builtin_func[builtin_index])(args);
			} else {
				// launch new process
				pid = fork();
				if (pid < 0) {
					perror("fork error");
				} else if (pid == 0) {
					if (execvp(args[0], args) < 0) {
						perror("exec error");
					}
				}
			}
			free(args);
		}

		//restore in / out defaults
		dup2(tmp_in, 0);
		dup2(tmp_out, 1);
		close(tmp_in);
		close(tmp_out);

		for (int i = 0; i < prog_count; i++) {
			wait(&p_status);
		}

		free(input);
		free(progs);
	}
}

char *get_input(void)
{	
    /* allocates memory for input 
     * reads input from stdin using getline()
     * rets ptr to input or NULL on err */
	size_t input_size = INPUT_BYTES;
	char *line_buf = malloc(sizeof(char *) * input_size);
	if (!line_buf) { // check for allocation error
		return NULL;
	}

	ssize_t bytes_read = getline(&line_buf, &input_size, stdin);
	if (bytes_read == -1) { // check for getline error
		return NULL;
	}
	return line_buf;
}

char **parse_progs(char *input) 
{
    /* allocates memory for program array
     * tokenizes progs based on pipes using strtok()
     * returns ptr to array or NULL on err */
	char **progs = malloc(sizeof(char *) * MAX_PROGS);
	if (!progs) { // allocation error
		return NULL;
	}

	int position = 0;
	char *token = strtok(input, "|");
	while (token) {
		progs[position] = token;
		position ++;
		token = strtok(NULL, "|");
	}

	progs[position] = NULL; // terminating NULL
	return progs;
}	

int count_progs(char **progs) // count number of progs in array 
{
	int count = 0;
	while (progs[count]) {
		count ++;
	}
	return count;
}

char **parse_args(char *prog)
{
    /* allocates memory for argument array
     * tokenizes based on spaces and newline
     * strips newline character
     * rets ptr to array or NULL on err */
	char **args = malloc(sizeof(char *) * ARG_SIZE);
	if (!args) { // allocation error
		return NULL;
	}

	int position = 0;
	char *token = strtok(prog, " \n");
	while (token) {
		size_t length = strlen(token);
		if (token[length - 1] == '\n')
		{
			token[length - 1] = '\0';
		}
		args[position] = token;
		position ++;
		token = strtok(NULL, " \n");
	}

	args[position] = NULL; // terminating NULL
	return args;
}

int parse_io(char **args, int flag) 
{
	/* checks for i/o redirection based on flag
	 * input: flag=0   ouput: flag=1
	 * returns -1 on error or no redirection */
	int fd;
	int current = 0;
	char *cmp, *cmp2;

	if (flag == 1) {
		cmp = ">";
		cmp2 = ">>";
	} else {
		cmp = "<";
	}

	while (args[current]) {
        // compare every argument to redirection operators 
		if (strcmp(args[current], cmp) == 0) {
		    if (flag == 1) {
				fd = open(args[current + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			} else {
				fd = open(args[current + 1], O_RDONLY, 0644);
			}
			if (fd < 0) {
				perror("open");
			}
			args[current] = NULL; // remove redirection operator (< or >) from args
			return fd;
		} else if (flag && (strcmp(args[current], cmp2) == 0)) {
            fd = open(args[current + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open");
            }
            args[current] = NULL; // remove >> operator from args
            return fd;
		} else if (!args[current + 1] && args[current + 2]) { // check for second operator
			current += 2;
		} else {
			current ++;
		}
	}
	return -1; // no redirection found
}

/* ---------------- BUILTIN FUNCTIONS ---------------- */

int is_builtin(char **args) 
{
	/* checks if a program is a builtin
	 * returns index w/in builtin_func[] if so, -1 otherwise */
	if (!args[0]) {
		return 0; // empty command: help()
	}
	for (int i = 0; i < builtin_size(); i++) {
		if ((strcmp(args[0], builtin_str[i])) == 0) {
			return i;
		}
	}
	return -1;
}

int builtin_size(void) // count number of builtin functions
{
	return sizeof(builtin_str) / sizeof(char *);
}

int tosh_pwd(char **args) // print working directory
{
	char cwd[256];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n", cwd);
	} // no err handling
	return 1;
}

int tosh_cd(char **args)
{ 
	/* changes cwd bassed on path passed to cd 
	 * assumes a path is given, otherwise no action */
	if (args[1]) { // consider changing to ~/ for no arg 
		if (chdir(args[1]) < 0) {
			perror("cd");
		}
	}
	return 1;
}

int tosh_help(char **args) // list builtin functions
{
	printf("------------------------------------------------\n");
	printf("tosh | Trey Oehmler | Middlebury CS315 Fall 2018\n");
	printf("------------------------------------------------\n");
	printf("The following functions are built in:\n");
	
	for (int i = 0; i < builtin_size(); i++) {
		printf(" - %s()\n", builtin_str[i]);
	}

	printf("\n");
	return 1;
}

int tosh_exit(char **args) // exit shell loop
{
	return 0;
}


