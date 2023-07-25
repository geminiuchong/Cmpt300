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

extern int optind;

void print_stat_info(struct stat file_stat) {
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
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
    return strcasecmp(dir_a->d_name, dir_b->d_name);
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

    d = opendir(dir_name);
    if (d) {
        // Read all the directory entries
        while ((dir_array = realloc(dir_array, sizeof(*dir_array) * (n + 1))) && (dir_array[n] = readdir(d)) != NULL) {
            n++;
        }

        // Sort the directory entries
        qsort(dir_array, n, sizeof(*dir_array), compare_dirent);

        // First pass: Process the sorted directory entries for files and directories
        // for (int i = 0; i < n; i++) {
        //     struct dirent *dir = dir_array[i];

        //     if(ignore_file(dir->d_name)) {
        //         continue;
        //     }

        //     snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
        //     stat(path, &file_stat);

        //     if (i_option) {
        //         printf("%-10lu ", dir->d_ino);
        //     }

        //     if (l_option) {
        //         print_stat_info(file_stat);
        //     }
            
        //     printf("%-s\n", dir->d_name);
        // }
        for (int i = 0; i < n; i++) {
            struct dirent *dir = dir_array[i];

            if(ignore_file(dir->d_name)) {
                continue;
            }

            snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
            stat(path, &file_stat);

            if (i_option) {
                printf("%-10lu ", dir->d_ino);
            }

            if (l_option) {
                print_stat_info(file_stat);
            }
            
            printf("%-s\n", dir->d_name);
        }

        // Second pass: Recursively list subdirectories if R_option is true
        if (R_option) {
            for (int i = 0; i < n; i++) {
                struct dirent *dir = dir_array[i];

                if(ignore_file(dir->d_name)) {
                    continue;
                }

                snprintf(path, sizeof(path), "%s/%s", dir_name, dir->d_name);
                stat(path, &file_stat);

                if (S_ISDIR(file_stat.st_mode) && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                    printf("\n%s/:\n", path);
                    list_files(path, i_option, l_option, R_option);
                }
            }
        }

        // Clean up
        free(dir_array);
        closedir(d);
    }
}


int main(int argc, char** argv) {
    int i_option = 0;
    int l_option = 0;
    int R_option = 0;
    char* dir_name = ".";
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
                fprintf(stderr, "Usage: %s [-ilR] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        dir_name = argv[optind];
    }
    list_files(dir_name, i_option, l_option, R_option);
    return 0;
}
