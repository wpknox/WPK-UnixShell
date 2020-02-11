#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 80

int exit_check(char *input, char *stop);
int parse_input_type(char *input);
int check_history(char *input, char *hist);
void space_parser(char *input, char **given_args);
void redirect(char *input, char **given_args, int cmd);
void parse_pipe(char *input, char **piped);
void exec_pipeline(char **left_pipe, char **right_pipe);
void exec_cmd(char **given_args, char *error_cmd, char *input, int cmd);

/*
*
* author - Willis Knox
*
* Main method for the program. When the program starts, it prints out "osh>" and
* then the user can enter commands. The main method first copies what the user entered
* into a char array and replaces the '\n' with a '\0'. It will then call cmd_type to
* see if a redirect or a pipeline is needed
*
* After the input has been correctly parsed, the main method calls exec_cmd(), which
* then actually tries to execute the given command.
*
* After the command has been executed, the program will go back to the top of the while
* loop until the user enters "exit" as a command
*
*/
int main(int argc, char *argv[])
{
  char *args[MAX_LINE/2 + 1];
  char input_str[MAX_LINE];
  char *hist_str;
  hist_str = (char *) malloc(sizeof(char) * MAX_LINE);
  int running = 1;
  int chars_entered = 0;
  while(running)
  {
    printf("osh>");
    fflush(stdout);
    
    // read from stdin and write it into input_str
    chars_entered = read(STDIN_FILENO, input_str, MAX_LINE);
    
    // replace the '\n' character with a NULL character
    input_str[chars_entered - 1] = '\0';
    
    if(!check_history(input_str, hist_str))
      continue;
      
    int cmd_type = parse_input_type(input_str);
    
    //if the user enters "exit" as command, exit the program.
    if (exit_check(input_str, "exit"))
    {
      running = 0;
      break;
    }
    
    // run the commands
    exec_cmd(args, hist_str, input_str, cmd_type);
  }
  return 0;
}

/* Simple function to see if the beginning of the input contains the exit command */
int exit_check(char *input, char *stop)
{
  return strncmp(stop, input, strlen(stop)) == 0;
}

/*
* Function that walks over input and see if there is a redirection or a pipeline
* to be created. Returns a specific int depending on what it finds.
*/
int parse_input_type(char *input)
{
  
  if (input[0] == '\0') // nothing entered, just leave
    return 0;
  
  int i;
    
  for (i = 0; i < MAX_LINE; i++)
  {
    if (input[i] == '<')
      return 1;
    if (input[i] == '>')
      return 2;
    if (input[i] == '|')
      return 3;
  }
  return 0;
}


/*
* Walks through the user input and separates the string where there are spaces.
* Will need to expand on this when dealing with piping and redirection
*/
void space_parser(char *input, char **given_args)
{
  if (input[0] == '\0') // nothing entered, just leave
    return;
  
  int i;
  
  for (i = 0; i < MAX_LINE; i++)
  {
    given_args[i] = strtok_r(input, " ", &input);
    if(given_args[i] == NULL)
      break;
  }
}

/*
* Function that checks the string that the user just entered in stdin
* to see if they are requesting the last command entered.
*
* If the user is not trying to access the history, the function copies
* what the user entered into the "history string" so this command will
* now be the command that the function fetches on the next iteration of
* the loop.
*/
int check_history(char *input, char *hist)
{
    if(strcmp(input, "!!") == 0)
    {
      strcpy(input, hist);
      if(input[0] == '\0')
      {
        printf("No commands in history!\n");
        return 0;
      }
      else
        printf("Command entered: \"%s\"\n", input);
      fflush(stdout);
    }
    strcpy(hist, input);
    return 1;
}

