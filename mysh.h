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

static char *get_input(void);
static char **parse_args(char *input);
static int execute_command(char **args);

#endif
