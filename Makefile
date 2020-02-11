all: unix_shell.c
	gcc unix_shell.c -Wall -Werror -ggdb3 -O2 -o osh
clean:
	rm -f osh *~