/*
* Function is called when a redirection was found in the input.
*
* If a '<' was present in the user input, the function opens a file and places its contents
* into STDIN
*
* If a '>' was present in the user input, the function tries to open an exisiting file and
* write the output of the given command into the file.
*/
void redirect(char *input, char **given_args, int cmd)
{
  if (input[0] == '\0') // nothing entered, just leave
    return;
  
  char *cmd_str;
  cmd_str = (char *) malloc(sizeof(char) * MAX_LINE);
  char *file_str;
  file_str = (char *) malloc(sizeof(char) * MAX_LINE);
  
  if (cmd == 1)
  {
    // get the command and then the file from the input
    cmd_str = strtok_r(input, "<", &input);
    file_str = strtok_r(input, " ", &input);
    
    // place command into args[][]
    space_parser(cmd_str, given_args);
    
    // open a file to read your input from.
    int fd = open(file_str, O_RDONLY);
    dup2(fd, 0);
    close(fd);
  }
  else //cmd == 2
  {
    // get the command and then the file from the input
    cmd_str = strtok_r(input, ">", &input);
    file_str = strtok_r(input, " ", &input);
    
    // place command into args[][]
    space_parser(cmd_str, given_args);
    
    // open a file to write your output to. If it doesn't exist, make it with rw permissions
    int fd = open(file_str, O_WRONLY|O_TRUNC|O_CREAT, 0666);
    dup2(fd, 1);
    close(fd);
  }
}

/*
* Helper function to place both commands entered by the user in two different parts of an array.
*/
void parse_pipe(char *input, char **piped)
{
  int i;
  for (i = 0; i < 2; i++)
  {
    piped[i] = strtok_r(input, "|", &input);
    if(piped[i] == NULL)
      break;
  }
}

/*
* Function that executes pipelined commands.
* Creates another child process and links the commands together with the pipe() function.
*
* In the child that is created in this function, pipedfd[1] replaces STDOUT
* and the command on the left of the pipe is executed
*
* In the parent process of this function (still the child of exec_cmd), pipedfd[0] replaces STDIN
* and the command on the right of the pipe is executed
*/
void exec_pipeline(char **left_pipe, char **right_pipe)
{
  // pipedfd[0] is the pipe that you read from
  // pipedfd[1] is the pipe that is written to
  int pipedfd[2];
  pid_t pid1;
  
  if (pipe(pipedfd) < 0)
  {
    printf("Error with pipe init\n");
    return;
  }
      
  pid1 = fork();
  
  if(pid1 < 0)
  {
    printf("error forking in pipeline");
    exit(1);
  }
  else if (pid1 == 0)
  {
    dup2(pipedfd[1], 1);
    close(pipedfd[0]);
    execvp(left_pipe[0], left_pipe);
    exit(0);
  }
  else
  {
    dup2(pipedfd[0], 0);
    close(pipedfd[1]);
    execvp(right_pipe[0], right_pipe);
    exit(0);
  }
}

/*
* Trys to run the command that the user entered. Creates a child process to do this.
* If it fails, it will print a message telling the user this.
*
* Depending on the type of command entered (normal, pipeline, redirect), this function
* may call other functions to intrepet and actually execute the given command.
*
* For example, if there is a pipeline, this calls exec_pipeline() and it will run
* the piped commands.
*
* If there is a redirection, this function will call redirect() to redirect the STDIN/STDOUT
* to the given file, but exec_cmd() still executes the command entered.
*
* If neither a pipeline or a redirection is entered, this function just calls space_parser()
* to correctly edit out the spaces and then it tries to execute the given command
*/
void exec_cmd(char **given_args, char *error_cmd, char *input, int cmd)
{
  
  // create new child process
  pid_t pid = fork();
  
  
  if (pid < 0) // didn't create child process
    return;
  else if (pid == 0) // execute child process
  {
    /* if/else if/else block to decide how to interpret input*/
    
    if (cmd == 1 || cmd == 2) //input or output... can make into two functions if needed
      redirect(input, given_args, cmd);
    else if (cmd == 3) // pipeline
    {
      char *piped_args[MAX_LINE/2 + 1];
      char *right_pipe[MAX_LINE/2 + 1];
      
      // split up the input into two strings
      parse_pipe(input, piped_args);
      // parse the command on the left of the '|'
      space_parser(piped_args[0], given_args);
      // parse the commmand on the right of the '|'
      space_parser(piped_args[1], right_pipe);
      
      // execute the two commands
      exec_pipeline(given_args, right_pipe);
    }
    else
      space_parser(input, given_args);
      
      
    if (execvp(given_args[0], given_args) < 0)
    {
      printf("cannot execute command: \"%s\"\n", error_cmd); // couldn't execute the command, display error msg
      exit(1);
    }
    exit(0);
  }
  else
  {
    waitpid(pid, NULL, 0); //wait for the child to finish execution
    return;
  }
}
