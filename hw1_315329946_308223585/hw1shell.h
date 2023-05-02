#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_COMMAND_LENGTH 1024
#define MAX_BACKGROUND_JOBS 4

typedef struct {
    int job_id;
    pid_t pid;
    char *command;
    int is_empty;
} job_t;

job_t background_jobs[MAX_BACKGROUND_JOBS];
int num_background_jobs = 0;
int num_bg_jobs_running = 0;
int is_background = 0;
int args_size = 0;

void execute_command(char **args, int num_args);
void internal_exit();
void internal_cd(char **args, int num_args);
void list_jobs();
void external_command(char **args, int num_args);
int add_job(pid_t pid, char *command);