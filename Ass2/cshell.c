#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct{
    char *name;
    char *value;
}EnvVar;

typedef struct{
    char *name;
    struct tm time;
    return value;
}Command;