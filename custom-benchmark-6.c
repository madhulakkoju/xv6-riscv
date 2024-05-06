#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_THREADS 10000
#define NUM_DIRECT_THREADS 5000
#define NUM_STALLING_THREADS 5000

double last_execution_time[NUM_THREADS];
int num_executions[NUM_THREADS];
int num_stopped[NUM_THREADS];
double total_latency[NUM_THREADS];
double total_idle_time[NUM_THREADS];
double total_fast_thread_run_time = 0.0;
double total_stalling_thread_run_time = 0.0;

void stall_cpu() {
    for (int i = 0; i < 10000000; i++) {
        double result = ( 2.0 * i + 9.9 * i ) % 10000000;
    }
}

void *faster_thread_function(void *arg)  {
    long thread_id = (long)arg;
    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);

    stall_cpu();

    gettimeofday(&end_time, NULL);

    double latency = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    total_latency[thread_id] += latency;

    num_executions[thread_id]++;

    last_execution_time[thread_id] = end_time.tv_sec + (end_time.tv_usec / 1000000.0);
    
    double idle_time = (start_time.tv_sec - last_execution_time[thread_id]);
    total_idle_time[thread_id] += idle_time;

    double run_time = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    total_fast_thread_run_time += run_time;

    pthread_exit(NULL);
}

void *slower_thread_function(void *arg) {
    long thread_id = (long)arg;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    stall_cpu();
    stall_cpu();
    stall_cpu();
    stall_cpu();
    stall_cpu();

    gettimeofday(&end_time, NULL);

    double latency = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    total_latency[thread_id] += latency;
    last_execution_time[thread_id] = end_time.tv_sec + (end_time.tv_usec / 1000000.0);
    num_executions[thread_id]++;

    double idle_time = (start_time.tv_sec - last_execution_time[thread_id]);
    total_idle_time[thread_id] += idle_time;

    double run_time = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    total_stalling_thread_run_time += run_time;

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

    output_file = fopen("benchmark_overall_vals.txt", "w");
    if (output_file == NULL) {
        printf("ERROR: Failed to open output file\n");
        exit(-1);
    }

    output_metrics = fopen("benchmark_metrics.csv", "w");
    if (output_file == NULL) {
        printf("ERROR: Failed to open output file\n");
        exit(-1);
    }



    for (int i = 0; i < NUM_THREADS; i++) {
        last_execution_time[i] = 0.0;
        num_executions[i] = 0;
        num_stopped[i] = 0;
        total_latency[i] = 0.0;
        total_idle_time[i] = 0.0;
    }

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double start_cpu = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);

    gettimeofday(&start_time, NULL);

    for (t = 0; t < NUM_THREADS; t++) {

        if(t % 2 == 0){
            rc = pthread_create(&threads[t], NULL, faster_thread_function, (void *)t);
        } else {
            rc = pthread_create(&threads[t], NULL, slower_thread_function, (void *)t);
        }
        
        if (rc) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    gettimeofday(&end_time, NULL);

    total_time = (end_time.tv_sec - start_time.tv_sec) + 
                 (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    getrusage(RUSAGE_SELF, &usage);
    double end_cpu = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000.0);

    cpu_usage = end_cpu - start_cpu;

    double cpu_idle = total_time - cpu_usage;
    double latency = total_time / NUM_THREADS;
    double speed = NUM_THREADS / total_time;

    fprintf(output_file, "Total execution time,%.6f\n", total_time);
    fprintf(output_file, "CPU usage during execution,%.6f\n", cpu_usage);
    fprintf(output_file, "CPU idle time,%.6f\n", cpu_idle);
    fprintf(output_file, "Latency (time per thread),%.6f\n", latency);
    fprintf(output_file, "Speed (threads per second),%.6f\n", speed);

    fprintf(output_metrics, "Thread Type, Thread ID,Last execution time,Executions,interrupts by scheduler,Total Latency,Total Idle Time\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        fprintf(output_metrics, "%d,T %d,%.6f,%d,%d,%.6f,%.6f\n",
                i%2, i, last_execution_time[i], num_executions[i], num_stopped[i], total_latency[i], total_idle_time[i]);
    }

    fprintf(output_file, "\nCumulative run times for each type of thread:\n");
    fprintf(output_file, "Total Direct Execution Thread Run Time,%.6f\n", total_direct_run_time);
    fprintf(output_file, "Total CPU Stalling Thread Run Time,%.6f\n", total_stalling_run_time);

    fclose(output_file);

    printf("Benchmark metrics written to benchmark_metrics.csv\n");

    pthread_exit(NULL);
}
