#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#define MAX_VEHICLES 1000
#define MAX_LINE_LENGTH 100

// Structure representing a vehicle.
typedef struct {
    char model[50];
    char type; // 'C' for car, 'T' for truck (case insensitive)
} Vehicle;

// Shared structure representing the state of the parking garage.
typedef struct {
    sem_t mutex;       // Semaphore for mutual exclusion.
    int currentType;   // 0 = empty, 1 = Car, 2 = Truck.
    int countInside;   // Number of vehicles currently parked.
    int spots;         // Total number of parking spots.
} Garage;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_spots> <number_of_vehicles>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int spots = atoi(argv[1]);
    int totalVehicles = atoi(argv[2]);
    if (spots <= 0 || totalVehicles <= 0) {
        fprintf(stderr, "Both number of spots and vehicles must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    // Open the input file "vehicles.txt". Adjust the file path if necessary.
    FILE *fp = fopen("vehicles.txt", "r");
    if (!fp) {
        perror("Error opening vehicles.txt");
        exit(EXIT_FAILURE);
    }

    // Read vehicle data from file.
    Vehicle vehicles[MAX_VEHICLES];
    int count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fp) && count < totalVehicles) {
        // Expecting lines in the format: "C Toyota" or "T Freightliner"
        char type;
        char model[50];
        if (sscanf(line, " %c %s", &type, model) == 2) {
            vehicles[count].type = type;
            strcpy(vehicles[count].model, model);
            count++;
        }
    }
    fclose(fp);
    if (count < totalVehicles) {
        fprintf(stderr, "Not enough entries in vehicles.txt. Expected %d, got %d\n", totalVehicles, count);
        exit(EXIT_FAILURE);
    }

    // Create a shared memory region for the garage state.
    Garage *garage = mmap(NULL, sizeof(Garage),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS,
                          -1, 0);
    if (garage == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the garage.
    sem_init(&garage->mutex, 1, 1);
    garage->currentType = 0; // 0 means the garage is empty.
    garage->countInside = 0;
    garage->spots = spots;

    // Seed the random number generator.
    srand(time(NULL));

    // Fork a process for each vehicle.
    for (int i = 0; i < totalVehicles; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) { // Child process: simulate a vehicle arriving.
            Vehicle v = vehicles[i];
            int vehicleType = (v.type == 'C' || v.type == 'c') ? 1 : 2;
            printf("Vehicle %s (%c) arrives. PID: %d\n", v.model, v.type, getpid());

            // Attempt to enter the garage.
            int parked = 0;
            while (!parked) {
                sem_wait(&garage->mutex);
                // Check if the garage is empty or occupied by the same vehicle type,
                // and that there is an available spot.
                if ((garage->currentType == 0 || garage->currentType == vehicleType) &&
                    (garage->countInside < garage->spots)) {
                    // If the garage is empty, set the current type.
                    if (garage->currentType == 0)
                        garage->currentType = vehicleType;
                    garage->countInside++;
                    printf("Vehicle %s (%c) parks. Occupancy: %d/%d\n",
                           v.model, v.type, garage->countInside, garage->spots);
                    sem_post(&garage->mutex);
                    parked = 1;
                } else {
                    sem_post(&garage->mutex);
                    // Wait before trying again.
                    usleep(100000); // 100 ms.
                }
            }

            // Simulate parking duration.
            sleep(1 + rand() % 3);  // Park for 1-3 seconds.

            // Leave the garage.
            sem_wait(&garage->mutex);
            garage->countInside--;
            printf("Vehicle %s (%c) leaves. Remaining: %d\n", v.model, v.type, garage->countInside);
            if (garage->countInside == 0) {
                // If garage becomes empty, reset the current type.
                garage->currentType = 0;
            }
            sem_post(&garage->mutex);
            exit(EXIT_SUCCESS);
        }
    }

    // Parent process waits for all vehicle processes to complete.
    for (int i = 0; i < totalVehicles; i++) {
        wait(NULL);
    }

    // Clean up shared memory and semaphore.
    sem_destroy(&garage->mutex);
    munmap(garage, sizeof(Garage));

    printf("All vehicles have been processed. Garage simulation complete.\n");
    return EXIT_SUCCESS;
}
