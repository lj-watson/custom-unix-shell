#include "shs.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Shell startup
void shs_init(void) {

    // Clear screen
    clear();

    print_loading_screen();

    sleep(1);
    clear();

}

// Shell running
void shs_go(void) {

    // Define char variables to be reused
    char hostname[HOST_NAME_MAX];
    char *username;
    char *user_in;
    struct command_list *piped_cmds;

    // Initialise shell loop
    while(1) {

        // Get hostname and username
        if(gethostname(hostname, HOST_NAME_MAX) == -1) die("gethostname() failed");
        if((username = getenv("USER")) == NULL) die("getenv() failed");

        printf("%s@%s(SHS):~$ ", username, hostname);

        // Receive user input
        user_in = readline();
        // Check empty string
        if(!user_in[0]) continue;
        // Parse user input into arguments
        int pipeFlag = 0;
        int ARGLIST_SIZE = 1024;
        char **parsed_args = NULL;
        if((parsed_args = malloc(ARGLIST_SIZE * sizeof(char**))) == NULL) die("malloc() failed");
        if(((piped_cmds = malloc(ARGLIST_SIZE*sizeof(*piped_cmds))) == NULL)) die("malloc() failed");
        pipeFlag = parseline(user_in, parsed_args, piped_cmds, &ARGLIST_SIZE);

        if(pipeFlag == -1) {
            printf("Command not recognized.\n");
            continue;
        }

        // Execute user input
        if(pipeFlag == 0) {
            shs_exe(parsed_args);
            piped_cmds[0].argv = NULL;
        }
        else shs_pipe(piped_cmds);

        // Free memory
        free(user_in);
        free(parsed_args);
        int i = 0;
        while(piped_cmds[i].argv != NULL) {
            free(piped_cmds[i].argv);
            i++;
        }
        free(piped_cmds);
    }

}

void shs_cmd(char **argv) {

    // Run a system command given by user input

    // Spawn new process
    pid_t ret_pid = fork();

    // Child
    if(ret_pid == 0) {
        if(execvp(argv[0], argv) == -1) {
            printf("Command was not found. Please try again.\n");
            exit(errno);
        }
        return;
    }
    // Parent
    if(ret_pid > 0) {
        int status;
        pid_t wret_pid;
        // Wait for child process to finish before returning
        do {
            if((wret_pid = waitpid(ret_pid, &status, WUNTRACED)) == -1) die("waitpid() failed");
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        return;
    }
    // Fork Error
    else die("fork() failed");
}

void shs_exe(char** argv) {

    // Execute user input

    // Check for built in command
    if(shs_bin(argv) == 1) return;

    // Not build in command, run system command
    shs_cmd(argv);
    return;
}

int shs_bin(char **argv) {

    int num_cmd = 5, which_cmd = -1;
    char* cmd_list[num_cmd];
    char cwd[PATH_MAX];

    // List of built in commands
    cmd_list[0] = "cd";
    cmd_list[1] = "help";
    cmd_list[2] = "exit";
    cmd_list[3] = "reversi";
    cmd_list[4] = "pwd";

    // Check input for built in command
    for(int i = 0; i < num_cmd; i++) {
        if(strcmp(argv[0], cmd_list[i]) == 0) which_cmd = i;
    }

    // Execute built in command
    switch(which_cmd) {
        case 0: 
            if(chdir(argv[1]) == -1) {
                if(argv[1] == NULL) printf("Error: no directory specified\n");
                else printf("Directory '%s' not found\n", argv[1]);
            }
            else {
                if(getcwd(cwd, sizeof(cwd)) == NULL) die("getcwd() failed");
                printf("Changed current directory to %s (SHS Shell)\n", cwd);
            }
            return 1;
        case 1: 
            print_help();
            return 1;
        case 2: 
            printf("Exiting Shell...\n");
            sleep(1);
            exit(0);
        case 3: 
            reversi();
            return 1;
        case 4:
            if(getcwd(cwd, sizeof(cwd)) == NULL) die("getcwd() failed");
            printf("%s (SHS Shell)\n", cwd);
            return 1;
        default: break;
    }

    return 0;

}

void shs_pipe(struct command_list *cmd_list) {

    // Save standard input/output file descriptor to switch back to later
    int stdout_copy = dup(1);
    int stdin_copy = dup(2);

    // First command in list reads from actual standard input
    int in = STDIN_FILENO;

    // Data channel pipe
    int fd[2];

    int i = 0;
    // Loop through all seperate command lists
    while(1) {
        if(cmd_list[i+1].argv == NULL) break;
        // Make pipe
        if(pipe(fd) == -1) die("pipe() failed");

        pid_t ret_pid;

        if((ret_pid = fork()) == -1) die("fork() failed");
        // New child process for ith command
        if(ret_pid == 0) {
            // Only need output file descriptor in pipe, so close input on child's side
            close(fd[0]);
            // Run the ith command from command list with pipe from prev. command
            run_command(in, fd[1], cmd_list[i].argv);
        }
        else {
            // i+1th command will read from prev command output, so set in to pipe input
            // Only need input from child so close output end of pipe on parent's side
            close(fd[1]);
            close(in);
            in = fd[0];
            int status;
            pid_t wret_pid;
            do {
                if((wret_pid = waitpid(ret_pid, &status, WUNTRACED)) == -1) die("waitpid() failed");
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        i++;
    }

    // Run final command, this time outputting result to standard output instead of pipe
    pid_t ret_pid;
    if((ret_pid = fork()) == -1) die("fork() failed");
    if(ret_pid == 0) {
        run_command(in, STDOUT_FILENO, cmd_list[i].argv);
    }
    else {
        // Renew original standard output and input
        close(in);
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
        dup2(stdin_copy, STDIN_FILENO);
        close(stdin_copy);
        int status;
        pid_t wret_pid;
        do {
            if((wret_pid = waitpid(ret_pid, &status, WUNTRACED)) == -1) die("waitpid() failed");
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return;
}
