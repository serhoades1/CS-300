/*
*Name: Stephen Rhoades
*CWID: 12387116
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define SHM_NAME "time_shm"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create shared memory region
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Resize the shared memory to hold struct timeval
    if (ftruncate(shm_fd, sizeof(struct timeval)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Map the shared memory
    struct timeval *shared_time = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_time == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        munmap(shared_time, sizeof(struct timeval));
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        printf("Child PID: %d\n", getpid());
        // Record the start time
        if (gettimeofday(shared_time, NULL) == -1) {
            perror("gettimeofday");
            exit(EXIT_FAILURE);
        }

        // Execute the command
        execvp(argv[1], &argv[1]);
        // If execvp fails
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    else { // Parent process
        printf("Parent PID: %d\n", getpid());
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);

        // Record the end time
        struct timeval end_time;
        if (gettimeofday(&end_time, NULL) == -1) {
            perror("gettimeofday");
            munmap(shared_time, sizeof(struct timeval));
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        // Calculate elapsed time
        double start_seconds = shared_time->tv_sec + (shared_time->tv_usec / 1000000.0);
        double end_seconds = end_time.tv_sec + (end_time.tv_usec / 1000000.0);
        double elapsed_time = end_seconds - start_seconds;

        // Print elapsed time
        printf("Elapsed time: %.5f seconds\n", elapsed_time);

        // Cleanup shared memory
        munmap(shared_time, sizeof(struct timeval));
        shm_unlink(SHM_NAME);
    }

    return 0;
}
