#include "../head/jsh.h"

typedef void (*CommandFunc)();

typedef struct {
  char *name;
  CommandFunc func;
} ExecutableCommand;

/**
 * Executes a command with arguments if not built-in
 *
 * @param args command arguments
 */
void execute_external_command(char **args) {
  int status;
  pid_t pid = fork();
  if (pid == 0) {
    // Processus enfant
    if (setpgid(getpid(), getpid())) {
      perror("jsh: setgid error");
      exit(EXIT_FAILURE);
    }
    tcsetpgrp(STDIN_FILENO, getpid());
    tcsetpgrp(STDOUT_FILENO, getpid());
    tcsetpgrp(STDERR_FILENO, getpid());
    signals(1);
    if (execvp(args[0], args) == -1) {
      perror("jsh: execution error");
      exit(EXIT_FAILURE);
    }
  }
  if (pid < 0) {
    // Erreur de fork
    perror("jsh: fork error");
    last_exit_code = EXIT_FAILURE;
    return;
  } else {
    // Processus parent
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));
    tcsetpgrp(STDIN_FILENO, getpid());
    tcsetpgrp(STDOUT_FILENO, getpid());
    tcsetpgrp(STDERR_FILENO, getpid());

    if (WIFSTOPPED(status)) {
      // Mise à jour de la liste des jobs
      char *cmd = get_command2(args);
      if (cmd == NULL) {
        last_exit_code = EXIT_FAILURE;
        return;
      }
      job_t *new_job = add_job(pid, STOPPED, cmd);
      print_job_details(new_job, STDERR_FILENO);
      last_exit_code = 128 + WSTOPSIG(status);
      return;
    }

    if (WIFSIGNALED(status))
      last_exit_code = 128 + WTERMSIG(status);
    else
      last_exit_code = WEXITSTATUS(status);
  }
}

/**
 * Executes a built-in command with arguments
 *
 * @param args : command arguments
 * @param forking : `1` if we need to fork in order to execute a non_builtin
 * command, `0` otherwise
 */
void execute_command(char **args, int forking) {
  ExecutableCommand commands[] = {
      {"exit", jexit},      {"cd", cd},     {"pwd", pwd},
      {"?", question_mark}, {"jobs", jobs}, {"kill", kill_job},
      {"fg", fg},           {"bg", bg},     {NULL, NULL} // end marker
  };

  for (int i = 0; commands[i].name != NULL; i++) {
    if (strcmp(args[0], commands[i].name) == 0) {
      commands[i].func(args);
      return;
    }
  }
  if (forking)
    execute_external_command(args);
  else {
    if (execvp(args[0], args) == -1) {
      perror("jsh: execution error");
      exit(EXIT_FAILURE);
    }
  }
}

/**
 * Perpares the execution of a command by applying redirections and handling
 * pipes
 *
 * @param cmd : command to execute
 * @param forking : `1` if we need to fork in order to execute the command, `0`
 * otherwise
 * @return `1` if an error occured, `0` otherwise and `last_exit_code` is set
 * appropriately
 */
int handle_command_redirections(Command *cmd, int forking) {
  // we are in the case `cmd | cmd1 ...`
  if (cmd->pipe != NULL) {
    pid_t pid = fork();

    // child process executes the command `cmd`
    if (!pid) {
      signals(1);
      // pipe settings
      close(cmd->pipe[0]);
      if (dup2(cmd->pipe[1], STDOUT_FILENO) == -1) {
        perror("jsh: dup2 error");
        exit(EXIT_FAILURE);
      }
      close(cmd->pipe[1]);
      // redirections settings
      if (apply_redirections(cmd->redirection)) {
        close(STDOUT_FILENO);
        exit(REDIRECT_ERROR);
      }
      // command execution
      char **args = get_full_command(cmd);
      if (args == NULL) {
        exit(EXIT_FAILURE);
      }
      execute_command(args, forking);
      exit(last_exit_code);
    }

    // fork error
    if (pid == -1) {
      perror("jsh: fork error");
      last_exit_code = EXIT_FAILURE;
      close(cmd->pipe[0]);
      close(cmd->pipe[1]);
      return 1;
    }

    // parent process
    close(cmd->pipe[1]);
    if (dup2(cmd->pipe[0], STDIN_FILENO) == -1) {
      perror("jsh: dup2 error");
      close(cmd->pipe[0]);
      last_exit_code = EXIT_FAILURE;
      return 1;
    }
    int status;
    waitpid(pid, &status, WUNTRACED | WNOHANG);
    if (WIFEXITED(status) && WEXITSTATUS(status) == REDIRECT_ERROR) {
      close(STDIN_FILENO);
      close(cmd->pipe[0]);
      last_exit_code = EXIT_FAILURE;
      return 1;
    }
    close(cmd->pipe[0]);
    return 0;
  }

  // we are in the case `cmd`
  char **args = get_full_command(cmd);
  if (args == NULL) {
    last_exit_code = EXIT_FAILURE;
    return 1;
  }
  // redirections settings
  if (apply_redirections(cmd->redirection)) {
    close(STDIN_FILENO);
    last_exit_code = EXIT_FAILURE;
    return 1;
  }
  // command execution
  execute_command(args, forking);
  free(args);
  return 0;
}

