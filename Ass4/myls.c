#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
    printf(" ");
}

void list_files(int show_inode, int long_format) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    struct passwd *pw;
    struct group *grp;
    char time_str[20];
    struct tm *time;

    dir = opendir(".");  // Open the current directory.
    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (long_format) {
            stat(entry->d_name, &file_stat);

            print_permissions(file_stat.st_mode);
            printf("%2lu", file_stat.st_nlink);

            pw = getpwuid(file_stat.st_uid);
            printf(" %-8s", pw->pw_name);

            grp = getgrgid(file_stat.st_gid);
            printf(" %-8s", grp->gr_name);

            printf(" %7ld", file_stat.st_size);

            time = localtime(&file_stat.st_mtime);
            strftime(time_str, sizeof(time_str), "%b %d %H:%M", time);
            printf(" %s", time_str);
        }

        if (show_inode)
            printf(" %-10lu", entry->d_ino);

        printf(" %s\n", entry->d_name);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    int opt;
    int show_inode = 0;
    int long_format = 0;

    while ((opt = getopt(argc, argv, "il")) != -1) {
        switch (opt) {
            case 'i':
                show_inode = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-il]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    list_files(show_inode, long_format);
    return 0;
}
