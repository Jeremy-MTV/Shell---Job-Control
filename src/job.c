#include "../head/jsh.h"

/**
 * Adds a job to the job list
 *
 * @param pid process id of the job
 * @param state state of the job
 * @param command command of the job
 * @return the job added
 */
job_t *add_job(pid_t pid, job_state state, char *command) {
  job_t *new_job = malloc(sizeof(job_t));
  if (!new_job) {
    fprintf(stderr, "jsh: allocation error\n");
    exit(EXIT_FAILURE);
  }
  njob++;
  new_job->age = idjob++;
  new_job->pid = pid;
  new_job->command = command;
  new_job->state = state;
  new_job->next = NULL;
  add_job_list(new_job);
  return new_job;
}

void add_job_list(job_t *job) {
  if (job_list == NULL) {
    job_list = job;
    return;
  }
  job_t *last_job = job_list;
  while (last_job->next != NULL) {
    last_job = last_job->next;
  }
  last_job->next = job;
}

void remove_job(pid_t pid) {
  job_t *current_job = job_list;
  job_t *previous_job = NULL;

  while (current_job != NULL) {
    if (current_job->pid == pid) {
      if (previous_job == NULL) {
        job_list = current_job->next;
      } else {
        previous_job->next = current_job->next;
      }
      if (current_job->age == idjob - 1)
        idjob--;
      free(current_job->command);
      free(current_job);
      break;
    }
    previous_job = current_job;
    current_job = current_job->next;
  }
  njob--;
}

void update_job(pid_t pid, job_state state) {
  job_t *current_job = job_list;

  while (current_job != NULL) {
    if (current_job->pid == pid) {
      current_job->state = state;
      return;
    }
    current_job = current_job->next;
  }
}


const char *job_state_strings[] = {"Running ", "Stopped ", "Done\t", "Killed ",
                                   "Detached "};


/**
 * Prints the details of a job, including the process tree
 * @param job job to print
 * @param fdout file descriptor to print to
 */
void print_job_details(job_t *job, int fdout) {
    dprintf(fdout, "[%d] %d %s%s\n", job->age, job->pid, job_state_strings[job->state],
            job->command);
}

/**
 * Prints the process tree for a given process ID recursively
 * @param pid process ID
 * @param fdout file descriptor to print to
 * @param indent level of indentation for tree structure
 */
void print_process_tree(pid_t pid, int fdout, int indent) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "/proc/%d/task/%d/status", pid, pid);

    FILE *statusFile = fopen(buffer, "r");
    if (statusFile == NULL) {
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), statusFile) != NULL) {
        if (strncmp(line, "PPid:", 5) == 0) {
            pid_t parentPid;
            sscanf(line + 5, "%d", &parentPid);

            for (int i = 0; i < indent; i++) {
                dprintf(fdout, "    ");
            }
            dprintf(fdout, "|-%d\n", pid);

            // Recursively print process tree for the parent process
            print_process_tree(parentPid, fdout, indent + 1);
            break;
        }
    }

    fclose(statusFile);
}


/**
 * Checks the status of all jobs
 * @param print if 1, print details about jobs whose status didn't changed
 * @param fdout file descriptor to print to
 */
void check_jobs(int print, int fdout) {
  job_t *current_job = job_list;
  job_t *previous_job = NULL;
  int status;

  while (current_job != NULL) {
    switch (
        waitpid(current_job->pid, &status, WNOHANG | WUNTRACED | WCONTINUED)) {
    case 0:
      if (print)
        print_job_details(current_job, fdout);
      goto next;
    case -1:
      if (errno == ECHILD)
        goto remove;
      if (errno == EINTR)
        continue;
      goto next;

    default: {
      if (WIFSIGNALED(status)) {
        current_job->state = KILLED;
        goto remove_print;
      }

      if (WIFEXITED(status)) {
        current_job->state = DONE;
        goto remove_print;
      }

      if (WIFSTOPPED(status)) {
        current_job->state = STOPPED;
        goto next_print;
      }

      if (WIFCONTINUED(status)) {
        current_job->state = RUNNING;
        goto next_print;
      }

      goto next;
    }
    }
  next_print:
    print_job_details(current_job, fdout);
  next:
    previous_job = current_job;
    current_job = current_job->next;
    continue;
  remove_print:
    print_job_details(current_job, fdout);
  remove:
    if (previous_job == NULL) {
      job_list = job_list->next;
      if (current_job->age == idjob - 1)
        idjob--;
      free(current_job->command);
      free(current_job);
      current_job = job_list;
      njob--;
      continue;
    }
    previous_job->next = current_job->next;
    if (current_job->age == idjob - 1)
      idjob--;
    free(current_job->command);
    free(current_job);
    current_job = previous_job->next;
    njob--;
  }
}

void free_job_list() {
  job_t *current_job = job_list;
  job_t *next_job;

  while (current_job != NULL) {
    next_job = current_job->next;
    free(current_job->command);
    free(current_job);
    current_job = next_job;
  }
}