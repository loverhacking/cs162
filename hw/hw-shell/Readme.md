# HW 2: Shell
I add more tests in `test.sh`.

## Add Support for cd and pwd
* implement two new functions `cmd_pwd` and `cmd_cd`
    * use `getcwd` and `chdir` for help

## Program Execution
* use `fork` and `execv` for help
* fork a child process, which calls one of the exec functions to run the new program. 
* the parent process should wait until the child process completes and then continue listening for more commands

## Path Resolution
* use `strchr(prog, '/')` to judge whether the path is a full path or not.
* use `strtok` to split the `PATH` by `:` 
* use `snprintf` to format the full path
* look for a program in each directory on the PATH environment variable and **runs the first one that it finds**.

## Input/Output Redirection
* use `dup2` to redirect input and output

## Pipes
* Refactored code: The command execution logic originally in the main function (such as redirection parsing, path resolution, and execv calls) has been extracted into a helper function, `exec_process`. This makes it easy to reuse the execution logic inside pipelines.
* Pipe handling logic:
    * In the main loop, first scan the tokens to count the number of pipe symbols (`|`).
        * No pipe: Keep the original logic: if it is a built-in command, execute it directly; otherwise, fork a child process to call `exec_process`.
        * With pipes:
            * Create 2 * `n_pipes` file descriptors (`pipefds`).
            * Iterate over each command segment (separated by `|`).
            * For each command segment, fork a child process.
            * Child process configuration:
                * If it's not the first command, use `dup2` to map the read end of the previous pipe to `STDIN_FILENO`.
                * If it's not the last command, use `dup2` to map the write end of the current pipe to `STDOUT_FILENO`.
                * **Close all pipe file descriptors** (very important, otherwise it may cause the program to hang).
                * Call `exec_process` to execute the command.
            * Parent process:
                * Close all pipe file descriptors.
                * Wait for all child processes to finish in a loop.

## Signal Handling and Terminal Control

* Shell signal handling (`init_shell`):
    * Ignore `SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGTTIN`, and `SIGTTOU`. This ensures the shell is not accidentally terminated by signals like Ctrl+C.
* Foreground process group management (main):
    * For a single command:
        * Child process: 
            * Call `setpgid(0, 0)` to put itself into a new process group. 
            * Call `tcsetpgrp` to obtain terminal control. 
            * Restore signal handling to the default actions (`SIG_DFL`).
        * Parent process: 
            * Call `setpgid(pid, pid)` to ensure the child has its own process group. 
            * Call `tcsetpgrp` to transfer terminal control to the child. 
            * After waiting for the child process to finish, call `tcsetpgrp` to regain terminal control.
    * For pipeline commands:
        * All child processes are placed into the same process group (PGID is the PID of the first child).
        * The first child process (or parent on its behalf) sets the process group and acquires terminal control.
        * All child processes restore default signal handling.
        * The parent process, after waiting for all child processes to finish, regains terminal control.

## Background Processes
* Built-in `cmd_wait` command:
    * Added a `cmd_wait` function, which uses `waitpid(-1, NULL, 0)` in a loop to wait for all child processes to finish.
    * Registered the wait command in the `cmd_table`.
* Background execution logic:
    * When processing tokens, check if the last token is `&`.
    * If so, set the background flag to true and decrease the tokens length by one (ignoring &).
    * Single command:
        * If background is true:
            * The child process does not call `tcsetpgrp` to acquire terminal control (the shell remains in the foreground).
            * The parent process does not call `wait`, nor does it call `tcsetpgrp` to give control to the child process.
    * Pipeline commands:
        * If background is true:
            * All child processes do not acquire terminal control.
            * The parent process does not wait for the child processes.

## Foreground/Background Switching

* Process management data structure:
    * A `struct process` linked list is defined to store process PID, terminal settings (`struct termios`), and running status.
    * Helper functions `add_process`, `remove_process`, `find_process`, and `get_recent_process` are implemented.
* fg command implementation:
    * Parses the PID argument; if absent, uses the most recent process.
    * Restores the terminal settings of the process (`tcsetattr`).
    * Sends `SIGCONT` if the process is stopped.
    * Sets the process group to the foreground (`tcsetpgrp`).
    * Uses `waitpid(..., WUNTRACED)` to wait for process state changes.
    * Updates the process list according to the status (stopped or exited), and restores the shell's terminal settings and foreground control.
* bg command implementation:
    * Parses the PID argument.
    * Sends `SIGCONT` to resume a stopped process.
    * Updates the process status to running.
* Foreground/background switching integration:
    * When launching child processes (including pipelines), `add_process` is called to record the process.
    * In foreground execution, `waitpid(..., WUNTRACED)` is used to catch stop signals (like Ctrl+Z); if the process stops, its status is updated and it remains in the list.
    * In background execution, the PID is printed directly and no waiting is performed.
