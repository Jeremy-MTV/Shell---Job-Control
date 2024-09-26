#include "../head/jsh.h"

RedirectionMap redirections[] = {
    {">", REDIRECT_OUT, 0, O_WRONLY, O_CREAT | O_EXCL},
    {"<", REDIRECT_IN, 0, O_RDONLY, 0},
    {">|", PIPE_OUT, 0, O_WRONLY | O_TRUNC, O_CREAT},
    {">>", APPEND_OUT, 0, O_WRONLY | O_APPEND, O_CREAT},
    {"2>", REDIRECT_ERR, 1, O_WRONLY, O_CREAT | O_EXCL},
    {"2>|", PIPE_ERR, 1, O_WRONLY | O_TRUNC, O_CREAT},
    {"2>>", APPEND_ERR, 1, O_WRONLY | O_APPEND, O_CREAT},
    {"|", PIPE, 0, 0, 0},
    {"&", BACKGROUND, 0, 0, 0},
    {"<(", SUBSTITUTION, 0, 0, 0},
    {")", SUBSTITUTION_OUT, 0, 0, 0},
};

int redirect_input(char *filename) {
  int fd = open(filename, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "jsh: open error (%s): %s\n", filename, strerror(errno));
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) == -1) {
    perror("jsh: dup2 error");
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}


/**
 * @brief Redirects stdout or stderr to a file
 * @param filename The file to redirect to
 * @param err `1` to redirect in `stderr`, `0` for `stdout`
 * @param mode `O_WRONLY` or `O_RDWR` or `O_APPEND` or `O_TRUNC`
 * @param create `O_CREAT` or `O_EXCL`
 * @return 1 on failure, 0 on success
 */
int redirect_output(char *filename, int err, int mode, int create) {
  int fd = open(filename, mode | create,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  if (fd == -1) {
    fprintf(stderr, "jsh: open error (%s): %s\n", filename, strerror(errno));
    return 1;
  }

  if (dup2(fd, err ? STDERR_FILENO : STDOUT_FILENO) == -1) {
    perror("jsh: dup2 error");
    close(fd);
    return 1;
  }

  close(fd);
  return 0;
}

/**
 * @brief Saves the current stdout, stdin and stderr
 * @param SAVE_STDOUT 
 * @param SAVE_STDIN 
 * @param SAVE_STDERR 
 * @return 1 on failure, 0 on success
 */
int save_redirections(int *SAVE_STDOUT, int *SAVE_STDIN, int *SAVE_STDERR) {
  *SAVE_STDOUT = dup(STDOUT_FILENO);
  *SAVE_STDIN = dup(STDIN_FILENO);
  *SAVE_STDERR = dup(STDERR_FILENO);
  if (*SAVE_STDOUT == -1 || *SAVE_STDIN == -1 || *SAVE_STDERR == -1) {
    perror("jsh: error dup");
    close(*SAVE_STDIN);
    close(*SAVE_STDOUT);
    close(*SAVE_STDERR);
    return 1;
  }
  return 0;
}

int apply_redirections(Redirection *redirection) {
  int res = 0;
  int fd ; 
  while (redirection != NULL) {
    for (size_t i = 0; i < sizeof(redirections) / sizeof(RedirectionMap); i++) {
      if (redirection->type == redirections[i].type) {
        switch (redirections[i].type) {
        case REDIRECT_OUT:
        case PIPE_OUT:
        case APPEND_OUT:
        case REDIRECT_ERR:
        case PIPE_ERR:
        case APPEND_ERR:
          res = redirect_output(redirection->value, redirections[i].err,
                                redirections[i].mode, redirections[i].create);
          break;
        case REDIRECT_IN:
          res = redirect_input(redirection->value);
          break;
        case SUBSTITUTION_OUT:
          fd = atoi(redirection->value);
          if (dup2(fd , STDOUT_FILENO) == -1) {
            perror("jsh: dup2 error");
            close(fd);
            res = 1;
          }  
          close(fd);
          break;
        default:
          fprintf(stderr, "Unknown redirection type\n");
          return 1; // Return 1 on failure
        }
        break;
      }
    }
    if (res != 0)
      return res;

    redirection = redirection->next;
  }
  return 0; // Return 0 on success
}

int reset_redirections(int SAVE_STDOUT, int SAVE_STDIN, int SAVE_STDERR) {
  int res = 0;
  if (dup2(SAVE_STDOUT, STDOUT_FILENO) == -1 ||
      dup2(SAVE_STDIN, STDIN_FILENO) == -1 ||
      dup2(SAVE_STDERR, STDERR_FILENO) == -1) {
    perror("jsh: error dup2");
    res = 1;
  }
  close(SAVE_STDIN);
  close(SAVE_STDOUT);
  close(SAVE_STDERR);
  return res;
}