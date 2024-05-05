#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define NUM_THREADS 32
#define NUM_ITERATIONS 1000000

pthread_mutex_t lock;
struct timeval start_time, end_time;
long long int total_execution_time = 0;
long long int total_idle_time = 0;
int num_executed_threads = 0;

// Function to simulate workload execution
void *simulate_workload(void *thread_id) {
    int tid = *(int *)thread_id;
    struct timeval start, end;
    double latency;
    long long int thread_execution_time;

    // Record start time
    gettimeofday(&start, NULL);

    // Simulate workload
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Do some computation
        int temp = 0;
        for (int j = 0; j < 1000; j++) {
            temp += j;
        }
    }

    // Record end time
    gettimeofday(&end, NULL);

    // Calculate latency
    latency = (double)(end.tv_sec - start.tv_sec) * 1000000 + (double)(end.tv_usec - start.tv_usec);

    // Update total execution time
    thread_execution_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    pthread_mutex_lock(&lock);
    total_execution_time += thread_execution_time;
    num_executed_threads++;
    pthread_mutex_unlock(&lock);

    // Write latency and thread info to a file
    pthread_mutex_lock(&lock);
    FILE *fp = fopen("thread_metrics.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "Thread %d: Latency: %.2f microseconds\n", tid, latency);
        fclose(fp);
    }
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Record start time
    gettimeofday(&start_time, NULL);

    // Initialize mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex initialization failed.\n");
        return 1;
    }

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        int rc = pthread_create(&threads[i], NULL, simulate_workload, (void *)&thread_ids[i]);
        if (rc) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Record end time
    gettimeofday(&end_time, NULL);

    // Calculate total idle time
    total_idle_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec) - total_execution_time;

    // Write total execution time and idle time to file
    FILE *fp = fopen("system_metrics.txt", "w");
    if (fp != NULL) {
        fprintf(fp, "Total execution time: %lld microseconds\n", total_execution_time);
        fprintf(fp, "Total idle time: %lld microseconds\n", total_idle_time);
        fprintf(fp, "Number of executed threads: %d\n", num_executed_threads);
        fclose(fp);
    }

    // Destroy mutex lock
    pthread_mutex_destroy(&lock);

    return 0;
}
