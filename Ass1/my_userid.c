#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERNAME_LENGTH 10
#define MAX_COMMAND_LENGTH 30
#define MAX_USERID_LENGTH 10

int main() {
    char username[MAX_USERNAME_LENGTH];
    char command[MAX_COMMAND_LENGTH];
    char userid[MAX_USERID_LENGTH];

    printf("Enter the username: ");
    scanf("%8s", username); // 8 characters + 1 for '\0', maximum 8 charactors for sfu username

    snprintf(command, MAX_COMMAND_LENGTH, "id -u %s 2>&1", username);

    FILE *command_output = popen(command, "r");
    if (command_output) {
        if (fgets(userid, sizeof(userid), command_output) != NULL && userid[0] >= '0' && userid[0] <= '9') { //remove id: ' ': no such user
            printf("%s", userid);
        } else {
            printf("No such user found\n");
        }
        pclose(command_output);
    } else {
        printf("Command execution failed\n");
    }
    return 0;
}
