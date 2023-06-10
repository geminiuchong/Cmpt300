// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>

// #define MAX_LINE 80

// // Function to execute a command
// void execute_command(char **args) {
//     pid_t pid = fork();

//     if (pid < 0) {  // Error occurred
//         fprintf(stderr, "Fork failed.\n");
//         exit(1);
//     } else if (pid == 0) {  // Child process
//         if (execvp(args[0], args) < 0) {
//             fprintf(stderr, "Execution failed.\n");
//             exit(1);
//         }
//     } else {  // Parent process
//         wait(NULL);
//     }
// }

// int main() {
//     char *args[MAX_LINE/2 + 1];  // command line arguments
//     char line[MAX_LINE];  // the input line

//     while (1) {
//         printf("cshell$ ");
//         fflush(stdout);

//         // Read a line
//         if (fgets(line, sizeof(line), stdin) == NULL) {
//             break;
//         }

//         // Remove the trailing newline character
//         line[strcspn(line, "\n")] = '\0';

//         // Split the line into tokens
//         int i = 0;
//         char *token = strtok(line, " ");
//         while (token != NULL) {
//             args[i] = token;
//             i++;
//             token = strtok(NULL, " ");
//         }
//         args[i] = NULL;  // Last element should be NULL for execvp

//         // Check for the exit command
//         if (i > 0 && strcmp(args[0], "exit") == 0) {
//             printf("Bye!\n");
//             break;
//         }

//         // Execute the command
//         execute_command(args);
//     }

//     return 0;
// }

// // Function to execute a command
// void execute_command(char **args) {
//     pid_t pid = fork();

//     if (pid < 0) {  // Error occurred
//         fprintf(stderr, "Fork failed.\n");
//         exit(1);
//     } else if (pid == 0) {  // Child process
//         if (execvp(args[0], args) < 0) {
//             fprintf(stderr, "Execution failed.\n");
//             exit(1);
//         }
//     } else {  // Parent process
//         wait(NULL);
//     }
// }

//#include "cshell.h"
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
Command commands[MAX_COMMANDS];  // Array to store commands
int command_count = 0;  // Number of commands

#define MAX_ENV_VARS 512
EnvVar env_vars[MAX_ENV_VARS];  // Array to store environment variables
int env_var_count = 0;  // Number of environment variables

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

    // Check if the maximum number of environment variables has been reached
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
void handle_exit(char *theme_color) {
    printf("%sBye!\n\033[0m", theme_color);  // Print the exit message in the theme color
    exit(0);
}

// Function to handle the 'log' command
void handle_log(char *theme_color) {
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
            char *value = get_env_var(args[i] + 1);  // Skip the '$'
            if (value) {
                printf("%s%s", theme_color, value);
            }
        } else {
            // Print the argument as is
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
        return "\033[0m";  // Default color
    }

    // Set the theme based on the argument
    if (strcmp(args[1], "red") == 0) {
        return "\033[0;31m";  // Red color
    } else if (strcmp(args[1], "green") == 0) {
        return "\033[0;32m";  // Green color
    } else if (strcmp(args[1], "blue") == 0) {
        return "\033[0;34m";  // Blue color
    } else {
        printf("unsupported theme\n");
        return "\033[0m";  // Default color
    }
}

