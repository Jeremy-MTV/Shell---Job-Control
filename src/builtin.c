#include "../head/jsh.h"

// Fonction pour changer le répertoire de travail

void cd(char **args) {

  char buffer[PATH_MAX];

  // Vérifie le nombre d'arguments
  if (args[1] != NULL && args[2] != NULL) {
    usleep(1000);
    fprintf(stderr, "cd : trop d'arguments\n");
    last_exit_code = EXIT_FAILURE;
    return;
  }

  // Variables pour le nouveau répertoire et le chemin résolu
  char *new_directory;
  char resolved_path[PATH_MAX];

  // Cas où aucun argument n'est fourni
  if (args[1] == NULL) {
    char *home_directory = getenv("HOME");
    if (home_directory == NULL) {
      fprintf(stderr,
              "cd : la variable d'environnement $HOME n'est pas définie\n");
      last_exit_code = EXIT_FAILURE;
      return;
    }
    new_directory = home_directory;
  }
  // Cas où l'argument est "-"
  else if (strcmp(args[1], "-") == 0) {
    char *old_directory = getenv("OLDPWD");
    if (old_directory == NULL) {
      fprintf(stderr,
              "cd : la variable d'environnement $OLDPWD n'est pas définie\n");
      last_exit_code = EXIT_FAILURE;
      return;
    }
    new_directory = old_directory;
  }
  // Cas où un chemin est spécifié
  else {
    // Résout le chemin absolu
    if (realpath(args[1], resolved_path) == NULL) {
      usleep(1000);
      perror("cd");
      last_exit_code = EXIT_FAILURE;
      return;
    }

    new_directory = resolved_path;

    // Vérifie si le chemin existe
    if (access(new_directory, F_OK) != 0) {
      usleep(1000);
      fprintf(stderr, "cd : chemin non existant\n ");
      last_exit_code = EXIT_FAILURE;
      return;
    }
  }

  // Enregistre l'ancien répertoire de travail
  char *old_directory = getcwd(buffer, sizeof(buffer));

  // Change le répertoire de travail
  if (chdir(new_directory) != 0) {
    usleep(1000);
    perror("cd");
    last_exit_code = EXIT_FAILURE;

  } else {
    // Met à jour les variables d'environnement $PWD et $OLDPWD
    setenv("OLDPWD", old_directory, 1);
    char *actual_directory = getcwd(buffer, sizeof(buffer));
    setenv("PWD", actual_directory, 1);
    last_exit_code = EXIT_SUCCESS;
  }
}

void jexit(char **args) {
  if (job_list != NULL && run != 2) {
    // If there are jobs in progress, display a warning message
    fprintf(
        stderr,
        "jsh: There are jobs in progress. Use 'exit' again to terminate.\n");
    last_exit_code = EXIT_FAILURE;
    run = 2;
  } else {
    // No jobs in progress, proceed with exit
    if (args[1] != NULL) {
      last_exit_code = atoi(args[1]);
    }
    run = 0;
  }
}

void pwd() {
  char *cwd = malloc(sizeof(char) * 256);
  if (!cwd) {
    fprintf(stderr, "jsh: allocation error\n");
    last_exit_code = EXIT_FAILURE;
  }
  if (getcwd(cwd, sizeof(char) * 256) != NULL) {
    fprintf(stdout, "%s\n", cwd);
  } else {
    perror("pwd failed");
    last_exit_code = EXIT_FAILURE;
  }
  free(cwd);
  last_exit_code = EXIT_SUCCESS;
}

void question_mark() {
  printf("%d\n", last_exit_code);
  last_exit_code = EXIT_SUCCESS;
}

void jobs(char **args) {
  // If no arguments are provided, list all jobs
  if (args[1] == NULL) {
    check_jobs(1, STDOUT_FILENO);
    last_exit_code = EXIT_SUCCESS;
    return;
  }

  // If the -t option is provided
  if (strcmp(args[1], "-t") == 0) {
    for (job_t *job = job_list; job != NULL; job = job->next) {
      print_job_details(job, STDOUT_FILENO);
      print_process_tree(job->pid, STDOUT_FILENO, 1);
    }
    last_exit_code = EXIT_SUCCESS;
    return;
  }
  // If there are too many arguments
  fprintf(stderr, "jobs: too many arguments\n");
  last_exit_code = EXIT_FAILURE;
  return;
}


