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
#define MAX_DEPTH 20

void print_stat_info(struct stat file_stat) {
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
    printf("    ");
    printf(" %-10ld", file_stat.st_size);
    char date[20];
    strftime(date, 20, "%b %d %Y %H:%M", localtime(&(file_stat.st_mtime)));
    printf(" %-21s", date); 
}


int compare_dirent(const void *a, const void *b) {
    struct dirent *dir_a = *(struct dirent **)a;
    struct dirent *dir_b = *(struct dirent **)b;
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

void list_files(const char* dir_name, int i_option, int l_option, int R_option, int depth, int max_depth) {
    if (depth > max_depth) {
        printf("Maximum depth reached at directory: %s\n", dir_name);
        return;
    }
    DIR *d;
    struct dirent **dir_array = NULL;
    struct stat file_stat;
    char path[1024];
    int n = 0;  
    struct stat path_stat;
    stat(dir_name, &path_stat);

    if (S_ISREG(path_stat.st_mode)) { 
        if (i_option) {
            printf("%-10lu ", path_stat.st_ino);
        }
        if (l_option) {
            print_stat_info(path_stat);
        }
        printf("%-s\n", dir_name);
        return; 
    }

    d = opendir(dir_name);
    if (d) {

        while ((dir_array = realloc(dir_array, sizeof(*dir_array) * (n + 1))) && (dir_array[n] = readdir(d)) != NULL) {
            n++;
        }

        // Sort the directory entries
        qsort(dir_array, n, sizeof(*dir_array), compare_dirent);

        // Process the sorted directory entries for files and directories
        for (int i = 0; i < n; i++) {
            struct dirent *dir = dir_array[i];

            if(ignore_file(dir->d_name)) {
                continue;
            }

            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            lstat(path, &file_stat); 

            if (i_option) {
                printf("%-10lu ", dir->d_ino);
            }

            if (l_option) {
                print_stat_info(file_stat);
            }

            if (l_option && S_ISLNK(file_stat.st_mode)) { // If the l option is set and the file is a symbolic link
                char target[1024];
                ssize_t len = readlink(path, target, sizeof(target) - 1); 
                if (len != -1) {
                    target[len] = '\0';
                    printf("%-s -> %-s\n", dir->d_name, target); // Print the name of the symbolic link and the file it points to
                }
            } else {
                printf("%-s\n", dir->d_name);
            }

        }

        // Recursively list subdirectories if R_option is true
        if (R_option) {
            for (int i = 0; i < n; i++) {
                struct dirent *dir = dir_array[i];

                if(ignore_file(dir->d_name)) {
                    continue;
                }

                snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
                lstat(path, &file_stat); 

                if (S_ISDIR(file_stat.st_mode)) {
                    printf("\n%s:\n", path);
                    list_files(path, i_option, l_option, R_option, depth + 1, max_depth);
                }
            }
        }

        free(dir_array);

        closedir(d);
    }
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    int i_option = 0;
    int l_option = 0;
    int R_option = 0;
    

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && strpbrk(argv[i], "ilR") == NULL) {
            fprintf(stderr, "Error: Unsupported Option\n");
            fprintf(stderr, "Usage: %s [-i] [-l] [-R] [file...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

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

    // Create two lists for directories and files
    char **files = malloc(argc * sizeof(char *));
    char **dirs = malloc(argc * sizeof(char *));
    int files_count = 0;
    int dirs_count = 0;


    struct stat path_stat;
    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            if (lstat(argv[i], &path_stat) != 0) {
                fprintf(stderr, "Error: Nonexistent file or directory '%s'\n", argv[i]);
                continue; 
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
            exit(EXIT_FAILURE); 
        }
        if (S_ISDIR(path_stat.st_mode)) {
            dirs[dirs_count++] = ".";
        } else {
            files[files_count++] = ".";
        }
    }


    // Process files first
    for (int i = 0; i < files_count; i++) {
        list_files(files[i], i_option, l_option, R_option, 0, MAX_DEPTH);
    }

    // Then process directories
    for (int i = 0; i < dirs_count; i++) {
        printf("\n%s:\n", dirs[i]); 
        list_files(dirs[i], i_option, l_option, R_option, 0, MAX_DEPTH);
    }

    // Free the allocated lists
    free(files);
    free(dirs);

    exit(EXIT_SUCCESS);


}
