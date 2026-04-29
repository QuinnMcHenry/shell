#include <stdio.h>
/*
fprintf()
printf()
stderr
getchar()
perror()
*/
#include <stdlib.h>
/*
malloc()
realloc()
free()
exit()
execvp()
EXIT_SUCCESS/EXIT_FAILURE
*/
#include <sys/wait.h> 
/*
waitpid()
*/
#include <unistd.h>
/*
chdir()
fork()
exec()
pid_t
*/
#include <string.h>
/*
strcmp()
strtok()
*/


/*
    Always return 1 to keep shell going and 0 to exit.
*/

// builtins declarations
int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);
int sh_print(char **args);

//builtin commands strings
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "print"
};

int (*builtin_func[]) (char **) = {
  &sh_cd,
  &sh_help,
  &sh_exit,
  &sh_print
};

int sh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

// change directory
int sh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "sh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("sh");
    }
  }
  return 1;
}

// print cur proc
int sh_print(char **args) {
    if (args[1]) {
        fprintf(stderr, "sh: print does not take args.\n");
    } else {
        printf("Current process: %d\n", getpid());
    }
    return 1;
}

// help
int sh_help(char **args)
{
  int i;
  printf("Quinn and Max's Shell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < sh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

// exit
int sh_exit(char **args)
{
  return 0;
}

// clone self to run program and wait for it to terminate.
int sh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // child
    if (execvp(args[0], args) == -1) { 
      perror("sh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("sh");
  } else {
    // parent
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

// run program or builtin
int sh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < sh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return sh_launch(args);
}

// read line from stdin
char *sh_read_line(void)
{
#ifdef SH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("sh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define SH_RL_BUFSIZE 1024
  int bufsize = SH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "sh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
        if (position < 100) {

          buffer[position] = c;
          position++;
        }
        else{
          fprintf(stderr,"input is greater than 100 characers, truncating\n");

          while (c != '\n' && c != EOF) {
            c = getchar();
          }

          buffer[0] = '\0';
          return buffer;
        }
    }

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += SH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"

// split line into tokens
char **sh_split_line(char *line)
{
  int bufsize = SH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "sh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, SH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

// check for valid input 
int valid_input(const char *line)
{
  for (int i = 0; line[i] != '\0'; i++) {
    char c = line[i];

    if (
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c >= '-' && c <= '.' || c == '/' || c == '_' ||
        c == ' ' || c == '\t' ||
        c == '&' || c == ';' 
    )   {
          continue;
    }

    return 0; // return 0 if not in char range
  }

  return 1;
}




// get input and execute it
void sh_loop(void)
{
  char *line;
  char **args;
  int status = 1;

  do {
    printf("?~ ");
    line = sh_read_line();
    // When input isn't in character range
    if(!valid_input(line)) {
      fprintf(stderr, "invalid character in input \n");
      free(line);
      continue; // skip execution
    }
    
    args = sh_split_line(line);
    status = sh_execute(args);

    free(line);
    free(args);
  } while (status);
}

// main
int main(int argc, char **argv)
{

  // command loop.
  sh_loop();

  return EXIT_SUCCESS;
}

