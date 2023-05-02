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
};


//functions decleration:
int is_empty(struct work_queue *queue);
void add_job(struct work_queue *queue, char *command);
struct job *pop_job(struct work_queue *queue);
int create_counter_files(int num_counters);
void worker_thread(void *arg);
void create_worker_threads(pthread_t* thread_ids, int num_threads);
void dispatcher(const char* cmdfile, int num_threads, struct work_queue *work_queue);
void cleanup(struct work_queue *queue, pthread_t *threads, int num_threads);
void create_log_file(int thread_num);
void log_start_job(int thread_num, long long start_time, char* job_line);
void log_end_job(int thread_num, long long end_time, char* job_line);
void log_dispatcher(long long time, char* cmd_line);

int main(int argc, char* argv[]) {
    start_time = START_TIME;

    //Checks if the number of arguments in the Dispatcher is correct
    if (argc != 5) {
        printf("The number of arguments is incorrect! Usage: %s hw2 cmdfile.txt num_threads num_counters log_enabled\n", argv[0]);
        return 1;
    }
    //Checks if the command line starts with "hw2"
    if (strcmp(argv[1], "hw2") != 0) {
        printf("ERROR: Invalid command '%s'. Expected 'hw2'\n", argv[1]);
        return 1; 
    }

    // Parse command-line arguments
    const char* cmdfile = argv[2];
    int num_threads = atoi(argv[3]);
    int num_counters = atoi(argv[4]);
    int log_enabled = atoi(argv[5]);

    // Validate arguments
    if (num_threads <= 0 || num_threads > MAX_THREADS) {
        printf("Invalid number of threads: %d\n", num_threads);
        return 1;
    }
    if (num_counters <= 0 || num_counters > MAX_COUNTERS) {
        printf("Invalid number of counters: %d\n", num_counters);
        return 1;
    }
    if(log_enabled==1){
    // Set LOG_ENABLED to 1
        #undef LOG_ENABLED
        #define LOG_ENABLED 1
    }
    struct work_queue* work_queue = malloc(sizeof(struct work_queue));
    work_queue->head = NULL;
    work_queue->tail = NULL;

    // Create counter files 
    create_counter_files(num_counters);

    dispatcher(cmdfile,num_threads,work_queue); 

    // Create an array of pthread_t to hold the thread IDs
    pthread_t thread_ids[num_threads];
    // Create the worker threads
    create_worker_threads(thread_ids, num_threads);

    // Wait for worker threads to finish
    for (int j=0; j<num_threads;j++){
        pthread_join(thread_ids[j],NULL);
    }
    // TO ASK BAHARY HOW DO WE WAIT TO THE THREAD TO BE FINISHED?

    // Free resources
    cleanup(work_queue, thread_ids, num_threads);


    return 0;
}

//checks if the linked list is empty
int is_empty(struct work_queue *queue) {
  return queue->head == NULL;
}

void add_job(struct work_queue *queue, char *command) {
  // Create a new job.
  struct job *job = malloc(sizeof(struct job));
  strncpy(job->command, command, 1023);
  job->next = NULL;

  // If the queue is empty, make the new job the head of the queue.
  if (queue->head == NULL) {
    queue->head = job;
    queue->tail = job;
  } else {
    // Otherwise, append the new job to the end of the queue.
    queue->tail->next = job;
    queue->tail = job;
  }
}

struct job *pop_job(struct work_queue *queue) {
  // If the queue is empty, there is nothing to pop.
  if (queue->head == NULL) {
    return NULL;
  }

  // Get the head of the queue.
  struct job *job = queue->head;

  // Set the head of the queue to the next job.
  queue->head = job->next;

  // Free the memory used by the job.
  free(job);

  // Return the job.
  return job;
}

