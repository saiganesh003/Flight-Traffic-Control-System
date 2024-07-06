#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PASSENGERS 10
#define CREW_WEIGHT 75
#define CREW_COUNT_PASSENGER 7
#define CREW_COUNT_CARGO 2

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

int main() {
    Plane plane;
    int weight_pipe[2];

    printf("Enter Plane ID:\n");
    scanf("%d", &plane.plane_id);
    printf("Enter Type of Plane:\n");
    scanf("%d", &plane.type_of_plane);

    if (plane.type_of_plane == 1) {
        printf("Enter Number of Occupied Seats:\n");
        scanf("%d", &plane.num_of_occupied_seats);

        if (pipe(weight_pipe) != 0) {
            perror("Failed to create pipe");
            exit(EXIT_FAILURE);
        }

        int total_passenger_weight = 0;
        for (int i = 0; i < plane.num_of_occupied_seats; i++) {
            pid_t pid = fork();
            if (pid == 0) {
                close(weight_pipe[0]);
                int luggage_weight, body_weight, total_weight;
                printf("Enter Weight of Your Luggage for Passenger %d:\n", i + 1);
                scanf("%d", &luggage_weight);
                printf("Enter Your Body Weight for Passenger %d:\n", i + 1);
                scanf("%d", &body_weight);
                total_weight = luggage_weight + body_weight;
                write(weight_pipe[1], &total_weight, sizeof(total_weight));
                close(weight_pipe[1]);
                exit(0);
            } else if (pid > 0) {
                int status;
                wait(&status);
            } else {
                perror("Fork failed");
                exit(EXIT_FAILURE);
            }
        }

        close(weight_pipe[1]);
        int passenger_weight;
        while (read(weight_pipe[0], &passenger_weight, sizeof(passenger_weight)) > 0) {
            total_passenger_weight += passenger_weight;
        }
        close(weight_pipe[0]);
        plane.total_weight = total_passenger_weight + CREW_WEIGHT * CREW_COUNT_PASSENGER;
    } else {
        printf("Enter Number of Cargo Items:\n");
        scanf("%d", &plane.num_cargo_items);
        printf("Enter Average Weight of Cargo Items:\n");
        scanf("%d", &plane.avg_cargo_weight);
        plane.total_cargo_weight = plane.num_cargo_items * plane.avg_cargo_weight;
        plane.total_weight = plane.total_cargo_weight + CREW_WEIGHT * CREW_COUNT_CARGO;
    }

    printf("Enter Airport Number for Departure:\n");
    scanf("%d", &plane.departure_airport);
    printf("Enter Airport Number for Arrival:\n");
    scanf("%d", &plane.arrival_airport);

    int msgid;
    key_t key = ftok("plane.c", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    PlaneMessage msg;
    msg.msg_type = 11;  // Message type for air traffic control
    msg.plane = plane;
    if (msgsnd(msgid, &msg, sizeof(msg.plane), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    printf("Sent plane details to air traffic control.\n");

    // Wait for final confirmation
    if (msgrcv(msgid, &msg, sizeof(msg), 10000+plane.plane_id, 0) == -1) {
        perror("msgrcv error in plane.c for final confirmation");
        exit(EXIT_FAILURE);
    }

    msg.msg_type = 30000;  // Message type for air traffic control
    msg.plane = plane;
    if (msgsnd(msgid, &msg, sizeof(msg.plane), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n",
           plane.plane_id, plane.departure_airport, plane.arrival_airport);
    return 0;
}
