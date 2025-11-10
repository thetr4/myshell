#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#define MAX 255
#define MAX_PATH 255
int tokenize_input(char *input, char *argv[], int max_args){
    int i = 0;
    char *token = strtok(input, " '\"");

    while (token != NULL){
        if (i >= max_args - 1){
            break;
        }
        else{
        argv[i] = token;
        token = strtok(NULL, " ");
        i++;
      }
    }
    argv[i] = NULL;
    return i;
}
int main(){
    char str[MAX];

    while(1){
        uid_t uid = getuid();

        struct passwd *pw;

        pw = getpwuid(uid);

        if (pw == NULL){
            fprintf(stderr, "error: username not found\n");
            exit(0);
        }

        char *username = pw->pw_name;

        char cd[MAX_PATH];
            
        if (getcwd(cd, sizeof(cd)) == NULL){
            fprintf(stderr, "error: can't get current directory\n");
            continue;
        }
        write(1, username, strlen(username));
        write(1, "[", 1);
        write(1, cd, strlen(cd));
        write(1, "]#: ", 4);
        ssize_t str_size = read(0, str, sizeof(str));

        if (str_size == 0){
            write(1, "\n", 1);
            break;
        }
        if (str_size > 0 && str[str_size - 1] == '\n'){
            str[str_size - 1] = '\0';
        }

        char *argv[MAX];
        int argc = tokenize_input(str, argv, MAX);

        char *input_file = NULL;
        char *output_file = NULL;

        if (argc == 0) continue;

        if (strcmp(argv[0], "exit") == 0){
            exit(0);
        }
        
        if (strcmp(argv[0], "cd") == 0){
            if (argv[1] == NULL){
                fprintf(stderr, "cd: missing argument\n");
            }
            else{
                if (chdir(argv[1]) != 0){
                fprintf(stderr, "cd: %s: failed to change directory\n", argv[1]);
            }
        }
            continue;
        }

        // There is a bug with input-output redirection in this part of the code.

        int i = 1;
        while(i++ <= argc)
        {
            if(i < argc && strcmp(argv[i], "<") == 0){
                input_file = argv[i++];
                argv[i++] = NULL;
                i++;
            }
            else if(i < argc && strcmp(argv[i], ">") == 0){
                output_file = argv[i++];
                argv[i++] = NULL;
                i++;
            }
            else{
                i++;
            }
        }

        pid_t pid = fork();

        if (pid == 0){
            struct stat st;
            
            if (input_file != NULL){
            if (stat(input_file, &st) == 0){

                if (S_ISDIR(st.st_mode)){
                    fprintf(stderr, "No such file: %s: is a directory\n", input_file);
                    _exit(EXIT_FAILURE);
                }
                
                int in_f = open(input_file, O_RDONLY);

                if (in_f == -1){
                    fprintf(stderr, "error input file\n");
                    _exit(EXIT_FAILURE);
                }
                dup2(in_f, STDIN_FILENO);
                close(in_f);
                
            }
        }
            if(output_file != NULL){
                if (stat(output_file, &st) == 0){
                    if (S_ISDIR(st.st_mode)){
                        fprintf(stderr, "%s: is directory\n", output_file);
                        _exit(EXIT_FAILURE);
                    }

                    int out_f = open(output_file, O_CREAT | O_TRUNC, 0744);

                    if (out_f == -1){
                        fprintf(stderr, "error output file\n");
                        _exit(1);
                    }
                    dup2(out_f, STDOUT_FILENO);
                    close(out_f);
                }
            }
            if (execvp(argv[0], argv) == -1){
                fprintf(stderr, "%s: Command not found\n", argv[0]);
                _exit(EXIT_FAILURE);
            }
        }
        if (pid > 0){
            wait(NULL);
        }
        if (pid < 0){
            exit(1);
        }
    }
    return 0;
}
