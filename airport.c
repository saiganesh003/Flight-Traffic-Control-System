#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define BACKUP_RUNWAY_CAPACITY 15000
#define MAX_RUNWAYS 10  
#define MAX_PLANES 1000 

typedef struct {
    int capacity;
    int occupied;  // 0 if free, 1 if occupied
    pthread_mutex_t lock;
} Runway;

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
    Plane plane;
    int airport_id;
    Runway *runways;
    int num_runways;  
} ThreadArgs;

Runway runways[MAX_RUNWAYS + 1]; 
int num_runways;
pthread_t plane_threads[MAX_PLANES];

void *process_plane(void *arg) {
   
    ThreadArgs *args = (ThreadArgs *)arg;
    Plane plane = args->plane;
    int selected_index = -1;
    int min_capacity_diff = 1000000;

    // First try to find a suitable main runway
    for (int i = 0; i < args->num_runways; i++) {
        pthread_mutex_lock(&args->runways[i].lock);
        if (!args->runways[i].occupied) {
            int capacity_diff = args->runways[i].capacity - plane.total_weight;
            if (capacity_diff >= 0 && capacity_diff < min_capacity_diff) {
                if (selected_index != -1) {
                    args->runways[selected_index].occupied = 0;
                    pthread_mutex_unlock(&args->runways[selected_index].lock);
                }
                selected_index = i;
                min_capacity_diff = capacity_diff;
                args->runways[i].occupied = 1; // This runway is now occupied
                pthread_mutex_unlock(&args->runways[i].lock);

            } else {
                pthread_mutex_unlock(&args->runways[i].lock);
            }
        } else {
            pthread_mutex_unlock(&args->runways[i].lock);
        }
    }

    

    // If no main runway is suitable, check the backup runway
    if (selected_index == -1) {
        int backup_index = args->num_runways;  // Backup runway index
        pthread_mutex_lock(&args->runways[backup_index].lock);
        if (!args->runways[backup_index].occupied && args->runways[backup_index].capacity >= plane.total_weight) {
            selected_index = backup_index;
            args->runways[backup_index].occupied = 1;  // Use the backup runway
        }
        pthread_mutex_unlock(&args->runways[backup_index].lock);
    }

    if (selected_index == -1) {
        printf("All runways are currently occupied. Waiting for a runway to become available...\n");
        usleep(100000);  // Wait and retry
        return process_plane(arg);
    }
    

    
    //pthread_mutex_lock(&args->runways[selected_index].lock);
    
    if (args->airport_id == plane.departure_airport) {
        sleep(3);
        printf("Plane %d has completed boarding/loading and taken off from Airport %d on Runway %d.\n", plane.plane_id, args->airport_id, selected_index + 1);
        sleep(2);  // Simulate takeoff time
        //printf("Plane %d has taken off from Airport %d on Runway  %d.\n", plane.plane_id, args->airport_id, selected_index + 1);
        args->runways[selected_index].occupied = 0;  // Free the runway
        //pthread_mutex_unlock(&args->runways[selected_index].lock);
        PlaneMessage msg;
        int msgid = msgget(ftok("plane.c", 65), 0666 | IPC_CREAT);

        msg.msg_type = 100 + plane.plane_id;
        msgsnd(msgid, &msg, sizeof(msg), 0);  // takeoff done

        sleep(30); // Journey time
        // Notify arrival airport that the plane is ready for landing
       
        msg.msg_type = 20000 + plane.plane_id;
        msgsnd(msgid, &msg, sizeof(msg), 0);  // Send ready for landing message
        
    } else {
       

        // Wait for takeoff and journey completion message
        int msgid = msgget(ftok("plane.c", 65), 0666 | IPC_CREAT);
        PlaneMessage msg;
        
        msgrcv(msgid, &msg, sizeof(msg), 20000 + plane.plane_id, 0);

       

        args->runways[selected_index].occupied = 1;  // Re-occupy the runway for landing
       // printf("Plane %d is now landing at Airport %d on Runway %d.\n", plane.plane_id, args->airport_id, selected_index + 1);

        sleep(2);  // Simulate landing time
        printf("Plane %d has landed at Airport %d on Runway %d.\n", plane.plane_id, args->airport_id, selected_index + 1);
        sleep(3);
        printf("Plane %d has completed deboarding/unloading at Airport %d on Runway %d.\n", plane.plane_id, args->airport_id, selected_index + 1);
        //msg.msg_type = 10000 + plane.plane_id;  // Unique message type for landing confirmation
       // msgsnd(msgid, &msg, sizeof(msg.plane), 0);
        args->runways[selected_index].occupied = 0;  // Free the runway
        //pthread_mutex_unlock(&args->runways[selected_index].lock);
        
    }
    //pthread_mutex_unlock(&args->runways[selected_index].lock);
    //printf("Operation completed for Plane %d on Runway %d at Airport %d.\n", plane.plane_id, selected_index + 1, args->airport_id);

    free(arg);
    return NULL;
}