void handle_command(char **args, char **theme_color) {
    // Store the command
    if (command_count < MAX_COMMANDS) {
        // Concatenate all arguments into a single string
        char *command_line = strdup(args[0]);
        for (int i = 1; args[i] != NULL; i++) {
            command_line = realloc(command_line, strlen(command_line) + strlen(args[i]) + 2);
            strcat(command_line, " ");
            strcat(command_line, args[i]);
        }
        commands[command_count].name = command_line;
        time_t t = time(NULL);
        commands[command_count].time = *localtime(&t);
        // Note: You need to set the return value after the command is executed
        command_count++;
    } else {
        printf("Error: Maximum number of commands reached.\n");
    }

    // Check if the command is to set an environment variable
    if (args[0][0] == '$' && strchr(args[0], '=') != NULL) {
        // Split the command into the variable name and value
        char *name = args[0] + 1;  // Skip the '$'
        char *value = strchr(args[0], '=') + 1;  // Skip the '='
        *strchr(name, '=') = '\0';  // Terminate the name string

        // Check if the value is NULL
        if (value == NULL) {
            printf("Variable value expected\n");
            return;
        }
        // Set the environment variable
        set_env_var(name, value);
    } else if (strcmp(args[0], "exit") == 0) {
        handle_exit(*theme_color);
    } else if (strcmp(args[0], "print") == 0) {
        handle_print(args, *theme_color);
    } else if (strcmp(args[0], "theme") == 0) {
        *theme_color = handle_theme(args);
    } else if (strcmp(args[0], "log") == 0) {
        handle_log(*theme_color);
    }else {
        // Execute non-built-in commands
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "Missing keyword or command, or permission problem: %s\n", args[0]);
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Error forking
            fprintf(stderr, "Missing keyword or command, or permission problem: %s\n", args[0]);
        } else {
            // Parent process
            int status;
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
}


int main(int argc, char *argv[]) {
    // EnvVar *env_vars = malloc(MAX_ENV_VARS * sizeof(EnvVar));
    env_var_count = 0;
    char *theme_color = strdup("\033[0m");

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    FILE *input = stdin;  // Default to standard input
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

        // Check if the command is too long
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

    free(line);

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}



// // Function to handle the 'print' command
// void handle_print(char **args, char *theme_color) {
//     // Print the arguments
//     for (int i = 1; args[i] != NULL; i++) {
//         printf("%s%s ", theme_color, args[i]);
//     }
//     printf("\033[0m\n");  // Reset the text color
// }

// int main() {
//     char *args[MAX_LINE/2 + 1];  // command line arguments
//     char line[MAX_LINE];  // the input line
//     char *theme_color = "\033[0m";  // Default color

//     while (1) {
//         printf("%scshell$ \033[0m", theme_color);  // Print the shell prompt in the theme color
//         fflush(stdout);

//         // Read a line
//         if (fgets(line, sizeof(line), stdin) == NULL) {
//             break;
//         }

//         // Remove the trailing newline character
//         line[strcspn(line, "\n")] = '\0';

//         // Split the line into tokens
//         int i = 0;
//         char *token = strtok(line, " ");
//         while (token != NULL) {
//             args[i] = token;
//             i++;
//             token = strtok(NULL, " ");
//         }
//         args[i] = NULL;  // Last element should be NULL for execvp

//         // Check for built-in commands
//         if (i > 0) {
//             if (strcmp(args[0], "exit") == 0) {
//                 handle_exit(theme_color);
//             } else if (strcmp(args[0], "print") == 0) {
//                 handle_print(args, theme_color);
//             } else if (strcmp(args[0], "theme") == 0) {
//                 theme_color = handle_theme(args);
//             } else {
//                 // Execute the command
//                 execute_command(args);
//             }
//         }
//     }

//     return 0;
// }




// int main() {
//     char *args[MAX_LINE/2 + 1];  // command line arguments
//     char line[MAX_LINE];  // the input line

//     while (1) {
//         printf("cshell$ ");
//         fflush(stdout);

//         // Read a line
//         if (fgets(line, sizeof(line), stdin) == NULL) {
//             break;
//         }

//         // Remove the trailing newline character
//         line[strcspn(line, "\n")] = '\0';

//         // Split the line into tokens
//         int i = 0;
//         char *token = strtok(line, " ");
//         while (token != NULL) {
//             args[i] = token;
//             i++;
//             token = strtok(NULL, " ");
//         }
//         args[i] = NULL;  // Last element should be NULL for execvp

//         // Check for built-in commands
//         if (i > 0) {
//             if (strcmp(args[0], "exit") == 0) {
//                 handle_exit();
//             } else if (strcmp(args[0], "log") == 0) {
//                 handle_log();
//             } else if (strcmp(args[0], "print") == 0) {
//                 handle_print(args);
//             } else if (strcmp(args[0], "theme") == 0) {
//                 handle_theme(args);
//             } else {
//                 // Execute the command
//                 execute_command(args);
//             }
//         }
//     }

//     return 0;
// }
