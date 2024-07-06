#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

typedef struct {
    long msg_type;
    char message[100];  
} Message;

int main() {
    Message msg;
    int msgid;
    char userResponse;
    key_t key = ftok("plane.c", 65);

    
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    
    do {
        printf("Do you want to initiate cleanup? (y/n): ");
        scanf(" %c", &userResponse);

        if (userResponse == 'y') {
            msg.msg_type = 60;  //  cleanup signals
            strcpy(msg.message, "cleanup");

            // Send cleanup message to air traffic control and airports
            if (msgsnd(msgid, &msg, sizeof(msg.message), 0) == -1) {
                perror("msgsnd failed");
                exit(EXIT_FAILURE);
            }

            printf("Cleanup message sent. Exiting now.\n");
            break;
        } else {
            printf("Continuing to monitor. Press 'y' and enter to initiate cleanup at any time.\n");
        }
    } while (userResponse != 'y');

    return 0;
}
