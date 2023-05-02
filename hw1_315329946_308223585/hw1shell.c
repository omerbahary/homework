#include "hw1shell.h"

int main(int argc, char **argv) {
    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    int num_args;
    int status;
    int curr_pid;

    while (1) {
        // Check for completed background jobs
        for (int i=0; i<MAX_BACKGROUND_JOBS; i++){
            if (background_jobs[i].is_empty != 0){ // if process is not empty
                pid_t result = waitpid(background_jobs[i].pid, &status, WNOHANG);
                if (result == 0) { //child still running
                    continue;
                }
                else if (result>0){ // Clean zombie or error
                    curr_pid = background_jobs[i].pid;
                    background_jobs[i].command = NULL;
                    background_jobs[i].pid = 0;
                    background_jobs[i].is_empty = 0;
                    for (int j = i; j < num_background_jobs - 1; j++) {
                        background_jobs[j] = background_jobs[j + 1];
                    }
                    num_background_jobs--;
                    printf("hw1shell: pid %d finished\n", (curr_pid));
                    break;

                }
            }
        } 

        printf("hw1shell$ ");
        fflush(stdout);

        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("\n");
            break;
        }
    
        // Parse command
        num_args = 0;
        args[num_args] = strtok(command, " \n");
        args_size = sizeof(args) / sizeof(args[0]);
        while (args[num_args] != NULL && num_args < MAX_ARGS - 1) {
            num_args++;
            args[num_args] = strtok(NULL, " \n");
        }
        args[num_args] = NULL;

        // Execute command
        if (num_args > 0) {
            if (strcmp(args[0], "exit") == 0) {
                internal_exit();
            } else if (strcmp(args[0], "cd") == 0) {
                internal_cd(args, num_args);
            } else if (strcmp(args[0], "jobs") == 0) {
                list_jobs();
            }
            else {
                execute_command(args, num_args);
            }
        }  
    }

    return 0;
}

int add_job(int pid, char *command) {
    if (num_background_jobs < MAX_BACKGROUND_JOBS) {
        job_t job;
        job.pid = pid;
        job.command = strdup(command);
        background_jobs[num_background_jobs] = job;
        background_jobs[num_background_jobs].is_empty = 1;
        return 1;
    }
    else {
        printf("hw1shell: too many background commands running\n");
        return -1;
    }

}

void execute_command(char **args, int num_args) {
    if (num_args == 0) {
        // Empty command
        return;
    } else if (strcmp(args[0], "exit") == 0) {
        // Internal command: exit
        internal_exit();
    } else if (strcmp(args[0], "cd") == 0) {
        // Internal command: cd
        internal_cd(args, num_args);
    } else if (strcmp(args[0], "jobs") == 0) {
        // Internal command: jobs
        list_jobs();
    } else {
        // External command
        external_command(args, num_args);
    }
}

void list_jobs() {
    int i;
    for (i = 0; i < num_background_jobs; i++) {
        if (background_jobs[i].pid != 0) {
            printf("%d\t%s\n", background_jobs[i].pid, background_jobs[i].command);
        }
    }
}

void external_command(char **args, int num_args) {
    pid_t pid;
    int output_fd;
    int status1;
    pid = fork();
    if (num_args > 1) {
        if (strcmp(args[num_args-1], "&") == 0) {
            args[num_args-1] = '\0';
            num_args--;
            if (pid != 0) {
                // Add job to background jobs array
                if (add_job(pid, args[0]) < 0) {
                    kill(pid, SIGTERM); // kill child process
                    wait(NULL); // wait for child to exit
                }
                else {
                    printf("hw1shell: pid %d started\n", pid);
                    num_background_jobs++;
                    return;
                }
            }
        }
    }

    if (pid == 0) {
        // Child process
        // Check if output redirection is specified
        output_fd = STDOUT_FILENO;
        int i;
        for (i = 0; i < num_args - 1; i++) {
            if (strcmp(args[i], ">") == 0) {
                output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (output_fd < 0) {
                    printf("Error: cannot open output file %s\n", args[i + 1]);
                    exit(1);
                }
                // Remove the output redirection arguments from the argument list
                args[i] = NULL;
                args[i + 1] = NULL;
                break;
            }
        }
        // Execute the command
        if (execvp(args[0], args) < 0) {
            printf("Error: external command not found\n");
            printf("hw1shell: %s failed, errno is %d\n", "execvp", errno);
            exit(EXIT_FAILURE);
        }

        // Close the output file descriptor if it was opened
        if (output_fd != STDOUT_FILENO) {
            close(output_fd);
        }
        exit(0);
    } else if (pid < 0) {
        // Fork failed
        printf("hw1shell: %s failed, errno is %d\n", "fork", errno);
        exit(EXIT_FAILURE);
    } else {
        // Parent process, foreground command
        // Wait for the child process to finish
        do {
            if (waitpid(pid, &status1, WUNTRACED) < 0)
            {
                printf("hw1shell: %s failed, errno is %d\n", "waitpid", errno);
            }
        } while (!WIFEXITED(status1) && !WIFSIGNALED(status1));
    }
}

void internal_exit()
{
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].pid > 0) {
        kill(background_jobs[i].pid, SIGTERM);
    }
    for (int i=0; i<num_background_jobs; i++)
    {
        if (waitpid(background_jobs[i].pid, NULL, WNOHANG) < 0)
        {
            printf("hw1shell: %s failed, errno is %d\n", "waitpid", errno);
        }
    }
    exit(0);
    }
 }

void internal_cd(char **args, int num_args) {
    if (num_args == 1) {
        chdir(getenv("HOME"));
    } else if (num_args == 2) {
        chdir(args[1]);
    } else {
        printf("hw1shell: invalid command\n");
    }
}
