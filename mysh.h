/* 
 * mysh.h
 * Trey Oehmler
 * CS 315 Assignment 5 Fall 2018
 */

#ifndef __MYSH_H
#define __MYSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define INPUT_BYTES 4096
#define ARG_SIZE 11
#define MAX_PROG_SIZE 100
#define MAX_PIPES 99 // redundant

int prog_count;
int pipe_fd[MAX_PIPES];

void tosh_loop(void);
char *get_input(void);
char **parse_progs(char *input);
char **parse_args(char *input);
int parse_output(char **args);
int parse_input(char **args);
int execute_command(char **args, int in_fd, int out_fd);
int launch_process(char **args, int in_fd, int out_fd);
void print_args(char **args);
void print_progs(char **progs);

int tosh_cd(char **args);
int tosh_pwd(char **args);
int tosh_help(char **args);
int tosh_exit(char **args);
int builtin_size(void);

char *builtin_str[] = {
	"cd",
	"pwd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
  &tosh_cd,
  &tosh_pwd,
  &tosh_help,
  &tosh_exit
};

#endif
