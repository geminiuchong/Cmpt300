#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>

extern int optind;

void print_stat_info(struct stat file_stat) {
    // Add the condition for symbolic link
    printf((S_ISLNK(file_stat.st_mode)) ? "l" : (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
    printf("    "); 
    printf(" %-3lu", file_stat.st_nlink);
    printf(" %-8s", getpwuid(file_stat.st_uid)->pw_name);
    printf(" %-8s", getgrgid(file_stat.st_gid)->gr_name);
    printf(" %-12ld", file_stat.st_size);
    char date[20];
    strftime(date, 20, "%b %d %Y %H:%M", localtime(&(file_stat.st_mtime)));
    printf(" %-21s", date); 
}


// Comparison function for qsort
int compare_dirent(const void *a, const void *b) {
    struct dirent *dir_a = *(struct dirent **)a;
    struct dirent *dir_b = *(struct dirent **)b;
    //return strcasecmp(dir_a->d_name, dir_b->d_name);
    return strcoll(dir_a->d_name, dir_b->d_name);
}


int ignore_file(const char *d_name) {
    const char *ignore_files[] = {".git", ".svn", ".hg", ".vscode", "settings.json", "launch.json", "c_cpp_properties.json",
                                  "node_modules", "package.json", "yarn.lock", "package-lock.json", "venv", "Pipfile", 
                                  "requirements.txt", "Desktop.ini", "Thumbs.db", ".DS_Store", NULL};
    if (d_name[0] == '.') {
        return 1;
    }
    for (int i = 0; ignore_files[i] != NULL; i++) {
        if (strcmp(d_name, ignore_files[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void list_files(const char* dir_name, int i_option, int l_option, int R_option) {
    DIR *d;
    struct dirent **dir_array = NULL;
    struct stat file_stat;
    char path[1024];
    int n = 0;  // Number of directory entries
    struct stat path_stat;
    stat(dir_name, &path_stat);

    if (S_ISREG(path_stat.st_mode)) { // If the path is a file, print it directly
        if (i_option) {
            printf("%-10lu ", path_stat.st_ino);
        }
        if (l_option) {
            print_stat_info(path_stat);
        }
        printf("%-s\n", dir_name);
        return; // Return early since there's nothing more to do for a file
    }

    d = opendir(dir_name);
    if (d) {
        // Read all the directory entries
        while ((dir_array = realloc(dir_array, sizeof(*dir_array) * (n + 1))) && (dir_array[n] = readdir(d)) != NULL) {
            n++;
        }

        // Sort the directory entries
        qsort(dir_array, n, sizeof(*dir_array), compare_dirent);

        // First pass: Process the sorted directory entries for files and directories
        for (int i = 0; i < n; i++) {
            struct dirent *dir = dir_array[i];

            if(ignore_file(dir->d_name)) {
                continue;
            }

            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            lstat(path, &file_stat); // Use lstat instead of stat

            if (i_option) {
                printf("%-10lu ", dir->d_ino);
            }

            if (l_option) {
                print_stat_info(file_stat);
            }

            // if (S_ISLNK(file_stat.st_mode)) { // If the file is a symbolic link
            //     char target[1024];
            //     ssize_t len = readlink(path, target, sizeof(target) - 1); // Read the file it points to
            //     if (len != -1) {
            //         target[len] = '\0';
            //         printf("%-s -> %-s\n", dir->d_name, target); // Print the name of the symbolic link and the file it points to
            //     }
            // } else {
            //     printf("%-s\n", dir->d_name);
            // }
            if (l_option && S_ISLNK(file_stat.st_mode)) { // If the l option is set and the file is a symbolic link
                char target[1024];
                ssize_t len = readlink(path, target, sizeof(target) - 1); // Read the file it points to
                if (len != -1) {
                    target[len] = '\0';
                    printf("%-s -> %-s\n", dir->d_name, target); // Print the name of the symbolic link and the file it points to
                }
            } else {
                printf("%-s\n", dir->d_name);
            }

        }

        // Second pass: Recursively list subdirectories if R_option is true
        if (R_option) {
            for (int i = 0; i < n; i++) {
                struct dirent *dir = dir_array[i];

                if(ignore_file(dir->d_name)) {
                    continue;
                }

                snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
                lstat(path, &file_stat); // Use lstat instead of stat

                if (S_ISDIR(file_stat.st_mode)) {
                    printf("\n%s:\n", path);
                    list_files(path, i_option, l_option, R_option);
                }
            }
        }

 
        // Free the directory entries
        // for (int i = 0; i < n; i++) {
        //     free(dir_array[i]);
        // }
        free(dir_array);

        closedir(d);
    }
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");  // Set the locale to the user's default locale

    int i_option = 0;
    int l_option = 0;
    int R_option = 0;
    
    // Check for unsupported options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && strpbrk(argv[i], "ilR") == NULL) {
            fprintf(stderr, "Error: Unsupported Option\n");
            fprintf(stderr, "Usage: %s [-i] [-l] [-R] [file...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // Check for long options (start with --)
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            fprintf(stderr, "Error: Unsupported Option\n");
            fprintf(stderr, "Usage: %s [-i] [-l] [-R] [file...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Parse options
    int opt;
    while ((opt = getopt(argc, argv, "ilR")) != -1) {
        switch (opt) {
            case 'i':
                i_option = 1;
                break;
            case 'l':
                l_option = 1;
                break;
            case 'R':
                R_option = 1;
                break;
            default:
                fprintf(stderr, "Error: Unsupported Option\n");
                fprintf(stderr, "Usage: %s [-i] [-l] [-R] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Create two lists for directories and files.
    char **files = malloc(argc * sizeof(char *));
    char **dirs = malloc(argc * sizeof(char *));
    int files_count = 0;
    int dirs_count = 0;

    // // Pre-check for nonexistent files or directories
    // struct stat path_stat;
    // if (optind < argc) {
    //     for (int i = optind; i < argc; i++) {
    //         if (stat(argv[i], &path_stat) != 0) {
    //             fprintf(stderr, "Error : Nonexistent files or directories\n");
    //             exit(EXIT_FAILURE);
    //         }
    //         if (S_ISDIR(path_stat.st_mode)) {
    //             dirs[dirs_count++] = argv[i];
    //         } else {
    //             files[files_count++] = argv[i];
    //         }
    //     }
    // } else {
    //     if (stat(".", &path_stat) != 0) {
    //         fprintf(stderr, "Error : Nonexistent files or directories\n");
    //         exit(EXIT_FAILURE);
    //     }
    //     if (S_ISDIR(path_stat.st_mode)) {
    //         dirs[dirs_count++] = ".";
    //     } else {
    //         files[files_count++] = ".";
    //     }
    // }
    // Pre-check for nonexistent files or directories
    struct stat path_stat;
    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            if (lstat(argv[i], &path_stat) != 0) {
                fprintf(stderr, "Error: Nonexistent file or directory '%s'\n", argv[i]);
                continue; // Skip this iteration instead of terminating the program
            }
            if (S_ISDIR(path_stat.st_mode)) {
                dirs[dirs_count++] = argv[i];
            } else {
                files[files_count++] = argv[i];
            }
        }
    } else {
        if (lstat(".", &path_stat) != 0) {
            fprintf(stderr, "Error: Nonexistent file or directory '.'\n");
            exit(EXIT_FAILURE); // Here we should exit, because if '.' doesn't exist, we can't continue
        }
        if (S_ISDIR(path_stat.st_mode)) {
            dirs[dirs_count++] = ".";
        } else {
            files[files_count++] = ".";
        }
    }


    // Process files first
    for (int i = 0; i < files_count; i++) {
        list_files(files[i], i_option, l_option, R_option);
    }

    // Then process directories
    for (int i = 0; i < dirs_count; i++) {
        printf("\n%s:\n", dirs[i]); // <-- printing directory/file header here only if it's a directory
        list_files(dirs[i], i_option, l_option, R_option);
    }

    // Free the allocated lists
    free(files);
    free(dirs);

    exit(EXIT_SUCCESS);


}

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <dirent.h>
// #include <string.h>
// #include <pwd.h>
// #include <grp.h>
// #include <time.h>

// extern int optind;

// void print_stat_info(struct stat file_stat) {
//     printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
//     printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
//     printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
//     printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
//     printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
//     printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
//     printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
//     printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
//     printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
//     printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
//     printf("    "); 
//     printf(" %-3lu", file_stat.st_nlink);
//     printf(" %-8s", getpwuid(file_stat.st_uid)->pw_name);
//     printf(" %-8s", getgrgid(file_stat.st_gid)->gr_name);
//     printf(" %-12ld", file_stat.st_size);
//     char date[20];
//     strftime(date, 20, "%b %d %Y %H:%M", localtime(&(file_stat.st_mtime)));
//     printf(" %-21s", date); 
// }



// // Comparison function for qsort
// int compare_dirent(const void *a, const void *b) {
//     struct dirent *dir_a = *(struct dirent **)a;
//     struct dirent *dir_b = *(struct dirent **)b;
//     return strcasecmp(dir_a->d_name, dir_b->d_name);
// }

// int ignore_file(const char *d_name) {
//     const char *ignore_files[] = {".git", ".svn", ".hg", ".vscode", "settings.json", "launch.json", "c_cpp_properties.json",
//                                   "node_modules", "package.json", "yarn.lock", "package-lock.json", "venv", "Pipfile", 
//                                   "requirements.txt", "Desktop.ini", "Thumbs.db", ".DS_Store", NULL};
//     if (d_name[0] == '.') {
//         return 1;
//     }
//     for (int i = 0; ignore_files[i] != NULL; i++) {
//         if (strcmp(d_name, ignore_files[i]) == 0) {
//             return 1;
//         }
//     }
//     return 0;
// }

// void list_files(const char* dir_name, int i_option, int l_option, int R_option) {
//     DIR *d;
//     struct dirent **dir_array = NULL;
//     struct stat file_stat;
//     char path[1024];
//     int n = 0;  // Number of directory entries

//     d = opendir(dir_name);
//     if (d) {
//         // Read all the directory entries
//         while ((dir_array = realloc(dir_array, sizeof(*dir_array) * (n + 1))) && (dir_array[n] = readdir(d)) != NULL) {
//             n++;
//         }

//         // Sort the directory entries
//         qsort(dir_array, n, sizeof(*dir_array), compare_dirent);

//         // First pass: Process the sorted directory entries for files and directories
//         for (int i = 0; i < n; i++) {
//             struct dirent *dir = dir_array[i];

//             if(ignore_file(dir->d_name)) {
//                 continue;
//             }

//             snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
//             stat(path, &file_stat);

//             if (i_option) {
//                 printf("%-10lu ", dir->d_ino);
//             }

//             if (l_option) {
//                 print_stat_info(file_stat);
//             }
            
//             printf("%-s\n", dir->d_name);
//         }

//         // Second pass: Recursively list subdirectories if R_option is true
//         if (R_option) {
//             for (int i = 0; i < n; i++) {
//                 struct dirent *dir = dir_array[i];

//                 if(ignore_file(dir->d_name)) {
//                     continue;
//                 }

//                 snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
//                 stat(path, &file_stat);

//                 if (S_ISDIR(file_stat.st_mode) && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
//                     printf("\n%s/:\n", path);
//                     list_files(path, i_option, l_option, R_option);
//                 }
//             }
//         }

//         // Clean up
//         free(dir_array);
//         closedir(d);
//     }
// }

// int main(int argc, char** argv) {
//     int i_option = 0;
//     int l_option = 0;
//     int R_option = 0;
//     char* dir_name = ".";
//     int opt;
    
//     // Check for unsupported options
//     for (int i = 1; i < argc; i++) {
//         if (argv[i][0] == '-' && strpbrk(argv[i], "ilR") == NULL) {
//             fprintf(stderr, "Error: Unsupported Option\n");
//             fprintf(stderr, "Usage: %s [-ilR] [file...]\n", argv[0]);
//             exit(EXIT_FAILURE);
//         }

//         // Check for long options (start with --)
//         if (argv[i][0] == '-' && argv[i][1] == '-') {
//             fprintf(stderr, "Error: Unsupported Option\n");
//             fprintf(stderr, "Usage: %s [-ilR] [file...]\n", argv[0]);
//             exit(EXIT_FAILURE);
//         }
//     }

//     while ((opt = getopt(argc, argv, "ilR")) != -1) {
//         switch (opt) {
//             case 'i':
//                 i_option = 1;
//                 break;
//             case 'l':
//                 l_option = 1;
//                 break;
//             case 'R':
//                 R_option = 1;
//                 break;
//             default:
//                 fprintf(stderr, "Usage: %s [-ilR] [file...]\n", argv[0]);
//                 exit(EXIT_FAILURE);
//         }
//     }

//     if (optind < argc) {
//         dir_name = argv[optind];
//     }

//     DIR *dir = opendir(dir_name);
//     if (dir) {
//         closedir(dir);  // If the directory exists, close it here. It will be reopened in list_files.
//     } else {
//         fprintf(stderr, "Error: Nonexistent files or directories\n");
//         exit(EXIT_FAILURE);
//     }

//     list_files(dir_name, i_option, l_option, R_option);
//     return 0;
// }
    // if (optind < argc) {
    //     for (int i = optind; i < argc; i++) {
    //         struct stat path_stat;
    //         stat(argv[i], &path_stat);
    //         if (S_ISDIR(path_stat.st_mode)) {
    //             printf("\n%s:\n", argv[i]); // <-- printing directory/file header here only if it's a directory
    //         }
    //         list_files(argv[i], i_option, l_option, R_option);
    //     }
    // } else {
    //     list_files(".", i_option, l_option, R_option);
    // }


    // exit(EXIT_SUCCESS);



           // // First pass: Process the sorted directory entries for files only
        // for (int i = 0; i < n; i++) {
        //     struct dirent *dir = dir_array[i];

        //     if(ignore_file(dir->d_name)) {
        //         continue;
        //     }

        //     snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        //     lstat(path, &file_stat); // Use lstat instead of stat

        //     if (!S_ISDIR(file_stat.st_mode)) { // If it's not a directory, process
        //         if (i_option) {
        //             printf("%-10lu ", dir->d_ino);
        //         }

        //         if (l_option) {
        //             print_stat_info(file_stat);
        //         }

        //         if (S_ISLNK(file_stat.st_mode)) { // If the file is a symbolic link
        //             char target[1024];
        //             ssize_t len = readlink(path, target, sizeof(target) - 1); // Read the file it points to
        //             if (len != -1) {
        //                 target[len] = '\0';
        //                 printf("%-s -> %-s\n", dir->d_name, target); // Print the name of the symbolic link and the file it points to
        //             }
        //         } else {
        //             printf("%-s\n", dir->d_name);
        //         }
        //     }
        // }

        // // Second pass: Process the sorted directory entries for directories only
        // for (int i = 0; i < n; i++) {
        //     struct dirent *dir = dir_array[i];

        //     if(ignore_file(dir->d_name)) {
        //         continue;
        //     }

        //     snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        //     lstat(path, &file_stat); // Use lstat instead of stat

        //     if (S_ISDIR(file_stat.st_mode)) { // If it's a directory, process
        //         if (i_option) {
        //             printf("%-10lu ", dir->d_ino);
        //         }

        //         if (l_option) {
        //             print_stat_info(file_stat);
        //         }

        //         printf("%-s\n", dir->d_name);
        //     }
        // }
        // // Third pass: Recursively list subdirectories if R_option is true
        // if (R_option) {
        //     for (int i = 0; i < n; i++) {
        //         struct dirent *dir = dir_array[i];

        //         if(ignore_file(dir->d_name)) {
        //             continue;
        //         }

        //         snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        //         lstat(path, &file_stat); // Use lstat instead of stat

        //         if (S_ISDIR(file_stat.st_mode)) {
        //             printf("\n%s:\n", path);
        //             list_files(path, i_option, l_option, R_option);
        //         }
        //     }
        // }


        // int compare_dirent(const void *a, const void *b) {
//     struct dirent *dir_a = *(struct dirent **)a;
//     struct dirent *dir_b = *(struct dirent **)b;

//     const char *a_name = dir_a->d_name;
//     const char *b_name = dir_b->d_name;

//     while (*a_name && *b_name) {
//         if (isdigit(*a_name) && isdigit(*b_name)) {
//             long a_digit = strtol(a_name, (char**)&a_name, 10);
//             long b_digit = strtol(b_name, (char**)&b_name, 10);
//             if (a_digit != b_digit) return a_digit - b_digit;
//         } else {
//             int diff = tolower(*a_name) - tolower(*b_name);
//             if (diff != 0) return diff;
//             a_name++;
//             b_name++;
//         }
//     }
//     return *a_name - *b_name;
// }