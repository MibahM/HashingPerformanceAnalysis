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

#define BSIZE 4096

typedef struct {
    int id;
    int num_threads;
    uint32_t nblocks;
    const uint8_t *data;
    pthread_t thread;
} ThreadData;

uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, uint64_t length);
void *thread_func(void *arg);
void Usage(char *s);

double GetTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    if (argc != 3) Usage(argv[0]);

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    uint32_t nblocks = sb.st_size / BSIZE;
    if (sb.st_size % BSIZE != 0) nblocks++;

    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) Usage(argv[0]);

    const uint8_t *data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ThreadData *threads = malloc(num_threads * sizeof(ThreadData));
    double start = GetTime();

    for (int i = 0; i < num_threads; i++) {
        threads[i].id = i;
        threads[i].num_threads = num_threads;
        threads[i].nblocks = nblocks / num_threads;
        threads[i].data = data + (i * threads[i].nblocks * BSIZE);
        pthread_create(&threads[i].thread, NULL, thread_func, &threads[i]);
    }

    uint32_t final_hash = 0;
    char hash_string[128] = "";
    for (int i = 0; i < num_threads; i++) {
        uint32_t hash_result;
        pthread_join(threads[i].thread, (void **)&hash_result);
        char buffer[32];
        sprintf(buffer, "%u", hash_result);
        strcat(hash_string, buffer);
    }

    final_hash = jenkins_one_at_a_time_hash((const uint8_t *)hash_string, strlen(hash_string));

    double end = GetTime();

    printf("threads = %d\n", num_threads);
    printf("blocks per thread = %u\n", nblocks / num_threads);
    printf("hash value = %u \n", final_hash);
    printf("time taken = %f \n", (end - start));
    printf("------------------------------------------------\n");

    munmap((void *)data, sb.st_size);
    close(fd);
    free(threads);
    return EXIT_SUCCESS;
}

uint32_t jenkins_one_at_a_time_hash(const uint8_t *key, uint64_t length) {
    uint32_t hash = 0;
    for (uint64_t i = 0; i < length; i++) {
        hash += key[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

void *thread_func(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    uint32_t hash = jenkins_one_at_a_time_hash(data->data, data->nblocks * BSIZE);
    return (void *)(uintptr_t)hash;
}

void Usage(char *s) {
    fprintf(stderr, "Usage: %s filename num_threads \n", s);
    exit(EXIT_FAILURE);
}

