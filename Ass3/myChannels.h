#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>
#include <float.h>

struct channelFile
{
    FILE *fp;
    double alpha;
    double beta;
};
struct threadArgs
{
    size_t tid;
    struct channelFile *files;
    FILE *output;
};
struct convertArray
{
    double *data;
    size_t size;
    size_t cur;
    int fid;
};