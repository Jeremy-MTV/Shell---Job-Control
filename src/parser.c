#include "../head/jsh.h"

Command *parse_command(char *input , int substuting) {
  char *token = strtok(input, DELIMITERS);
  if (substuting && !token) {
    perror("jsh: error: Syntax error around << <( >>\n");
    errno = 2 ;
    return NULL; 
  }
  if (find_redirection_type(token) != NULL) {
    if (!strcmp(token , ")")) return NULL;
    perror("jsh: error: Syntax error around << <( >>\n");
    errno = 2;
    return NULL;
  }
  Command *command = create_command(token, NULL, NULL, 0);

  Command *currentCommand = command;
  while ((token = strtok(NULL, " ")) != NULL) {
    RedirectionType *ptype = find_redirection_type(token);
    if (ptype != NULL) {

      if (*ptype == SUBSTITUTION_OUT){
        if (substuting) return command;
        fprintf(stderr , "jsh: error: Syntax error around << ) >>\n");
        errno = 2;
        return command;
      }

      if (*ptype == SUBSTITUTION) {
        Command * to_substitute = parse_command(NULL , 1);
        if (!to_substitute) {
          if (errno != 0) {
            clear_command(to_substitute);
            return command ; 
          }
          continue;
        }
        int fds[2] = {-1 , -1}; 
        if (pipe(fds)) {
          fprintf(stderr , "jsh: error: Pipe creation failed\n");
          errno = 2 ;
          clear_command(to_substitute);
          return command ; 
        }
        if (currentCommand->nb_substitutions + 1 >= currentCommand->size_substitutions) {
          currentCommand->size_substitutions += 10;
          currentCommand->substitutions = realloc(currentCommand->substitutions , sizeof(Command *)*currentCommand->size_substitutions);
        }
        currentCommand->substitutions[currentCommand->nb_substitutions++] = to_substitute;
        char arg_val[20];
        sprintf(arg_val , "/dev/fd/%d" , fds[0]) ; 
        add_argument(currentCommand , arg_val);
        Command *last_cmd ; 
        for (last_cmd = to_substitute ; last_cmd->next != NULL ; last_cmd = last_cmd->next) ; 
        char redir_val[20] = {0};
        sprintf(redir_val , "%d" , fds[1]);
        add_redirection(last_cmd , SUBSTITUTION_OUT , redir_val);
        continue;
      } else if (*ptype == PIPE) {
        char *next = strtok(NULL, " ");
        if (next == NULL) {
          fprintf(stderr , "jsh: error: Syntax error around << | >>\n");
          errno = 2;
          return command;
        }
        Command *nextCommand = create_command(next, NULL, NULL, 0);
        currentCommand->pipe = malloc(2 * sizeof(int));
        currentCommand->next = nextCommand;
        currentCommand = nextCommand;
      } else if (*ptype == BACKGROUND) {
        if (currentCommand->pipe != NULL) {
          fprintf(stderr , "jsh: error: Syntax error around << & >>\n");
          errno = 2;
          return command;
        }
        currentCommand->background = 1;
        token = strtok(NULL, " ");
        if (token != NULL) { // If there's another command after the &
          Command *nextCommand = create_command(token, NULL, NULL, 0);
          currentCommand->next = nextCommand;
          currentCommand = nextCommand;
        }
      } else {
        char *value = strtok(NULL, " ");
        if (value == NULL) {
          fprintf(stderr , "jsh: error: Syntax error newLine expected\n");
          errno = 2;
          return command;
        }
        RedirectionType *next = find_redirection_type(value);
        if (next != NULL) {
          if (*next != SUBSTITUTION) {
            fprintf(stderr ,"jsh: error: Syntax error newLine expected\n");
            errno = 2;
            return command;
          }
          // here 
          Command * to_substitute = parse_command(NULL , 1);
          if (!to_substitute) {
            if (errno != 0) {
              clear_command(to_substitute);
              return command ; 
            }
            continue;
          }
          int fds[2] = {-1 , -1}; 
          if (pipe(fds)) {
            fprintf(stderr , "jsh: error: Pipe creation failed\n");
            errno = 2 ;
            clear_command(to_substitute);
            return command ; 
          }
          if (currentCommand->nb_substitutions + 1 >= currentCommand->size_substitutions) {
            currentCommand->size_substitutions += 10;
            currentCommand->substitutions = realloc(currentCommand->substitutions , sizeof(Command *)*currentCommand->size_substitutions);
          }
          currentCommand->substitutions[currentCommand->nb_substitutions++] = to_substitute;
          char to_redir_val[20];
          sprintf(to_redir_val , "/dev/fd/%d" , fds[0]) ; 
          add_redirection(currentCommand , *ptype , to_redir_val);
          Command *last_cmd ; 
          for (last_cmd = to_substitute ; last_cmd->next != NULL ; last_cmd = last_cmd->next) ; 
          char redir_val[20] = {0};
          sprintf(redir_val , "%d" , fds[1]);
          add_redirection(last_cmd , SUBSTITUTION_OUT , redir_val);

          continue;
        }
        add_redirection(currentCommand, *ptype, value);
      }
    } else {
      add_argument(currentCommand, token);
    }
  }
  return command;
}

char **get_full_command(Command *cmd) {
  size_t bufsize = MAX_INPUT;
  size_t position = 0;
  char **command = malloc(bufsize * sizeof(char *));

  if (!command)
    goto error_alloc;

  command[position++] = cmd->name;

  for (Argument *i = cmd->arguments; i != NULL; i = i->next) {
    if (position >= bufsize) {
      bufsize += MAX_INPUT;
      command = realloc(command, bufsize * sizeof(char *));
      if (!command)
        goto error_alloc;
    }
    command[position++] = i->value;
  }
  command[position] = NULL;
  return command;
error_alloc:
  fprintf(stderr, "jsh: allocation error\n");
  return NULL;
}

char *get_command(Command *cmd) {
  size_t bufsize = MAX_INPUT;
  size_t position = 0;
  size_t len;
  char *command = malloc(bufsize * sizeof(char));

  if (!command)
    goto error_alloc;
  for (Command *k = cmd; k != NULL; k = k->next) {
    len = strlen(k->name);
    memmove(command + position, k->name, len);
    position += strlen(k->name);
    command[position++] = ' ';

    for (Argument *i = k->arguments; i != NULL; i = i->next) {
      len = strlen(i->value);
      if (position + len >= bufsize) {
        bufsize += MAX_INPUT;
        command = realloc(command, bufsize * sizeof(char));
        if (!command)
          goto error_alloc;
      }
      memmove(command + position, i->value, len);
      position += len;
      command[position++] = ' ';
    }
    if (k->pipe != NULL) {
      command[position++] = '|';
      command[position++] = ' ';
    }
  }
  command[position - 1] = '\0';
  return command;
error_alloc:
  fprintf(stderr, "jsh: allocation error\n");
  return NULL;
}

char *get_command2(char **args) {
  size_t bufsize = MAX_INPUT;
  size_t position = 0;
  size_t len;
  char *command = malloc(bufsize * sizeof(char));
  if (!command)
    goto error_alloc;

  for (size_t i = 0; args[i] != NULL; i++) {
    len = strlen(args[i]);
    if (position + len >= bufsize) {
      bufsize += MAX_INPUT;
      command = realloc(command, bufsize * sizeof(char));
      if (!command)
        goto error_alloc;
    }
    memmove(command + position, args[i], len);
    position += len;
    command[position++] = ' ';
  }
  command[position - 1] = '\0';
  return command;
error_alloc:
  fprintf(stderr, "jsh: allocation error\n");
  return NULL;
}
