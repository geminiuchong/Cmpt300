#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define Txt_File "my_storage_use.txt"
#define MAX_LINE_LENGTH 1000

//Print the file system usage information to a .txt file
void getUsage(){
    char usage_command[] = "df -h ";
    char* getUsageCommand = malloc(strlen(usage_command) + strlen(Txt_File) + 3);
    strcpy(getUsageCommand, usage_command);
    strcat(getUsageCommand, ">> ");
    strcat(getUsageCommand, Txt_File);
    int status = system(getUsageCommand);
    if (status == -1){
        printf("command failed");
    }
}

//Average the use% column
double averageUage(){
    FILE *file = fopen(Txt_File, "r");
    char line[MAX_LINE_LENGTH];
    int count = 0;
    double sum;

    while(fgets(line, sizeof(line), file) != NULL){
        double single_usage;
        if (sscanf(line, "%*s %*s %*s %*s %lf", &single_usage) == 1) {
            sum += single_usage;
            count++;
        }
    }
    fclose(file);
    printf("%f, %i\n", sum, count);
    double result = sum/count;
    return result;

}

int main(){
    getUsage();
    printf("%.2f\n", averageUage());

    return 0;
}
