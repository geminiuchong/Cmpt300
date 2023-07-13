#include "myChannels.h"

int buffer_size;
int num_threads;
char *metadata_file_path;
int lock_config;
int global_checkpointing;
char *output_file_path;
int num_files;
double *result;
size_t result_size = 1;
size_t output_size;
sem_t mutex;
sem_t *local_mutex;
int lock = 0;
sem_t checkpoint_sem;
int *checkpoint_counters;

void global_lock_add(int i, int j, struct convertArray *input, double *result) {
    sem_wait(&mutex);
    double temp;
    if (result[j] > DBL_MAX - input[i].data[j]) {
        temp = DBL_MAX;
    } else {
        temp = result[j] + input[i].data[j];
    }
    result[j] = temp;
    sem_post(&mutex);
}

void each_entry_lock_add(int i, int j, struct convertArray *input, double *result) {
    sem_wait(&local_mutex[j]);
    double temp;
    if (result[j] > DBL_MAX - input[i].data[j]) {
        temp = DBL_MAX;
    } else {
        temp = result[j] + input[i].data[j];
    }
    result[j] = temp;
    sem_post(&local_mutex[j]);
}

void compare_and_swap_add(int i, int j, struct convertArray *input, double *result) {
    int temp = lock;
    if (temp == 0)
        lock = 1;
    while (temp != 0)
    {
        temp = lock;
        if (temp == 0)
            lock = 1;
    }
    double temp_result;
    if (result[j] > DBL_MAX - input[i].data[j]) {
        temp_result = DBL_MAX;
    } else {
        temp_result = result[j] + input[i].data[j];
    }
    result[j] = temp_result;
    lock = 0;
}

void (*func_ptr[3])(int, int, struct convertArray *, double *) = {global_lock_add, each_entry_lock_add, compare_and_swap_add};


