#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

/* Linked list of processes */
typedef struct process {
  pid_t pid;
  struct termios tmodes; // terminal modes for the process
  bool running; // true = running, false = suspended
  struct process *next;
} process_t;

process_t *process_list = NULL;

void add_process(pid_t pid) {
  process_t *new_proc = malloc(sizeof(process_t));
  new_proc->pid = pid;
  new_proc->running = true;
  new_proc->next = process_list;
  process_list = new_proc;
  // save the terminal modes for the process
  tcgetattr(shell_terminal, &new_proc->tmodes);
}

void remove_process(pid_t pid) {
  process_t **curr = &process_list;
  while (*curr) {
    if ((*curr)->pid == pid) {
      process_t *temp = *curr;
      *curr = (*curr)->next;
      free(temp);
      return;
    }
    curr = &(*curr)->next;
  }
}

process_t* find_process(pid_t pid) {
  process_t *curr = process_list;
  while (curr) {
    if (curr->pid == pid) return curr;
    curr = curr->next;
  }
  return NULL;
}

/* Find the most recently added process */
process_t* get_recent_process() {
    return process_list;
}

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);
int cmd_wait(struct tokens* tokens);
int cmd_fg(struct tokens* tokens);
int cmd_bg(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "print current working directory"},
    {cmd_cd, "cd", "change current working directory"},
    {cmd_wait, "wait", "wait for all background jobs to finish"},
    {cmd_fg, "fg", "move process to foreground"},
    {cmd_bg, "bg", "resume process in background"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints current working directory */
int cmd_pwd(unused struct tokens* tokens) {
  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    fprintf(stdout, "%s\n", cwd);
    return 0;
  }
  return 1;
}

/* Changes current working directory */
int cmd_cd(struct tokens* tokens) {
  if (tokens_get_length(tokens) > 1) {
    char* path = tokens_get_token(tokens, 1);
    if (chdir(path) == -1) {
      return -1;
    }
    return 0;
  }
  return -1;
}

/* Waits for all background jobs to finish */
int cmd_wait(unused struct tokens* tokens) {
  while (waitpid(-1, NULL, 0) > 0);
  return 0;
}

/* Move process to foreground */
int cmd_fg(struct tokens* tokens) {
  pid_t pid = -1;
  if (tokens_get_length(tokens) > 1) {
    pid = atoi(tokens_get_token(tokens, 1));
  } else {
    process_t *proc = get_recent_process();
    if (proc) pid = proc->pid;
  }

  if (pid == -1) {
    fprintf(stderr, "fg: no current job\n");
    return -1;
  }

  process_t *proc = find_process(pid);
  if (!proc) {
      fprintf(stderr, "fg: job not found: %d\n", pid);
      return -1;
  }

  if (shell_is_interactive) {
      // Restore process terminal modes
      tcsetattr(shell_terminal, TCSADRAIN, &proc->tmodes);
      
      // Send SIGCONT if stopped
      if (!proc->running) {
          kill(-pid, SIGCONT);
          proc->running = true;
      }

      // Put the process in the foreground process group (transfer terminal control)
      tcsetpgrp(shell_terminal, pid);

      // Wait for the process to finish/pause (WUNTRACED captures paused state)
      int status;
      waitpid(pid, &status, WUNTRACED);

      // Save terminal modes(If the process is paused again, the subsequent switch needs to reuse it)
      tcgetattr(shell_terminal, &proc->tmodes);
      
      // Restore the shell's terminal control and terminal modes
      // tcsetpgrp: set the process group id for the shell
      // tcsetattr: set the terminal attributes for the shell
      tcsetpgrp(shell_terminal, shell_pgid);
      tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);

      // Handle process status: update status if paused, remove tracking if exited/signaled
      // WIFSTOPPED: check if the process is stopped
      if (WIFSTOPPED(status)) {
          proc->running = false;
          printf("[%d] Stopped\n", pid);
      } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
          // WIFEXITED: check if the process exited
          // WIFSIGNALED: check if the process was signaled
          remove_process(pid);
      }
  } else {
      int status;
      waitpid(pid, &status, 0);
      remove_process(pid);
  }
  return 0;
}

/* Resume process in background */
int cmd_bg(struct tokens* tokens) {
  pid_t pid = -1;
  if (tokens_get_length(tokens) > 1) {
    pid = atoi(tokens_get_token(tokens, 1));
  } else {
    process_t *proc = get_recent_process();
    // Try to find a stopped process preferably
    if (proc && proc->running) {
        // Just pick the first one
    }
    if (proc) pid = proc->pid;
  }

  if (pid == -1) {
    fprintf(stderr, "bg: no current job\n");
    return -1;
  }

  process_t *proc = find_process(pid);
  if (!proc) {
      fprintf(stderr, "bg: job not found: %d\n", pid);
      return -1;
  }

  if (!proc->running) {
      kill(-pid, SIGCONT);
      proc->running = true;
      printf("[%d] Running\n", pid);
  }
  return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
  
  /* Ignore interactive and job-control signals */
  // prevent the shell from being stopped by Ctrl-C, Ctrl-Z, etc.
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
}

