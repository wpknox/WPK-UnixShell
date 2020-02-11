# Willis Knox
## COM S 352 - Project 1

To build this project, type `make` in the terminal.

After the project has been built, to run the project type: `./osh` in the terminal.

There is a history feature that you can access by typing `!!` as a command. This will run the most recent command you
have typed in.

The program will then begin and you will be able to enter different commands such as: `ls`, `cp file1 file2`, `pwd`, and others.
This program also supports the redirection of input and output with `<` and `>` when they are entered in a command.
The program also supports a pipeline when it is entered in a command. `|`

According to the spec in the book, I can assume that there will be at most a single special character (`<, >, |`) in
a single command, and pipelines will never be in the same command as a redirection.

To end the program, the user can type `exit` as a command and then the program will end.

To clean the project, type `make clean` in the terminal and the Makefile will delete all excess files/executables that are present in the folder.
