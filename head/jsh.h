#ifndef JSH_H
#define JSH_H

#define MAX_PROMPT_LENGTH 30
#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"
#define REDIRECT_ERROR 3
#define REDIRECTIONS_SIZE 11

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum {
  REDIRECT_OUT,
  REDIRECT_IN,
  PIPE_OUT,
  APPEND_OUT,
  REDIRECT_ERR,
  PIPE_ERR,
  APPEND_ERR,
  PIPE,
  BACKGROUND,
  SUBSTITUTION,
  SUBSTITUTION_OUT
} RedirectionType;

typedef struct {
  const char *symbol;
  RedirectionType type;
  int err;
  int mode;
  int create;
} RedirectionMap;

extern RedirectionMap redirections[REDIRECTIONS_SIZE];

typedef struct Argument {
  char *value;
  struct Argument *next;
} Argument;

typedef struct Redirection {
  RedirectionType type;
  char *value;
  struct Redirection *next;
} Redirection;

typedef struct Command {
  char *name;
  Argument *arguments;
  Redirection *redirection;
  struct Command **substitutions;
  size_t nb_substitutions;
  size_t size_substitutions;
  int background;
  int *pipe;
  struct Command *next;
} Command;

typedef enum { RUNNING, STOPPED, DONE, KILLED, DETACHED } job_state;

typedef struct job {
  int age;          // job number
  pid_t pid;        // process ID
  job_state state;  // job state
  char *command;    // command line
  struct job *next; // next job in the list
} job_t;

extern int last_exit_code;
extern int run;
extern job_t *job_list;
extern int njob;
extern int idjob;

// main.c
void signals(int mode);

// builtin.c
void cd(char **args);
void jexit(char **args);
void pwd(void);
void question_mark(void);
void jobs(char **args);
void fg(char **args);
void bg(char **args);
void send_signal(int sig, char *target);
void kill_job(char **args);
void check_state(pid_t pid, int sig);
int is_Number(const char *str);

// prompt.c
void current_folder(char *prompt);
void build_prompt(char *prompt);

// parser.c
Command *parse_command(char *line , int substituting);
char **get_full_command(Command *cmd);
char *get_command(Command *cmd);
char *get_command2(char **args);

// execute.c
void execute_external_command(char **args);
void execute_command(char **args, int forking);
int handle_command_redirections(Command *cmd, int forking);
int handle_piped_commands(Command *commands, int forking);
int handle_substitution_commands(Command *commands, int forking, char *path);
void execution(Command *commands , int substituting);

// job.c
job_t *add_job(pid_t pid, job_state state, char *command);
void add_job_list(job_t *job);
void remove_job(pid_t pid);
void update_job(pid_t pid, job_state state);
void print_job_details(job_t *job, int fdout);
void check_jobs(int print, int fdout);
void free_job_list(void);
void print_process_tree(pid_t pid, int fdout, int indent);

// command.c
Command *create_command(char *name, Argument *arguments,
                        Redirection *redirection, int background);
Argument *create_argument(char *value);
Argument *add_argument(Command *command, char *value);
Redirection *create_redirection(RedirectionType type, char *value);
Redirection *add_redirection(Command *command, RedirectionType type,
                             char *value);
RedirectionType *find_redirection_type(char *token);
void clear_command(Command *command);

// redirections.c
void create_pipe(void);
int apply_redirections(Redirection *redirection);
int redirect_output(char *filename, int err, int mode, int create);
int save_redirections(int *SAVE_STDOUT, int *SAVE_STDIN, int *SAVE_STDERR);
int reset_redirections(int SAVE_STDOUT, int SAVE_STDIN, int SAVE_STDERR);
int redirect_input(char *filename);

#endif