/* Executes a program segment with redirections and path resolution */
void exec_process(struct tokens* tokens, int start, int end) {
  size_t n_tokens = end - start;
  char** args = malloc((n_tokens + 1) * sizeof(char*));
  int arg_idx = 0;
  char* input_file = NULL;
  char* output_file = NULL;

  // analyze the tokens and set the input and output files
  for (int i = start; i < end; i++) {
    char* token = tokens_get_token(tokens, i);
    if (strcmp(token, "<") == 0) {
      if (i + 1 < end) {
        input_file = tokens_get_token(tokens, ++i);
      } else {
        fprintf(stderr, "Missing input file after <\n");
        exit(1);
      }
    } else if (strcmp(token, ">") == 0) {
      if (i + 1 < end) {
        output_file = tokens_get_token(tokens, ++i);
      } else {
        fprintf(stderr, "Missing output file after >\n");
        exit(1);
      }
    } else {
      args[arg_idx++] = token;
    }
  }
  args[arg_idx] = NULL;
 
  // handle input redirection
  if (input_file) {
    // open the input file
    // O_RDONLY: read only
    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
      perror("open input file failed");
      exit(1);
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
      perror("dup2 input failed");
      exit(1);
    }
    close(fd);
  }

  // handle output redirection
  if (output_file) {
    // open the output file
    // O_WRONLY: write only
    // O_CREAT: create if not exists
    // O_TRUNC: truncate if exists
    // 0644: permissions
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("open output file failed");
      exit(1);
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
      perror("dup2 output failed");
      exit(1);
    }
    close(fd);
  }

  char* prog = args[0];
  if (!prog) {
    exit(0);
  }

  /* Check for built-in commands if running in pipe */
  int fundex = lookup(prog);
  if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
      exit(0);
  }

  // if the program is a full path, execute it directly
  if (strchr(prog, '/') != NULL) {
    if (execv(prog, args) == -1) {
      fprintf(stderr, "%s: %s\n", prog, strerror(errno));
      exit(1);
    }
  } else {
    // if the program is not a full path, search the PATH environment variable
    // get the PATH environment variable
    char* path_env = getenv("PATH");
    // copy the PATH environment variable to a new string to avoid modifying the original string
    char* path_copy = strdup(path_env);
    // strtok is a function that splits a string into tokens
    // it returns a pointer to the next token
    char* dir = strtok(path_copy, ":");
    char full_path[4096];
    int executed = 0;

    while (dir != NULL) {
      // format the full path
      // e.g. dir = /bin, prog = ls, full_path = /bin/ls
      snprintf(full_path, sizeof(full_path), "%s/%s", dir, prog);
      // check if the file is executable
      // X_OK is a constant for checking if the file is executable
      if (access(full_path, X_OK) == 0) {
        if (execv(full_path, args) == -1) {
          fprintf(stderr, "%s: %s\n", full_path, strerror(errno));
          exit(1);
        }
      }
      // pass NULL to strtok to get the next token
      dir = strtok(NULL, ":");
    }

    if (!executed) {
      fprintf(stderr, "%s: command not found\n", prog);
      free(path_copy);
      exit(1);
    }
    free(path_copy);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);
    size_t n_tokens = tokens_get_length(tokens);

    /* Check for pipes */
    int n_pipes = 0;
    for (size_t i = 0; i < n_tokens; i++) {
      if (strcmp(tokens_get_token(tokens, i), "|") == 0) n_pipes++;
    }
    
    /* Check for background execution */
    bool background = false;
    if (n_tokens > 0 && strcmp(tokens_get_token(tokens, n_tokens - 1), "&") == 0) {
        background = true;
        n_tokens--; // remove the & token from processing
    }

    // if there are no pipes, execute the command directly
    if (n_pipes == 0) {
        /* Find which built-in function to run. */
        int fundex = lookup(tokens_get_token(tokens, 0));
        if (fundex >= 0) {
          cmd_table[fundex].fun(tokens);
        } else {
          pid_t pid = fork();
          if (pid == 0) {
            if (shell_is_interactive) {
              // create a new process group, and the child process becomes the group leader
              setpgid(pid, pid);
              // give control to the child process only if foreground
              if (!background) {
                  tcsetpgrp(shell_terminal, getpid());
              }
              // restore the default signal handlers
              signal(SIGINT, SIG_DFL);
              signal(SIGQUIT, SIG_DFL);
              signal(SIGTSTP, SIG_DFL);
              signal(SIGTTIN, SIG_DFL);
              signal(SIGTTOU, SIG_DFL);
            }
            exec_process(tokens, 0, n_tokens);
          } else if (pid > 0) {
            add_process(pid); // Track process
            if (shell_is_interactive) {
              // ensure the child process is in the same process group as the shell process
              setpgid(pid, pid);
              // give control to the child process only if foreground
              if (!background) {
                  tcsetpgrp(shell_terminal, pid);
              }
            }
            
            if (!background) {
                int status;
                waitpid(pid, &status, WUNTRACED); // Wait and check if stopped
                
                // give control back to the shell process group
                if (shell_is_interactive) {
                  tcsetpgrp(shell_terminal, shell_pgid);
                  tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
                }

                if (WIFSTOPPED(status)) {
                     process_t *proc = find_process(pid);
                     if (proc) proc->running = false;
                     printf("[%d] Stopped\n", pid);
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                     remove_process(pid);
                }
            } else {
                // Background process: print info
                printf("[%d] Background\n", pid);
            }
          } else {
            perror("fork failed");
          }
        }
    } else {
        /* Pipeline execution */
        int n_cmds = n_pipes + 1;
        int pipefds[2 * n_pipes];
        for (int i = 0; i < n_pipes; i++) {
           // create a pipe for each command
           // pipefds + i * 2 is the file descriptor for the input of the pipe
           // pipefds[i * 2 + 1] is the file descriptor for the output of the pipe
           if (pipe(pipefds + i * 2) < 0) {
               perror("pipe");
               exit(1);
           }
        }

        int start = 0;
        int cmd_idx = 0;
        pid_t first_pid = 0;

        for (size_t i = 0; i <= n_tokens; i++) {
           // if the current token is a pipe or the end of the tokens
           if (i == n_tokens || strcmp(tokens_get_token(tokens, i), "|") == 0) {
               pid_t pid = fork();
               if (pid == 0) {
                   if (shell_is_interactive) {
                     // set the process group id to the first command's process group id
                     pid_t pid_to_set = (first_pid == 0) ? getpid() : first_pid;
                     setpgid(getpid(), pid_to_set);
                     // if the command is the first command and not background, 
                     // give control to the child process
                     if (first_pid == 0 && !background) {
                       tcsetpgrp(shell_terminal, pid_to_set);
                     }
                     // restore the default signal handlers
                     signal(SIGINT, SIG_DFL);
                     signal(SIGQUIT, SIG_DFL);
                     signal(SIGTSTP, SIG_DFL);
                     signal(SIGTTIN, SIG_DFL);
                     signal(SIGTTOU, SIG_DFL);
                   }

                   // if the current command is not the first command, redirect the input from the previous command
                   if (cmd_idx > 0) {
                       dup2(pipefds[(cmd_idx - 1) * 2], STDIN_FILENO);
                   }
                   // if the current command is not the last command, redirect the output to the next command
                   if (cmd_idx < n_cmds - 1) {
                       dup2(pipefds[cmd_idx * 2 + 1], STDOUT_FILENO);
                   }
                   // close all the file descriptors for the pipes
                   // avoid file descriptor leak
                   for (int k = 0; k < 2 * n_pipes; k++) {
                       close(pipefds[k]);
                   }
                   exec_process(tokens, start, i);
               }
               
               if (shell_is_interactive) {
                   // if the first command, set the process group id to the child process
                   // and give control to the child process if not background
                   if (first_pid == 0) {
                       first_pid = pid;
                       setpgid(pid, pid);
                       if (!background) {
                           tcsetpgrp(shell_terminal, pid);
                       }
                   } else { 
                       // if the command is not the first command, 
                       // set the process group id to the first command's process group id
                       setpgid(pid, first_pid);
                   }
               }

               start = i + 1;
               cmd_idx++;
           }
        }

         // in the parent process
         // close all the file descriptors for the pipes
        for (int k = 0; k < 2 * n_pipes; k++) {
            close(pipefds[k]);
        }
        
        if (!background) {
             // wait for all the commands to finish
            for (int k = 0; k < n_cmds; k++) {
                waitpid(-1, NULL, 0);
            }
            
            if (shell_is_interactive) {
                tcsetpgrp(shell_terminal, shell_pgid);
            }
        } else {
            // For pipeline, technically should track the PGID (first_pid)
             printf("[%d] Background Pipeline\n", first_pid);
             add_process(first_pid); // Simply tracking leader for now
        }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
