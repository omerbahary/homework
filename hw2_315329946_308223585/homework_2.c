#include "homework_2.h"


int main(int argc, char* argv[]) {
    // Store the start time
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //Checks if the number of arguments in the Dispatcher is correct
    if (argc != 5) {
        printf("The number of arguments is incorrect! Usage: %s hw2 cmdfile.txt num_threads num_counters log_enabled\n", argv[0]);
        return 1;
    }

    // Parse command-line arguments
    const char* cmdfile = argv[1];
    int num_threads = atoi(argv[2]);
    int num_counters = atoi(argv[3]);
    int log_enabled = atoi(argv[4]);

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
    create_worker_threads(thread_ids, num_threads, work_queue);

    // Wait for worker threads to finish
    for (int j=0; j<num_threads;j++){
        pthread_join(thread_ids[j],NULL);
    }

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

}void* worker_thread(void *arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    struct work_queue* work_queue = data->work_queue;
    int thread_num = data->thread_id;

    while (1) {
        struct job *job = NULL;
        pthread_mutex_lock(&work_queue->mutex);
        if (work_queue->head != NULL) {
            job = work_queue->head;
            work_queue->head = work_queue->head->next;
            if (work_queue->head == NULL) {
                work_queue->tail = NULL;
            }
        }
        pthread_mutex_unlock(&work_queue->mutex);
        if (job == NULL) {
            // Queue is empty, exit thread
            pthread_exit(NULL);
        }
        printf("THREAD ID IS %d\n", thread_num);

        struct timespec job_start_time;
        clock_gettime(CLOCK_MONOTONIC, &job_start_time);
        long long elapsed_time = (job_start_time.tv_sec - start_time.tv_sec) * 1000LL;
        log_start_job(thread_num, elapsed_time, job->command);

        // split the command string by spaces
        char raw_command[1024];
        strcpy(raw_command, job->command);
        char *full_command = job->command;
        char *cmd_token = strtok(full_command, " ");
        char *cmd = cmd_token;
        char *cmd_arg = strtok(NULL, " ");
        if (cmd == NULL)
        {
            pthread_exit(NULL);
        }

        // check the command type
        if (strcmp(cmd, "msleep") == 0) {
            // sleep for the specified number of milliseconds
            int msleep_time = atoi(cmd_arg);
            printf("Sleeping\n");
            usleep(msleep_time * 10000);
        }

        else if (strcmp(cmd, "increment") == 0) {
            // increment the counter in the counter file
            printf("Incrementing\n");
            int x = atoi(cmd_arg);
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
        else if (strcmp(cmd, "decrement") == 0) {
            // decrement the counter in the counter file
            int x = atoi(cmd_arg);
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
        else if (strcmp(cmd, "repeat") == 0) {
            // repeat the sequence of commands x times
            char* repeat_count_char = strtok(cmd_arg, ";");
            int repeat_count = atoi(repeat_count_char);
            char current_token[20];
            char* repeat_token = strtok(raw_command, ";");
            repeat_token = strtok(NULL, ";");
            char* initial_repeat_token = repeat_token;
            int i =0;

            while (i < repeat_count - 1) {
                if (repeat_token == NULL)
                {
                    repeat_token = initial_repeat_token;
                    i++;
                }
                char input[20];
                strcpy(input, repeat_token); // store the current token before advancing
                int repeat_command_arg;
                sscanf(input, "%s %d", current_token, &repeat_command_arg);

                //printf("repeat command is %s\n", current_token);
               // printf("repeat command argument is %d\n", repeat_command_arg);

                if (repeat_token != NULL) {
                    // execute the repeated command
                    if (strstr(current_token, "increment") != NULL) {
                        // increment the counter in the counter file
                        char filename[MAX_COUNTER_NAME_LENGTH];
                        sprintf(filename, "count%02d.txt", repeat_command_arg);

                        int counter_value;
                        FILE *counter_file = fopen(filename, "r+");
                        fscanf(counter_file, "%d", &counter_value);
                        counter_value++;
                        rewind(counter_file);
                        fprintf(counter_file, "%d", counter_value);
                        fclose(counter_file);
                    } else if (strstr(current_token, "decrement") != NULL) {
                        // decrement the counter in the counter file
                        char filename[MAX_COUNTER_NAME_LENGTH];
                        sprintf(filename, "count%02d.txt", repeat_command_arg);

                        int counter_value;
                        FILE *counter_file = fopen(filename, "r+");
                        fscanf(counter_file, "%d", &counter_value);
                        counter_value--;
                        rewind(counter_file);
                        fprintf(counter_file, "%d", counter_value);
                        fclose(counter_file);
                    } else if (strstr(current_token, "msleep") != NULL) {
                        // sleep for the specified number of milliseconds
                        usleep(repeat_command_arg * 1000);
                    } else {
                        fprintf(stderr, "Invalid command: %s\n", current_token);
                        // Handle the error appropriately, e.g., return an error code or exit the program
                    }
                }
                repeat_token = strtok(NULL, ";");
            }
        }
        struct timespec job_end_time;
        clock_gettime(CLOCK_MONOTONIC, &job_end_time);
        long long end_elapsed_time = (job_end_time.tv_sec - start_time.tv_sec) * 1000LL;
        log_end_job(thread_num, end_elapsed_time, job->command);

    }
}

// Function to create worker threads
void create_worker_threads(pthread_t* thread_ids, int num_threads, struct work_queue *work_queue) {
    struct ThreadData thread_data[num_threads];
    for (int i = 0; i < num_threads; i++) {
        create_log_file(i);
        thread_data[i].thread_id = i;
        thread_data[i].work_queue = work_queue;
        if (pthread_create(&thread_ids[i], NULL, worker_thread, (void*)&thread_data[i]) != 0) {
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

        struct timespec cmd_time;
        clock_gettime(CLOCK_MONOTONIC, &cmd_time);
        long long cmd_elapsed_time = (cmd_time.tv_sec - start_time.tv_sec) * 1000LL;
        log_dispatcher(cmd_elapsed_time,line);

        // Check if the line is a dispatcher command
        if (strncmp(line, "dispatcher", 10) == 0) {
            //Every dispatcher line which had been read - write it into the log file.

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
            char* worker_cmd = strtok(line, " ");
            worker_cmd = strtok(NULL, "\n");
            add_job(work_queue, worker_cmd);
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
}
//function to create log files
void create_log_file(int thread_num) {
    if (LOG_ENABLED != 1) {
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
void remove_job(struct work_queue *queue) {
  if (queue->head != NULL) {
    struct job *old_head = queue->head;
    queue->head = old_head->next;
    free(old_head);
  }
}
