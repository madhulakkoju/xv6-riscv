#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_THREADS 10000
#define NUM_DIRECT_THREADS 5000
#define NUM_STALLING_THREADS 5000

// Global variables to store metrics
double last_execution_time[NUM_THREADS];
int num_executions[NUM_THREADS];
int num_stopped[NUM_THREADS];
double total_latency[NUM_THREADS];
double total_idle_time[NUM_THREADS];
double total_direct_run_time = 0.0;
double total_stalling_run_time = 0.0;

// Function to stall CPU for some time
void stall_cpu() {
    // Introduce some delay to simulate CPU stall
    for (int i = 0; i < 10000000; i++) {
        // Dummy computation
        double result = 2.0 * 3.0 + i;
    }
}

// Function to be executed by each thread
void *thread_function(void *arg) {
    // Get thread ID
    long thread_id = (long)arg;

    // Record the start time
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Perform computation
    if (thread_id < NUM_DIRECT_THREADS) {
        // Direct execution thread
        // Perform some computation without stalling CPU
        for (int i = 0; i < 10000000; i++) {
            // Some dummy computation
            double result = 2.0 * 3.0 + i;
        }
    } else {
        // CPU stalling thread
        // Stall CPU for some time before performing computation
        stall_cpu();
    }

    // Record the end time
    gettimeofday(&end_time, NULL);

    // Calculate the latency for this execution
    double latency = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // Record the latency
    total_latency[thread_id] += latency;

    // Record the time of last execution
    last_execution_time[thread_id] = end_time.tv_sec + (end_time.tv_usec / 1000000.0);

    // Increment the number of times the thread has been executed
    num_executions[thread_id]++;

    // Calculate the idle time for this execution
    double idle_time = (start_time.tv_sec - last_execution_time[thread_id]);

    // Record the idle time
    total_idle_time[thread_id] += idle_time;

    // Calculate the run time for this thread
    double run_time = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // Update cumulative run times based on thread type
    if (thread_id < NUM_DIRECT_THREADS) {
        // Direct execution thread
        total_direct_run_time += run_time;
    } else {
        // CPU stalling thread
        total_stalling_run_time += run_time;
    }

    // Print thread ID and return
    printf("Thread %ld completed\n", thread_id);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;
    struct timeval start_time, end_time;
    double total_time;
    double cpu_usage;
    FILE *output_file;

    // Open output file
    output_file = fopen("benchmark_metrics.txt", "w");
    if (output_file == NULL) {
        printf("ERROR: Failed to open output file\n");
        exit(-1);
    }

    // Initialize global variables
    for (int i = 0; i < NUM_THREADS; i++) {
        last_execution_time[i] = 0.0;
        num_executions[i] = 0;
        num_stopped[i] = 0;
        total_latency[i] = 0.0;
        total_idle_time[i] = 0.0;
    }

    // Get initial CPU usage
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double start_cpu = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);

    // Get start time
    gettimeofday(&start_time, NULL);

    // Create threads
    for (t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, thread_function, (void *)t);
        if (rc) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Wait for all threads to finish
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Get end time
    gettimeofday(&end_time, NULL);

    // Get total execution time
    total_time = (end_time.tv_sec - start_time.tv_sec) + 
                 (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    // Get final CPU usage
    getrusage(RUSAGE_SELF, &usage);
    double end_cpu = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);

    // Calculate CPU usage during the execution
    cpu_usage = end_cpu - start_cpu;

    // Calculate CPU idle time
    double cpu_idle = total_time - cpu_usage;

    // Calculate latency (time per thread)
    double latency = total_time / NUM_THREADS;

    // Calculate speed (threads per second)
    double speed = NUM_THREADS / total_time;

    // Write metrics to output file
    fprintf(output_file, "Total execution time: %.6f seconds\n", total_time);
    fprintf(output_file, "CPU usage during execution: %.6f seconds\n", cpu_usage);
    fprintf(output_file, "CPU idle time: %.6f seconds\n", cpu_idle);
    fprintf(output_file, "Latency (time per thread): %.6f seconds\n", latency);
    fprintf(output_file, "Speed (threads per second): %.6f\n", speed);

    // Write thread-specific metrics
    fprintf(output_file, "\nThread-specific metrics:\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        fprintf(output_file, "Thread %d - Last execution time: %.6f seconds, Executions: %d, Stopped by scheduler: %d times, Total Latency: %.6f seconds, Total Idle Time: %.6f seconds\n",
                i, last_execution_time[i], num_executions[i], num_stopped[i], total_latency[i], total_idle_time[i]);
    }

    // Write cumulative run times for each type of thread
    fprintf(output_file, "\nCumulative run times for each type of thread:\n");
    fprintf(output_file, "Total Direct Execution Thread Run Time: %.6f seconds\n", total_direct_run_time);
    fprintf(output_file, "Total CPU Stalling Thread Run Time: %.6f seconds\n", total_stalling_run_time);

    // Close output file
    fclose(output_file);

    printf("Benchmark metrics written to benchmark_metrics.txt\n");

    pthread_exit(NULL);
}