int create_counter_files(int num_counters) {
    char filename[MAX_COUNTER_NAME_LENGTH];
    long long initial_value = 0;
    for (int i = 0; i < num_counters; i++) {
        snprintf(filename, MAX_COUNTER_NAME_LENGTH, "count%02d.txt", i);
        FILE *fp = fopen(filename, "w"); //Creates new file
        if (fp == NULL) {
            fprintf(stderr, "Error: Failed to create file %s\n", filename);
            exit(EXIT_FAILURE);
        }
        fprintf(fp, "%lld", initial_value);
        fclose(fp);
        }
    return 1;
}
void worker_thread(void *arg) {
    struct job *job = (struct job *)arg;

    struct timeval current_time;
    gettimeofday(&current_time, NULL); //Get the current time
    //calculate the current time
    long long start_time = ((current_time.tv_sec - START_TIME.tv_sec) * 1000LL) + ((current_time.tv_usec - START_TIME.tv_usec) / 1000LL);
    
    // THE LOG FILE SECTION - SECTION 2
    // TO ASK BAHARY HOW TO GET THE NUMBER (i) OF THE THREAD INTO THE THREAD //////
    // Write into the logFile TIME: %lld: START job %s
    // log_start_job(thread_num, start_time, job->command);

    // split the command string by spaces
    char *token = strtok(job->command, " ");

    while (token != NULL) {
        // check the command type
        if (strcmp(token, "msleep") == 0) {
            // sleep for the specified number of milliseconds
            token = strtok(NULL, " ");
            int msleep_time = atoi(token);
            usleep(msleep_time * 1000);
        }
        else if (strcmp(token, "increment") == 0) {
            // increment the counter in the counter file
            token = strtok(NULL, " ");
            int x = atoi(token);
            char filename[MAX_COUNTER_NAME_LENGTH];
            sprintf(filename, "count%02d.txt", x);
            int counter_value;
            FILE *counter_file = fopen(filename, "r+");
            fscanf(counter_file, "%d", &counter_value);
            counter_value++;
            rewind(counter_file);
            fprintf(counter_file, "%d", counter_value);
            fclose(counter_file);
        }
        else if (strcmp(token, "decrement") == 0) {
            // decrement the counter in the counter file
            token = strtok(NULL, " ");
            int x = atoi(token);
            char filename[MAX_COUNTER_NAME_LENGTH];
            sprintf(filename, "count%02d.txt", x);
            int counter_value;
            FILE *counter_file = fopen(filename, "r+");
            fscanf(counter_file, "%d", &counter_value);
            counter_value--;
            rewind(counter_file);
            fprintf(counter_file, "%d", counter_value);
            fclose(counter_file);
        } 
        else if (strcmp(token, "repeat") == 0) {
        // repeat the sequence of commands x times
        token = strtok(NULL, " ");
        int repeat_count = atoi(token);
        char *repeat_command = strchr(job->command, ':') + 1;  // find the start of the command to repeat
        int repeat_length = strlen(repeat_command);
        char *repeat_token = strtok(repeat_command, ";");  // split the repeat command by semicolon
        for (int i = 0; i < repeat_count; i++) {
            while (repeat_token != NULL) {
                // execute the repeated command
                if (strstr(repeat_token, "increment") != NULL) {
                    token = strtok(NULL, " ");
                    int x = atoi(token);
                    char filename[MAX_COUNTER_NAME_LENGTH];
                    sprintf(filename, "count%02d.txt", x);
                    int counter_value;
                    FILE *counter_file = fopen(filename, "r+");
                    fscanf(counter_file, "%d", &counter_value);
                    counter_value++;
                    rewind(counter_file);
                    fprintf(counter_file, "%d", counter_value);
                    fclose(counter_file);
                }
                else if (strstr(repeat_token, "decrement") != NULL) {
                    // decrement the counter in the counter file
                    token = strtok(NULL, " ");
                    int x = atoi(token);
                    char filename[MAX_COUNTER_NAME_LENGTH];
                    sprintf(filename, "count%02d.txt", x);
                    int counter_value;
                    FILE *counter_file = fopen(filename, "r+");
                    fscanf(counter_file, "%d", &counter_value);
                    counter_value--;
                    rewind(counter_file);
                    fprintf(counter_file, "%d", counter_value);
                    fclose(counter_file); 
                }
                else if (strstr(repeat_token, "msleep") != NULL) {
                    // sleep for the specified number of milliseconds
                    token = strtok(NULL, " ");
                    int msleep_time = atoi(token);
                    usleep(msleep_time * 1000);
                }
                 else {
                    fprintf(stderr, "Invalid command: %s\n", repeat_token);
                }
                repeat_token = strtok(NULL, ";");
            }
        // reset the repeat_token for the next iteration
        repeat_token = strtok(repeat_command, ";");
        }
    token = NULL;  // break out of the while loop since the repeat command has been handled
    }
         else {
            fprintf(stderr, "Invalid command: %s\n", token);
            token = strtok(NULL, " ");
        }
    }
    //WORKER THREAD FINISH WRITE INTO THE FILE TIME AND JOB END
    gettimeofday(&current_time, NULL); //Get the current time
    //calculate the current time
    long long finish_time = ((current_time.tv_sec - START_TIME.tv_sec) * 1000LL) + ((current_time.tv_usec - START_TIME.tv_usec) / 1000LL);
    
    // THE LOG FILE SECTION - SECTION 3
    // TO ASK BAHARY HOW TO GET THE NUMBER (i) OF THE THREAD INTO THE THREAD //////
    // Write into the logFile TIME: %lld: END job %s ---- log_end_job is the function
    // log_end_job(thread_num, finish_time, job->command);

    // split the command string by spaces
}
// Function to create worker threads
void create_worker_threads(pthread_t* thread_ids, int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        create_log_file(i);
        if (pthread_create(&thread_ids[i], NULL, worker_thread, NULL) != 0) {
            printf("Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}
//Function to dispatcher 
void dispatcher(const char* cmdfile, int num_threads, struct work_queue *work_queue) {
    // Open the log file //
    FILE* log_file = fopen("dispatcher.txt", "w");
    if (log_file == NULL) {
        printf("Error: Failed to open log file!\n");
        exit(1);
    }

    // Open the command file for reading
    FILE* file = fopen(cmdfile, "r");
    if (file == NULL) {
        printf("Error opening command file %s\n", cmdfile);
        exit(EXIT_FAILURE);
    }
    // Loop through each line in the command file
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove the newline character from the end of the line
        line[strcspn(line, "\n")] = 0;

        // Check if the line is a dispatcher command
        if (strncmp(line, "dispatcher ", 11) == 0) {
            //Every dispatcher line which had been read - write it into the log file.

            struct timeval current_time;
            gettimeofday(&current_time, NULL); //Get the current time
            //calculate the current time
            long long elapsed_time = ((current_time.tv_sec - START_TIME.tv_sec) * 1000LL) + ((current_time.tv_usec - START_TIME.tv_usec) / 1000LL);
    
            log_dispatcher(elapsed_time,line);
            // Parse the command type and argument
            char cmd[20];
            int arg;
            sscanf(line + 11, "%s %d", cmd, &arg);

            // Execute the command
            if (strcmp(cmd, "msleep") == 0) {
                // Sleep for the specified number of milliseconds
                usleep(arg * 1000);
            }
            else if (strcmp(cmd, "wait") == 0) {
                // Wait for all pending background commands to complete
                // Loop until the work queue is empty
                while (!is_empty(work_queue)) {
                    // Sleep for a short time to avoid busy waiting
                    usleep(1000);
                }
            }
            else {
                // Unknown dispatcher command
                printf("Unknown dispatcher command: %s\n", line);
            }
        }
// Otherwise, the line is a job for a worker thread
       else if (strncmp(line, "worker ", 7) == 0) {
        // Parse the job commands and arguments
        struct job j;
        // Add the command to the job and add it to work_queue
            add_job(work_queue, line +6);
            // the +6 is to copy without the word worker

       }
    }
}
// Function to free the memory has been used:
void cleanup(struct work_queue *queue, pthread_t *threads, int num_threads) {
    // Free any remaining jobs in the work queue.
    while (!is_empty(queue)) {
        struct job *job = pop_job(queue);
        free(job);
    }

    // Free the work queue itself.
    free(queue);

    // Close any open file pointers.
    for (int i = 0; i < MAX_COUNTERS; i++) {
        char filename[MAX_COUNTER_NAME_LENGTH];
        sprintf(filename, "count%02d.txt", i);
        fclose(fopen(filename, "r+"));
    }

    // Destroy any created threads.
    for (int i = 0; i < num_threads; i++) {
        pthread_cancel(threads[i]);
    }
}
//function to create log files
void create_log_file(int thread_num) {
    if (!LOG_ENABLED) {
        return;
    }

    char filename[20];
    sprintf(filename, "thread%02d.txt", thread_num); // The first is thread00, thread01 and go forth
    FILE* log_file = fopen(filename, "w");
    if (log_file == NULL) {
        perror("Failed to create log file");
        exit(1);
    }
    fclose(log_file);
}

void log_start_job(int thread_num, long long start_time, char* job_line) {
    if (!LOG_ENABLED) {
        return;
    }

    char filename[20];
    sprintf(filename, "thread%02d.txt", thread_num);
    FILE* log_file = fopen(filename, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(1);
    }
    fprintf(log_file, "TIME %lld: START job %s\n", start_time, job_line);
    fclose(log_file);
}

void log_end_job(int thread_num, long long end_time, char* job_line) {
    if (!LOG_ENABLED) {
        return;
    }

    char filename[20];
    sprintf(filename, "thread%02d.txt", thread_num);
    FILE* log_file = fopen(filename, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(1);
    }
    fprintf(log_file, "TIME %lld: END job %s\n", end_time, job_line);
    fclose(log_file);
}

void log_dispatcher(long long time, char* cmd_line) {
    if (!LOG_ENABLED) {
        return;
    }

    FILE* log_file = fopen("dispatcher.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(1);
    }
    fprintf(log_file, "TIME %lld: read cmd line: %s\n", time, cmd_line);
    fclose(log_file);
}