void *handle_departure(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    //printf("Handling departure for Plane %d at Airport %d.\n", args->plane.plane_id, args->airport_id);
    process_plane(arg); 
    return NULL;
}

void *handle_arrival(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    //printf("Handling arrival for Plane %d at Airport %d.\n", args->plane.plane_id, args->airport_id);
    Plane plane = args->plane;

    process_plane(arg); 
    int msgid = msgget(ftok("plane.c", 65), 0666 | IPC_CREAT);
    PlaneMessage msg;
    msg.msg_type = 10000 + plane.plane_id;  //  message type for landing confirmation
    msgsnd(msgid, &msg, sizeof(msg.plane), 0);
    
    return NULL;
}
void checktermination(int msgid){
    
    PlaneMessage msg;
    
    if (msgrcv(msgid, &msg, sizeof(msg), 8, IPC_NOWAIT) != -1) {
        

        exit(0);
    }
    

}

int main() {
    int airport_id;
    printf("Enter Airport ID:\n");
    scanf("%d", &airport_id);
    printf("Enter number of Runways:\n");
    scanf("%d", &num_runways);

    printf("Enter loadCapacity of Runways (give as a space-separated list in a single line):\n");
    for (int i = 0; i <= num_runways; i++) {  // Including backup runway
        if (i < num_runways) {
            scanf("%d", &runways[i].capacity);
        } else {
            runways[i].capacity = BACKUP_RUNWAY_CAPACITY;
        }
        runways[i].occupied = 0;
        pthread_mutex_init(&runways[i].lock, NULL);
    }

    int msgid;
    key_t key = ftok("plane.c", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    while (1) {
        checktermination(msgid);
        PlaneMessage msg;
        if (msgrcv(msgid, &msg, sizeof(msg.plane), 1000 + airport_id, IPC_NOWAIT) != -1) {
            pthread_t tid;
            ThreadArgs *args = malloc(sizeof(ThreadArgs));
            args->plane = msg.plane;
            args->airport_id = airport_id;
            args->runways = runways;
            args->num_runways = num_runways;
            int plane_index = msg.plane.plane_id % MAX_PLANES;  
            pthread_create(&plane_threads[plane_index], NULL, handle_departure, args);
            //pthread_detach(plane_threads[plane_index]);
        } else if (msgrcv(msgid, &msg, sizeof(msg.plane), 2000 + airport_id, IPC_NOWAIT) != -1) {
            pthread_t tid;
            ThreadArgs *args = malloc(sizeof(ThreadArgs));
            args->plane = msg.plane;
            args->airport_id = airport_id;
            args->runways = runways;
            args->num_runways = num_runways;
            int plane_index = msg.plane.plane_id % MAX_PLANES;  
            pthread_create(&plane_threads[plane_index], NULL, handle_arrival, args);
           // pthread_detach(plane_threads[plane_index]);
        }
                

    }

    for (int i = 0; i <= num_runways; i++) {
        pthread_mutex_destroy(&runways[i].lock);
    }
    //msgctl(msgid, IPC_RMID, NULL);  // Clean up message queue

    return 0;
}
