#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 80

typedef struct{
    char *name;
    char *value;
}EnvVar;

typedef struct{
    char *name;
    struct tm time;
    int value;
}Command;

#define MAX_COMMANDS 512
Command commands[MAX_COMMANDS]; 
int command_count = 0;  

#define MAX_ENV_VARS 512
EnvVar env_vars[MAX_ENV_VARS]; 
int env_var_count = 0; 

char **parse_line(char *line) {
    int bufsize = 64;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "cshell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " ");
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "cshell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " ");
    }
    tokens[position] = NULL;
    return tokens;
}

// Function to set an environment variable
void set_env_var(char *name, char *value) {
    // Check if the variable already exists
    for (int i = 0; i < env_var_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            // If the variable exists, update its value
            free(env_vars[i].value);
            env_vars[i].value = strdup(value);
            return;
        }
    }

    if (env_var_count >= MAX_ENV_VARS) {
        printf("Error: Maximum number of environment variables reached.\n");
        return;
    }

    // If the variable does not exist, add a new one
    env_vars[env_var_count].name = strdup(name);
    env_vars[env_var_count].value = strdup(value);
    env_var_count++;
}

// Function to get the value of an environment variable
char *get_env_var(char *name) {
    // Check if the variable exists
    for (int i = 0; i < env_var_count; i++) {
        if (strcmp(env_vars[i].name, name) == 0) {
            // If the variable exists, return its value
            return env_vars[i].value;
        }
    }

    // If the variable does not exist, print an error message and return NULL
    printf("Error: No Environment Variable $%s found.", name);
    return NULL;
}

// Function to handle the 'exit' command
void handle_exit(char **args, char *theme_color) {
    if (args[1] != NULL) {
        printf("%sError: Too many arguments detected\n\033[0m", theme_color);
        return;
    }
    printf("%sBye!\n\033[0m", theme_color);  // Print the exit message in the theme color
    exit(0);
}

// Function to handle the 'log' command
void handle_log(char **args, char *theme_color) {
    if (args[1] != NULL) {
        printf("%sError: Too many arguments detected\n\033[0m", theme_color);
        return;
    }
    for (int i = 0; i < command_count; i++) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%c", &commands[i].time);
        printf("%s%s\n %s %d\n", theme_color, time_str, commands[i].name, commands[i].value);
    }
}

void handle_print(char **args, char *theme_color) {
    int i = 1;
    while (args[i]) {
        if (args[i][0] == '$') {
            // Print the value of the environment variable
            char *value = get_env_var(args[i] + 1); 
            if (value) {
                printf("%s%s", theme_color, value);
            }
        } else {
            printf("%s%s ", theme_color, args[i]);
        }
        i++;
    }
    printf("\n");
}


// Function to handle the 'theme' command
char* handle_theme(char **args) {
    // Check if an argument was passed
    if (args[1] == NULL) {
        printf("Error: No arguments passed\n");
        return strdup("\033[0m");  // Default color
    }

    // Set the theme based on the argument
    if (strcmp(args[1], "red") == 0) {
        return strdup("\033[0;31m");  // Red color
    } else if (strcmp(args[1], "green") == 0) {
        return strdup("\033[0;32m");  // Green color
    } else if (strcmp(args[1], "blue") == 0) {
        return strdup("\033[0;34m");  // Blue color
    } else {
        printf("unsupported theme\n");
        return strdup("\033[0m");  // Default color
    }
}

// Function to store the command
void store_command(char **args, char **theme_color) {
    if (command_count < MAX_COMMANDS) {
        char *command_line = strdup(args[0]);
        for (int i = 1; args[i] != NULL; i++) {
            command_line = realloc(command_line, strlen(command_line) + strlen(args[i]) + 2);
            strcat(command_line, " ");
            strcat(command_line, args[i]);
        }
        commands[command_count].name = command_line;
        time_t t = time(NULL);
        commands[command_count].time = *localtime(&t);
        command_count++;
    } else {
        printf("%sError: Maximum number of commands reached.\n", *theme_color);
    }
}

//Function to handle built-in and non built-in commands
void handle_command(char **args, char **theme_color) {
    // Store the command
    store_command(args, theme_color);

    // Check if the command is to set an environment variable
    if (args[0][0] == '$') {
        char *name = args[0] + 1; 
        char *value = NULL;

        if (strchr(name, '=') != NULL) {
            value = strchr(name, '=') + 1; 
            *strchr(name, '=') = '\0'; 

            // Check for multiple '=' characters
            if (strchr(value, '=') != NULL) {
                printf("%sInvalid syntax: multiple '=' characters\n", *theme_color);
                return;
            }
        }
        // Check if spaces around '='
        else if (args[1] != NULL && strcmp(args[1], "=") == 0 && args[2] != NULL) {
            printf("%sVariable value expected\n", *theme_color);
            return;
        }

        if (value == NULL) {
            printf("%sVariable name and value expected\n", *theme_color);
            return;
        }

        set_env_var(name, value);

    } else if (strcmp(args[0], "exit") == 0) {
        handle_exit(args, *theme_color);
    } else if (strcmp(args[0], "print") == 0) {
        handle_print(args, *theme_color);
    } else if (strcmp(args[0], "theme") == 0) {
        free(*theme_color);
        *theme_color = handle_theme(args);
    } else if (strcmp(args[0], "log") == 0) {
        handle_log(args, *theme_color);
    }else {
        // Replace arguments that start with '$' with the value of the environment variable
        for (int i = 0; args[i] != NULL; i++) {
            if (args[i][0] == '$') {
                char *value = get_env_var(args[i] + 1);
                if (value) {
                    args[i] = value;
                }
            }
        }
        // Create a pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Execute non-built-in commands
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(pipefd[0]); 
            dup2(pipefd[1], STDOUT_FILENO); 
            dup2(pipefd[1], STDERR_FILENO); 
            close(pipefd[1]); 

            if (execvp(args[0], args) == -1) {
                printf("Missing keyword or command, or permission problem\n");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            printf("Missing keyword or command, or permission problem\n");
        } else {
            // Parent process
            close(pipefd[1]);

            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            // Read from the pipe and print the output with the desired color
            char buffer[1024];
            ssize_t nbytes;
            while ((nbytes = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
                buffer[nbytes] = '\0';
                printf("%s%s\033[0m", *theme_color, buffer); // Print the output with the theme color
            }
            close(pipefd[0]);
        }
    }
}

int main(int argc, char *argv[]) {
    env_var_count = 0;
    char *theme_color = strdup("\033[0m");

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE *input = stdin;
    if (argc > 1) {
        // If a script file is provided, open it
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Unable to read script file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    while (1) {
        if (input == stdin){
            printf("%scshell$ \033[0m", theme_color);  // Print the shell prompt in the theme color
        }

        if ((read = getline(&line, &len, input)) == -1) {
            break;
        }

        if (read > MAX_LINE) {
            printf("Error: Command too long.\n");
            continue;
        }

        // Remove the newline character
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // Parse the line into arguments
        char **args = parse_line(line);

        // Handle the command
        handle_command(args, &theme_color);

        // Free the arguments
        free(args);
    }

    printf("\033[0m");

    free(theme_color);

    free(line);

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}