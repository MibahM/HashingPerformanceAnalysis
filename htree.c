#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define BSIZE 4096  // Block size in bytes

// Structure to store thread-specific data
typedef struct {
    int id;  // Thread ID
    int num_threads;  // Total number of threads
    uint32_t nblocks;  // Number of blocks assigned to this thread
    const uint8_t *data;  // Pointer to data this thread will process
    pthread_t thread;  // Thread handle
} ThreadData;

// Hash function definition
uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, uint64_t length);

// Thread function definition
void *thread_func(void *arg);

// Function to display usage information
void Usage(char *s);

// Function to get the current time in seconds
double GetTime() {
    struct timespec ts;  // Structure for holding time information
    clock_gettime(CLOCK_MONOTONIC, &ts);  // Get current time
    return ts.tv_sec + ts.tv_nsec / 1e9;  // Return time as seconds
}

int main(int argc, char **argv) {
    // Ensure correct number of arguments are provided
    if (argc != 3) Usage(argv[0]);

    // Open the file for reading
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {  // Handle error
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    // Get file information
    struct stat sb;  // Structure for holding file statistics
    if (fstat(fd, &sb) == -1) {  // Handle error
        perror("fstat failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Calculate the number of blocks based on the file size
    uint32_t nblocks = sb.st_size / BSIZE;
    if (sb.st_size % BSIZE != 0) nblocks++;  // Adjust if there's a remainder

    // Get the number of threads from the arguments
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) Usage(argv[0]);  // Check if threads are positive

    // Map the file into memory for shared read access
    const uint8_t *data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {  // Handle error
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for thread data
    ThreadData *threads = malloc(num_threads * sizeof(ThreadData));

    // Start timing
    double start = GetTime();

    // Create threads and assign them data
    for (int i = 0; i < num_threads; i++) {
        threads[i].id = i;
        threads[i].num_threads = num_threads;
        threads[i].nblocks = nblocks / num_threads;  // Distribute blocks equally
        threads[i].data = data + (i * threads[i].nblocks * BSIZE);  // Set data pointer for this thread
        pthread_create(&threads[i].thread, NULL, thread_func, &threads[i]);  // Create thread
    }

    // Final hash and related variables
    uint32_t final_hash = 0;
    char hash_string[128] = "";  // String to store intermediate hash results

    // Join threads and collect their results
    for (int i = 0; i < num_threads; i++) {
        uint32_t hash_result;
        pthread_join(threads[i].thread, (void **)&hash_result);  // Wait for thread to finish
        char buffer[32];  // Buffer for string conversion
        sprintf(buffer, "%u", hash_result);  // Convert hash result to string
        strcat(hash_string, buffer);  // Append to hash string
    }

    // Compute the final hash using concatenated thread results
    final_hash = jenkins_one_at_a_time_hash((const uint8_t *)hash_string, strlen(hash_string));

    // End timing
    double end = GetTime();

    // Output results
    printf("threads = %d\n", num_threads);
    printf("blocks per thread = %u\n", nblocks / num_threads);
    printf("hash value = %u \n", final_hash);
    printf("time taken = %f \n", (end - start));  // Display elapsed time
    printf("------------------------------------------------\n");

    // Clean up and release resources
    munmap((void *)data, sb.st_size);  // Unmap the memory
    close(fd);  // Close the file descriptor
    free(threads);  // Free the allocated memory
    return EXIT_SUCCESS;  // Indicate successful execution
}

// Jenkins one-at-a-time hash function
uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, uint6464 length) {
    uint32_t hash = 0;  // Initial hash value
    for (uint64_t i = 0; i < length; i++) {
        hash += key[i];  // Add current byte
        hash += hash << 10;  // Left shift and add to hash
        hash ^= hash >> 6;  // Right shift and XOR with hash
    }
    hash += hash << 3;  // Further hash mixing
    hash ^= hash >> 11;  // Right shift and XOR again
    hash += hash << 15;  // Final left shift and add
    return hash;  // Return the computed hash value
}

// Function executed by each thread to compute its hash
void *thread_func(void *arg) {
    ThreadData *data = (ThreadData *)arg;  // Cast argument to ThreadData
    uint32_t hash = jenkins_one_at_a_time_hash(data->data, data->nblocks * BSIZE);  // Compute hash
    return (void *)(uintptr_t)hash;  // Return the hash as a pointer
}

// Function to display correct usage information
void Usage(char *s) {
    fprintf(stderr, "Usage: %s filename num_threads \n", s);  // Display error message
    exit(EXIT_FAILURE);  // Exit with failure status
}