/**
 * Handles the execution of a command as `cmd` or `cmd1 | ... | cmdn`
 *
 * @param commands : list of commands
 * @param forking : `1` if we need to fork in order to execute the command, `0`
 * otherwise
 * @return `1` if an error occured, `0` otherwise and `last_exit_code` is set
 * appropriately
 */
int handle_piped_commands(Command *commands, int forking) {
  Command *i = NULL;
  for (i = commands; i->next != NULL; i = i->next) {
    if (pipe(i->pipe) == -1) {
      perror("jsh: pipe error");
      return 1;
    }
    if (handle_command_redirections(i, 0))
      return 1;
  }
  if (handle_command_redirections(i, forking))
    return 1;
  return 0;
}

/**
 * Executes a list of commands
 *
 * @param commands : list of commands
 */
void execution(Command *commands , int substituting) {
  // we save the beginning of the command we want to execute each time in
  // `start`
  Command *i = commands;
  Command *start = commands;
  int status ; 
  int has_pipe = 0 ; 

  while (i != NULL) {
    // we are in the case : `cmdi & ...` OR `cmd1 | ... | cmdi &`
    if (i->background) {
      pid_t pid = fork();

      // child process executes the command `start`
      if (!pid) {
        i->next = NULL;
        if (setpgid(getpid(), getpid())) {
          perror("jsh: setgid error");
          exit(EXIT_FAILURE);
        }
        signals(1);
        for (Command * j = start ; j != NULL ; j = j->next) {
          for (size_t k = 0 ; k < j->nb_substitutions ; k++) {
            execution(j->substitutions[k] , 1);
          }
        }
        if (handle_piped_commands(start, 0))
          exit(last_exit_code);
        exit(last_exit_code);
      }

      // fork error
      if (pid == -1) {
        perror("jsh: error fork");
        last_exit_code = 1;
        return;
      }

      // parent process
      Command *tmp = i->next;
      // end marker for `get_command()`
      i->next = NULL;
      char *cmd = get_command(start);
      i->next = tmp;
      // malloc error
      if (cmd == NULL) {
        last_exit_code = EXIT_FAILURE;
        return;
      }
      job_t *new_job = add_job(pid, RUNNING, cmd);
      print_job_details(new_job, STDERR_FILENO);
      waitpid(pid, RUNNING, WNOHANG);
      // we update `i` and `start` by assigning them the next command to execute
      i = tmp;
      start = i;
      continue;
    } else { // we are in the case : `cmdi | cmdj ... ` OR `... cmdi`
      // we are in the case : `... cmdi` then we have to execute `cmdi` then
      // exit the loop
      if (i->pipe == NULL) {
        for (Command * j = start ; j != NULL ; j = j->next) {
          for (size_t k = 0 ; k < j->nb_substitutions ; k++) {
            execution(j->substitutions[k] , 1);
          }
        }
        if (!has_pipe || has_pipe && substituting) {
          int SAVE_STDOUT, SAVE_STDIN, SAVE_STDERR;
          if (save_redirections(&SAVE_STDOUT, &SAVE_STDIN, &SAVE_STDERR))
            goto error_redirection;
          handle_piped_commands(start, 1);
          if (reset_redirections(SAVE_STDOUT, SAVE_STDIN, SAVE_STDERR))
            goto error_redirection;
          return;
        } else {
          pid_t pid = fork();
          switch(pid) {
            case 0: 
              setpgid(getpid(), getpid());
              tcsetpgrp(STDIN_FILENO, getpid());
              tcsetpgrp(STDOUT_FILENO, getpid());
              tcsetpgrp(STDERR_FILENO, getpid());
              signals(1);
              handle_piped_commands(start, 0);
              exit(last_exit_code);
            case -1:
              perror("jsh: error fork");
              last_exit_code = 1;
              return;

            default:
              do {
                waitpid(pid, &status, WUNTRACED);
              }while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));
              tcsetpgrp(STDIN_FILENO, getpid());
              tcsetpgrp(STDOUT_FILENO, getpid());
              tcsetpgrp(STDERR_FILENO, getpid());
              if (WIFSTOPPED(status)) {
                // Update the job state
                job_t *new_job = add_job(pid, STOPPED, get_command(start));
                print_job_details(new_job, STDERR_FILENO);
                last_exit_code = 128 + WSTOPSIG(status);
                return;
              }
              if (WIFSIGNALED(status))
                last_exit_code = 128 + WTERMSIG(status);
              else
                last_exit_code = WEXITSTATUS(status);
          }
        }
      }else has_pipe = 1 ;
      // otherwise we are in the case : `cmdi | cmdj ...` then we continue the
      // loop until we each the last command
    }
    i = i->next;
  }
  return;

error_redirection:
  free_job_list();
  char *err[1] = {"3"};
  jexit(err);
}