#include "../head/jsh.h"

void current_folder(char *prompt) {
  char cwd[256];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("cwd");
  }

  if (strlen(cwd) > MAX_PROMPT_LENGTH - 2) {
    memcpy(prompt, "...", 3);
    prompt[3] = '\0';
    strncpy(prompt + 3, cwd + strlen(cwd) - (MAX_PROMPT_LENGTH - 8),
            MAX_PROMPT_LENGTH - 4);
  } else {
    strcpy(prompt, cwd);
  }
}

void build_prompt(char *prompt) {
  if (last_exit_code == 0) {
    strcpy(prompt, "\001\033[32m\002");
  } else {
    strcpy(prompt, "\001\033[31m\002");
  }
  char jobs_str[10];
  snprintf(jobs_str, sizeof(jobs_str), "[%d]", njob);
  strcat(prompt, jobs_str);
  strcat(prompt, "\001\033[33m\002");
  char temp[MAX_PROMPT_LENGTH];
  current_folder(temp);
  strcat(prompt, temp);
  strcat(prompt, "\001\033[00m\002");
  strcat(prompt, "$ ");
}