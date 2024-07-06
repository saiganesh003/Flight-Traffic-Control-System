#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

typedef struct {
    int plane_id;
    int type_of_plane;
    int num_of_occupied_seats;
    int total_weight;
    int num_cargo_items;
    int avg_cargo_weight;
    int total_cargo_weight;
    int departure_airport;
    int arrival_airport;
} Plane;

typedef struct {
    long msg_type;
    Plane plane;
} PlaneMessage;

typedef struct {
    long msg_type;
    char message[100];
} Message;

static int active_planes = 0;
static int cleanup_requested = 0;


void checkForCleanup(int msgid,int num_airports) {
    Message msg1;
    PlaneMessage msg;
    if (msgrcv(msgid, &msg1, sizeof(msg1.message), 60, IPC_NOWAIT) != -1) {
        printf("Cleanup requested. Checking conditions...\n");
        cleanup_requested = 1;
        // if (active_planes == 0) {
        //     printf("No active planes. Proceeding with cleanup...\n");
        //     Plane p = msg1.plane;
        //     for(int i=0;i<=num_airports;i++){
            
        //         msg1.msg_type = 8; // Departure message type
        //         msgsnd(msgid, &msg, sizeof(msg1.plane), 0);}
            
        //     sleep(2);


        //     msgctl(msgid, IPC_RMID, NULL);
        //     exit(0);
        // }
    }
    if (cleanup_requested==1&&active_planes == 0) {
            printf("No active planes. Proceeding with cleanup...\n");
            Plane p = msg.plane;
            for(int i=0;i<=num_airports;i++){
               
                msg.msg_type = 8; // Departure message type
                msgsnd(msgid, &msg, sizeof(msg.plane), 0);}
            
            sleep(2);


            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }
}

void handleLandingConfirmation(int msgid,int num_airports) {
    PlaneMessage msg;
    
    if (msgrcv(msgid, &msg, sizeof(msg.plane), 30000, IPC_NOWAIT) != -1) {
        active_planes--;  // Decrement for each landing
        printf("Plane %d confirmed landed. Active planes: %d\n", msg.plane.plane_id, active_planes);
        if (cleanup_requested && active_planes == 0) {
            printf("Cleanup conditions met. Exiting.\n");
            Plane p = msg.plane;
            for(int i=0;i<=num_airports;i++){
            
                msg.msg_type = 8; 
                msgsnd(msgid, &msg, sizeof(msg.plane), 0);}
            
            sleep(2);

            msgctl(msgid, IPC_RMID, NULL);
            exit(0);
        }
    }
    else if (cleanup_requested && active_planes == 0) {
        Plane p = msg.plane;
        for(int i=0;i<=num_airports;i++){
            
            msg.msg_type = 8; 
            msgsnd(msgid, &msg, sizeof(msg.plane), 0);}
            
        sleep(2);

        msgctl(msgid, IPC_RMID, NULL);
        exit(0);

    }

}

int main() {
    PlaneMessage msg;
    int msgid;
    key_t key;

    int num_airports;

    printf("enter no of airports: ");
    scanf("%d",&num_airports);

    key = ftok("plane.c", 65);
    if (key == -1) {
        perror("ftok error");
        return EXIT_FAILURE;
    }

    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget error");
        return EXIT_FAILURE;
    }

    printf("Air Traffic Control Ready and Receiving...\n");
    

    while (1) {
        handleLandingConfirmation(msgid,num_airports);
        checkForCleanup(msgid,num_airports);
        
        if (msgrcv(msgid, &msg, sizeof(msg.plane), 11, IPC_NOWAIT) != -1 && !cleanup_requested) {
                active_planes++;
                Plane p = msg.plane;

               


                msg.msg_type = 1000 + p.departure_airport; // Departure message type
                msgsnd(msgid, &msg, sizeof(msg.plane), 0);

                
                sleep(1);
                

                
                FILE *file = fopen("AirTrafficController.txt", "a");
                if (file == NULL) {
                    perror("Failed to open or create file");
                    exit(EXIT_FAILURE); 
                }

                // Append the journey information to the file
                fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n",
                        p.plane_id, p.departure_airport, p.arrival_airport);

                // Close the file
                fclose(file);


                // Send arrival information to arrival airport
                msg.msg_type = 2000 + p.arrival_airport; // Arrival message type
                msgsnd(msgid, &msg, sizeof(msg.plane), 0);

                printf("Forwarded Plane ID: %d details to airports %d (takeoff) and %d (landing)\n", p.plane_id, p.departure_airport, p.arrival_airport);
                       
            }
       
        

         
    }

    printf("Cleaning up message queue...\n");
    msgctl(msgid, IPC_RMID, NULL); 

    return 0;
}