void *thread_function(void *thread_args)
{
    struct threadArgs *args = thread_args;
    int thread_id = (int)args->tid;
    int fileid = thread_id;
    struct channelFile *files = args->files;
    int p = num_files / num_threads;
    //int *filelist = (int *)malloc(p * sizeof(int));
    int *filelist;
    long long temp_size = (long long)p * sizeof(int);
    if (temp_size > 65535) {
        filelist = (int *)malloc(65535);
    } else {
        filelist = (int *)malloc(temp_size);
    }
    if (filelist == NULL) {
        printf("Error: malloc failed\n");
        free(filelist);
        return NULL;
    }

    struct convertArray input[p];
    int *checkpoint = (int *)malloc(p * sizeof(int));

    for (int i = 0; i < p; ++i)
    {
        filelist[i] = fileid;
        checkpoint[i] = 0;
        fseek(files[fileid].fp, 0L, SEEK_END);
        long long temp_size = (long long)ftell(files[fileid].fp) + (buffer_size - 1);
        if (temp_size > 65535) {
            input[i].size = 65535;
        } else {
            input[i].size = temp_size / buffer_size; // rounding up
        }
        input[i].cur = 0;
        rewind(files[fileid].fp);
        input[i].data = (double *)malloc(input[i].size * sizeof(double));
        if (input[i].data == NULL) {
            printf("Error: malloc failed for input[%d].data\n", i);
            free(filelist);
            free(input[i].data);
            return NULL;
        }
        input[i].fid = fileid;
        fileid += num_threads;
    }

    int finished = 0;
    int current = 0;

    do{
        for (int i = 0; i < p; ++i) {

            if (global_checkpointing && checkpoint_counters[thread_id] != current) {
                sem_wait(&checkpoint_sem);
                continue;
            }

            int fid = filelist[i];
            if (feof(files[fid].fp)) {
                finished++;
                if (finished == p)
                    break;
                continue;
            }

            char buffer[buffer_size];
            memset(buffer, 0, sizeof(buffer));
            int k = 0;

            for (int j = 0; j < buffer_size; ++j)
            {
                int x = fgetc(files[fid].fp);
                if (isdigit(x))
                {
                    buffer[k++] = (char)x;
                }
            }

            buffer[k] = '\0';

            if (k != 0)
            {
                input[i].data[input[i].cur++] = (double)(int32_t)strtol(buffer, NULL, 10);
            }
            checkpoint[i]++;

            if (global_checkpointing) {
                checkpoint_counters[thread_id]++;
                if (checkpoint_counters[thread_id] == num_threads) {
                    for (int j = 0; j < num_threads; ++j) {
                        sem_post(&checkpoint_sem);
                    }
                }
            }

        }
        current++;
    }while (finished < p);

    for (int i = 0; i < p; ++i)
    {
        if (files[input[i].fid].alpha < 0 || files[input[i].fid].alpha > 1)
            return NULL; // handle error
        if (input[i].cur < 2)
            return NULL;
        for (int j = 1; j < input[i].cur; ++j)
        {
            // input[i].data[j] = files[input[i].fid].alpha * input[i].data[j] + (1 - files[input[i].fid].alpha) * input[i].data[j - 1];
            // // if (thread_id == 1)
            // //     printf("current result = %f\n", input[i].data[j]);
            double temp = files[input[i].fid].alpha * input[i].data[j] + (1 - files[input[i].fid].alpha) * input[i].data[j - 1];
            if (temp > 65535)  // Check for overflow
            {
                temp = 65535;  // Set to maximum value
            }
            input[i].data[j] = temp;
        }

        if (files[input[i].fid].beta < 0 || files[input[i].fid].beta > 1)
            return NULL; // handle error
        if (input[i].cur < 1)
            return NULL;
        for (int j = 0; j < input[i].cur; ++j)
        {
            // input[i].data[j] *= files[input[i].fid].beta;
            // // if (thread_id == 1)
            // //     printf("current result = %f\n", input[i].data[j]);
            double temp = input[i].data[j] * files[input[i].fid].beta;
            if (temp > 65535)  // Check for overflow
            {
                temp = 65535;  // Set to maximum value
            }
            input[i].data[j] = temp;
        }
    }

    size_t max_size = 0;

    for (int i = 0; i < p; ++i)
    {
        if (input[i].cur > max_size)
        {
            max_size = input[i].cur;
        }
    }

    if (max_size > result_size)
    {
        temp_size = (long long)max_size * sizeof(double);
        if (temp_size > 65535) {
            result = realloc(result, 65535);
        } else {
            result = realloc(result, temp_size);
        }

        temp_size = (long long)max_size * sizeof(sem_t);
        if (temp_size > 65535) {
            local_mutex = realloc(local_mutex, 65535);
        } else {
            local_mutex = realloc(local_mutex, temp_size);
        }

        for (size_t current_size = result_size; current_size < max_size; ++current_size)
        {
            result[current_size] = 0;
            sem_init(&local_mutex[current_size], 0, 1);
        }
        result_size = max_size;
        output_size = max_size;
    }
    if (lock_config > 0 && lock_config < 4) {
        for (int i = 0; i < p; ++i)
        {
            for (int j = 0; j < input[i].cur; ++j)
            {
                (*func_ptr[lock_config-1])(i, j, input, result); 
            }
        }
    } else {
        exit(1);  // handle error
    }

    for (int i = 0; i < p; ++i)
    {
        if (input[i].data != NULL)
        {
            free(input[i].data);
            input[i].data = NULL;
        }
    }

    if (filelist != NULL)
    {
        free(filelist);
        filelist = NULL;
    }

    if (checkpoint != NULL)
    {
        free(checkpoint);
        checkpoint = NULL;
    }
    if (thread_args != NULL)
    {
        free(thread_args);
        thread_args = NULL;
    }

    pthread_exit(NULL);
}

void handle_buffer_size(char *value)
{
    buffer_size = atoi(value);
    if (buffer_size < 1)
    {
        printf("Buffer size must be greater than 0.\n");
        exit(1);
    }
}

void handle_num_threads(char *value) {
    num_threads = atoi(value);
    if (num_threads < 1)
    {
        printf("Number of threads must be greater than 0.\n");
        exit(1);
    }
}

void handle_metadata_file_path(char *value) {
    metadata_file_path = value;
}

void handle_lock_config(char *value) {
    lock_config = atoi(value);
    if (lock_config < 1 || lock_config > 3)
    {
        printf("Lock config must be between 1 and 3.\n");
        exit(1);
    }
}

void handle_global_checkpointing(char *value) {
    global_checkpointing = atoi(value);
    if (global_checkpointing < 0 || global_checkpointing > 1)
    {
        printf("Global checkpointing must be 0 or 1.\n");
        exit(1);
    }
}

void handle_output_file_path(char *value) {
    output_file_path = value;
}

void (*handlers[])(char *) = {
    handle_buffer_size,
    handle_num_threads,
    handle_metadata_file_path,
    handle_lock_config,
    handle_global_checkpointing,
    handle_output_file_path
};

