#include "../head/jsh.h"

char main_prompt[MAX_PROMPT_LENGTH];
int last_exit_code = EXIT_SUCCESS;
int run = 1;
int njob = 0;
int idjob = 1;
job_t *job_list = NULL;

/**
 * Ignores or resets a set of signals
 *
 * @param mode : `0` to ignore the signals, `1` to reset them
 */
void signals(int mode) {
  int signals[] = {SIGINT, SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU, SIGTSTP};

  struct sigaction sa;

  // Set up the signal handler to ignore the signals
  sa.sa_handler = mode ? SIG_DFL : SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  // Set the signal handler for each signal in the array
  for (size_t i = 0; i < sizeof(signals) / sizeof(int); i++) {
    sigaction(signals[i], &sa, NULL);
  }
}

int main() {
  char *input;
  Command *commands;

  signals(0);

  while (run) {
    build_prompt(main_prompt);
    rl_outstream = stderr;
    input = readline(main_prompt);
    add_history(input);

    if (input == NULL)
      break;
    if (input[0] == '\0')
      goto clear;

    errno = 0;
    commands = parse_command(input , 0);
    if (errno != 0)
      goto clear_command;
    execution(commands , 0);

  clear_command:
    clear_command(commands);
  clear:
    free(input);
    check_jobs(0, STDERR_FILENO);
  }
  free_job_list();
  exit(last_exit_code);
}