void fg(char **args) {
  // If no arguments are provided
  errno = 0;
  int age;
  if (args[1] == NULL || args[2] != NULL ||
      (age = is_Number((*args[1] == '%') ? args[1] + 1 : args[1])) <= 0 ||
      errno != 0 || age >= idjob) {
    fprintf(stderr, "fg: invalid arguments\n");
    last_exit_code = EXIT_FAILURE;
    return;
  }

  // Find the job with the given job number
  job_t *job;
  for (job = job_list; job->age != age; job = job->next)
    ;

  // Send the SIGCONT signal to the job
  tcsetpgrp(STDIN_FILENO, job->pid);
  tcsetpgrp(STDOUT_FILENO, job->pid);
  tcsetpgrp(STDERR_FILENO, job->pid);
  killpg(job->pid, SIGCONT);

  // Wait for the job to finish
  int status;
  do {
    waitpid(job->pid, &status, WUNTRACED);
  } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

  tcsetpgrp(STDIN_FILENO, getpid());
  tcsetpgrp(STDOUT_FILENO, getpid());
  tcsetpgrp(STDERR_FILENO, getpid());
  if (WIFSTOPPED(status)) {
    // Update the job state
    job->state = STOPPED;
    print_job_details(job, STDERR_FILENO);
    last_exit_code = 128 + WSTOPSIG(status);
    return;
  }

  if (WIFSIGNALED(status))
    last_exit_code = 128 + WTERMSIG(status);
  else
    last_exit_code = WEXITSTATUS(status);

  // Update the job list
  remove_job(job->pid);
}

void bg(char **args) {
  errno = 0;
  int age;
  if (args[1] == NULL || args[2] != NULL ||
      (age = is_Number((*args[1] == '%') ? args[1] + 1 : args[1])) <= 0 ||
      errno != 0 || age >= idjob) {
    fprintf(stderr, "bg: invalid arguments\n");
    last_exit_code = EXIT_FAILURE;
    return;
  }
  // Find the job with the given job number
  job_t *job;
  for (job = job_list; job->age != age; job = job->next)
    ;

  // Send the SIGCONT signal to the job
  kill(job->pid, SIGCONT);

  // Update the job state
  job->state = RUNNING;
  last_exit_code = EXIT_SUCCESS;
}

/**
 * Sends a signal to a process or a job
 * @param sig : signal number
 * @param target : process ID or job number
 */
void send_signal(int sig, char *target) {
  pid_t pid;
  // case of job id
  if (*target == '%') {
    int age = is_Number(target + 1);
    if (age <= 0 || age >= idjob) {
      fprintf(stderr, "kill: %s : no such job\n", target);
      goto exit;
    }
    // find the job with the given job number
    job_t *job_target;
    for (job_target = job_list; job_target->age != age;
         job_target = job_target->next)
      ;
    pid = job_target->pid;
  } else {
    // case of pid
    errno = 0;
    pid = is_Number(target);
    if (errno != 0) {
      fprintf(
          stderr,
          "kill: %s : argument must be an identifier of task or processus\n",
          target);
      goto exit;
    }
  }
  if (!killpg(pid, sig)) {
    check_state(pid, sig);
    last_exit_code = EXIT_SUCCESS;
    return;
  }
  switch (errno) {
  case EPERM:
    fprintf(stderr, "kill: (%s) - operation not permitted\n", target);
    goto exit;
  case ESRCH:
    fprintf(stderr, "kill: (%s) - no such process\n", target);
    goto exit;
  default:
    fprintf(stderr, "kill: an error occured\n");
    goto exit;
  }
exit:
  last_exit_code = EXIT_FAILURE;
}

/**
 * Checks the state of a process
 * @param pid : process ID
 * @param sig : signal number sent
 */
void check_state(pid_t pid, int sig) {
  int state;
  waitpid(pid, &state, WNOHANG | WUNTRACED | WCONTINUED);
  if (WIFSTOPPED(state) && sig != SIGSTOP && sig != SIGTTIN && sig != SIGTTOU &&
      sig != SIGTSTP)
    kill(pid, SIGCONT);
}

/**
 * Kills a job
 * @param args : arguments of the command
 */
void kill_job(char **args) {
  if (args[1] == NULL || args[2] != NULL && args[3] != NULL)
    goto error_args;
  int sig = 15;
  // check signal
  if (*args[1] == '-') {
    if (args[2] == NULL)
      goto error_args;
    sig = is_Number(args[1] + 1);
    if (sig < 0 || sig > 64) {
      fprintf(stderr, "kill: %s : invalid signal specification\n", args[1] + 1);
      goto exit;
    }
    send_signal(sig, args[2]);
    return;
  }
  if (args[2] != NULL)
    goto error_args;
  send_signal(sig, args[1]);
  return;
error_args:
  fprintf(stderr, "kill: incorrect arguments , try 'kill [-sig] pid' or 'kill "
                  "[-sig] %%job'\n");
exit:
  last_exit_code = EXIT_FAILURE;
}

/**
 * Checks if a string is a number
 * @param str : string to check
 * @return the value of the number if the string is a number , `-1` otherwise
 * and errno is set to `EINVAL`
 */
int is_Number(const char *str) {
  size_t start = 0;
  if (*str == '-')
    start = 1;
  for (size_t i = start; i < strlen(str); i++) {
    if (!isdigit(str[i])) {
      errno = EINVAL;
      return -1;
    }
  }
  return atoi(str);
}