int get_index_for_key(char *key) {
    char *keys[] = {
        "buffer_size",
        "num_threads",
        "metadata_file_path",
        "lock_config",
        "global_checkpointing",
        "output_file_path"
    };
    int num_keys = sizeof(keys) / sizeof(char *);

    for (int i = 0; i < num_keys; i++) {
        if (strcmp(key, keys[i]) == 0) {
            return i;
        }
    }

    return -1; 
}

int main(int argc, char *argv[])
{
    
    // Check if the number of arguments is correct
    if (argc != 7)
    {
        printf("Incorrect number of arguments.\n");
        exit(1);
    }

    for (int i = 1; i < argc; ++i)
    {
        char *key = strtok(argv[i], "=");
        char *value = strtok(NULL, "=");

        int index = get_index_for_key(key);
        if (index != -1)
        {
            handlers[index](value);
        }
        else
        {
            printf("Unknown argument: %s\n", key);
            exit(1);
        }
    }

    sem_init(&checkpoint_sem, 0, 0);


    FILE *metafile = fopen(metadata_file_path, "r");
    if (metafile == NULL)
        exit(2); // file cannot open

    char line[256];

    if (fgets(line, 256, metafile))
    {
        num_files = (int)strtol(line, (char **)NULL, 10);
    }
    else
    {
        perror("Error reading from metadata file");
        exit(2);
    }

    if (num_files < 0)
    {
        fprintf(stderr, "Error: Number of files cannot be negative\n");
        exit(2);
    }

    if (num_files % num_threads != 0)
    {
        fprintf(stderr, "Error: Number of files is not a multiple of the number of threads\n");
        exit(2);
    }

    struct channelFile chfiles[num_files];

    for (int f = 0; f < num_files; ++f) {
        fgets(line, 256, metafile);
        line[strcspn(line, "\r\n")] = '\0';

        chfiles[f].fp = fopen(line, "r");
        if (!chfiles[f].fp)
            exit(2);

        fgets(line, 256, metafile);
        chfiles[f].alpha = strtod(line, NULL);

        fgets(line, 256, metafile);
        chfiles[f].beta = strtod(line, NULL);
    }

    FILE *output = fopen(output_file_path, "w");

    if (output == NULL)
        exit(2);
    sem_init(&mutex, 0, 1);
    local_mutex = (sem_t *)malloc(result_size * sizeof(sem_t));
    if (local_mutex == NULL) {
        printf("Error: malloc failed for local_mutex\n");
        free(result);
        exit(2);
    }

    for (int i = 0; i < result_size; ++i)
    {
        sem_init(&local_mutex[i], 0, 1);
    }

    result = (double *)calloc(result_size, sizeof(double));
    if (result == NULL)
        exit(2);

    pthread_t threads[num_threads];
    int tid;
    int rv;

    checkpoint_counters = (int*)malloc(num_threads * sizeof(int));
    if (checkpoint_counters == NULL) {
        printf("Failed to allocate memory for checkpoint_counters\n");
        exit(1); 
    }
    for (int i = 0; i < num_threads; i++) {
        checkpoint_counters[i] = 0;
    }


    for (tid = 0; tid < num_threads; ++tid)
    {
        struct threadArgs *args = malloc(sizeof(struct threadArgs));
        args->tid = tid;
        args->files = chfiles;
        args->output = output;
        rv = pthread_create(&threads[tid], NULL, thread_function, (void *)args);
        if (rv)
            exit(4);
    }

    for (int i = 0; i < num_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < output_size; ++i)
    {
        char bufferLine[256];

        double x = result[i];
        if (x > 65535)  // Check for overflow
        {
            x = 65535;  // Set to maximum value
        }
        else if (x > (int)x)
        {
            x += 1;
        }
        if (i == output_size - 1)
        {
            sprintf(bufferLine, "%d", (int)x);
        }else{
            sprintf(bufferLine, "%d\n", (int)x);
        }

        fputs(bufferLine, output);
        sem_destroy(&local_mutex[i]);
    }


    if (result != NULL)
    {
        free(result);
        result = NULL;
    }

    //free(checkpoint_counters);
    if (checkpoint_counters != NULL)
    {
        free(checkpoint_counters);
        checkpoint_counters = NULL;
    }

    sem_destroy(&mutex);
    if (local_mutex != NULL)
    {
        free(local_mutex);
        local_mutex = NULL;
    }
    return 0;
}