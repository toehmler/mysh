mysh: mysh.c
	gcc -Wall -lreadline -pedantic -g -o mysh mysh.c

.phony: clean
clean:
	rm -rf mysh

