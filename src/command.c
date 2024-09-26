#include "../head/jsh.h"

Command *create_command(char *name, Argument *arguments,
                        Redirection *redirection, int background) {
  Command *cmd = malloc(sizeof(Command));
  cmd->name = strdup(name); // Create a copy of the string
  cmd->arguments = arguments;
  cmd->redirection = redirection;
  cmd->substitutions = malloc(sizeof(Command *)*5);
  cmd->nb_substitutions = 0;
  cmd->size_substitutions = 5;
  cmd->background = background;
  cmd->next = NULL;
  cmd->pipe = NULL;
  return cmd;
}

Argument *create_argument(char *value) {
  Argument *arg = malloc(sizeof(Argument));
  arg->value = strdup(value); // Create a copy of the string
  arg->next = NULL;
  return arg;
}

Argument *add_argument(Command *command, char *value) {
  Argument *arg = create_argument(value);
  if (command->arguments == NULL) {
    command->arguments = arg;
  } else {
    Argument *currentArg = command->arguments;
    while (currentArg->next != NULL) {
      currentArg = currentArg->next;
    }
    currentArg->next = arg;
  }
  return arg;
}

Redirection *create_redirection(RedirectionType type, char *value) {
  Redirection *redir = malloc(sizeof(Redirection));
  redir->type = type;
  redir->value = strdup(value); // Create a copy of the string
  redir->next = NULL;
  return redir;
}

Redirection *add_redirection(Command *command, RedirectionType type,
                             char *value) {
  Redirection *redir = create_redirection(type, value);
  if (command->redirection == NULL) {
    command->redirection = redir;
  } else {
    Redirection *currentRedir = command->redirection;
    while (currentRedir->next != NULL) {
      currentRedir = currentRedir->next;
    }
    currentRedir->next = redir;
  }
  return redir;
}
/**
 * @brief check if the token is a redirection
 * @param token 
 * @return return redirection type or NULL
 */
RedirectionType *find_redirection_type(char *token) {
  for (size_t i = 0; i < REDIRECTIONS_SIZE; i++) {
    if (strcmp(token, redirections[i].symbol) == 0) {
      return &redirections[i].type;
    }
  }
  return NULL;
}

void clear_command(Command *command) {
  if (command == NULL)
    return;
  free(command->name);
  free(command->pipe);
  Argument *arg = command->arguments;
  while (arg != NULL) {
    Argument *nextArg = arg->next;
    free(arg->value);
    free(arg);
    arg = nextArg;
  }
  Redirection *redir = command->redirection;
  while (redir != NULL) {
    Redirection *nextRedir = redir->next;
    free(redir->value);
    free(redir);
    redir = nextRedir;
  }
  clear_command(command->next);
  for (size_t i = 0 ; i < command->nb_substitutions ; i++) {
    clear_command(command->substitutions[i]);
  }
  free(command->substitutions);
  free(command);
}