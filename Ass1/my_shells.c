#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define Max_LINE_LENGTH 1000

char* Shells[500];

char* getShellName(char* line){
    unsigned int i = strlen(line)-1;
    unsigned int index = 0;
    char *name = malloc(50);

    for(; i >= 0; i--){
        if (line[i-1] == '/'){
            while(line[i] != '\0'){
                name[index] = line[i];
                index++;
                i++;
            }
            name[index+1] = '\0';
            break;
        }
    }
    //printf("%s", name);
    return name;
}

bool checkDuplicate(char* singleShell){
    for(unsigned int i = 0; Shells[i] != NULL; i++){
        if (strcmp(Shells[i], singleShell) == 0){
            return true;
        }
    }
    return false;
}

void getShell(){
    FILE *file;
    char* shellFilePath = "/etc/shells";
    char line[Max_LINE_LENGTH];
    unsigned int shellIndex = 0;

    file = fopen(shellFilePath, "r");
    if (file == NULL){
        printf("File fail to open\n");
    }

    fgets(line,Max_LINE_LENGTH, file); //skip the first line
    while(fgets(line,Max_LINE_LENGTH, file)){
        char* shellName = getShellName(line);
        //printf("%s", shellName);

        //Avoid dulicate shell name
        if (checkDuplicate(shellName) == false){
            Shells[shellIndex] = malloc(strlen(shellName) * sizeof(char));
            strcpy(Shells[shellIndex], shellName);
            Shells[shellIndex+1] = NULL;
            shellIndex++;
       }
    }
    fclose(file);
}

int main(){

    getShell();
    for(unsigned int i = 0; Shells[i] != NULL; i++){
        printf("%s", Shells[i]);
        free(Shells[i]);
    }

}