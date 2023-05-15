#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h> //to use usleep
#include <time.h>
#include <sys/time.h>

#define MAX_THREADS 4096
#define MAX_COUNTERS 100
#define MAX_COUNTER_NAME_LENGTH 15
#define LOG_ENABLED 0
#define START_TIME get_start_time()

struct timeval start_time;

struct timeval get_start_time() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return current_time;
}

/* Define the job struct */
struct job {
    char command[1024];
    struct job *next;
};

/* Define the work queue struct - this is a linked list*/
struct work_queue {
    struct job *head;
    struct job *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond_q_empty;
    int no_more_jobs;
    int curr_num_jobs;
    clock_t hw2_start_time;
};

// Structure to hold thread ID and work queue pointer
struct ThreadData {
    int thread_id;
    struct work_queue* work_queue;
};

// Structure to hold job statistics
typedef struct {
    double total_running_time;
    double sum_of_job_turnaround_time;
    double min_job_turnaround_time;
    double max_job_turnaround_time;
    int num_jobs;
} JobStatistics;

//functions decleration:
int is_empty(struct work_queue *queue);
void add_job(struct work_queue *queue, char *command);
struct job *pop_job(struct work_queue *queue);
int create_counter_files(int num_counters);
void* worker_thread(void *arg);
void create_worker_threads(pthread_t* thread_ids, int num_threads, struct work_queue *work_queue, struct ThreadData *thread_data);
void dispatcher(const char* cmdfile, int num_threads, struct work_queue *work_queue, pthread_t* thread_ids, struct ThreadData *thread_data);
void cleanup(struct work_queue *queue, pthread_t *threads, int num_threads);
void create_log_file(int thread_num);
void log_start_job(int thread_num, double start_time, char* job_line);
void log_end_job(int thread_num, double end_time, char* job_line);
void log_dispatcher(double time, char* cmd_line);
void remove_job(struct work_queue *queue);
void display_statistics(JobStatistics* job_stats);
