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

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"


int sh_execute(char **args) {
    int i;
    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < sh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return sh_launch(args);
}

int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);

char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[]) (char **) = {&sh_cd, &sh_help, &sh_exit};

int sh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}


int sh_cd(char **args) {
    if (args[i] == NULL) {
        fprintf(stderr, "sh: expected argument to \ "cd\ "\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("sh");
        }
    }
    return 1;
}

int sh_help(char **args) {
    int i;
    printf("Quinn and Max's Shell\n");
    printf("Type program name and arguments, and hit enter\n");
    printf("The following are built in: \n");
    for (i = 0; i < sh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int sh_exit(char **args) {
    return 0;
}

int sh_launch(char **args) {
    pid_t pid, wpid;
    int status;
    
    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("sh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("sh");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

char** sh_split_line(char *line) {
    int bufsize = SH_TOK_BUFSIZE;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
    
        if (position >= bufsize) {
            budsize += SH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "sh: reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}



char* sh_read_line(void) {
    char* line = NULL;
    ssize_t bufsize = 0; 
    
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

void sh_loop(void) {
    char* line;
    char** args;
    int status;
    
    do {
        printf("|> ");
        line = sh_read_line();
        args = sh_split_line(line);
        status = sh_execute(args);
        
        free(line);
        free(args);
    } while (status);
}


int main(int argc, char** argv) {
    
    sh_loop();

    return EXIT_SUCESS;  
}
