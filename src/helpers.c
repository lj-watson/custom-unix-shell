#include <shs.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void die(const char *msg) {
    int error = errno;
    perror(msg);
    exit(error);
}

void clear(void) {
    printf("\033[H\033[J");
    return;
}

char *readline(void) {
    // Read a new line from the user input
    char *newcmd;
    int BUF_SIZE = 1024;
    if((newcmd = malloc(BUF_SIZE * sizeof(char))) == NULL) die("malloc() failed");   

    int curr_char;
    int pos = 0;
    // Get each character until end of input reached
    while((curr_char = getchar()) != '\n' && curr_char != EOF) {
        // Fill newcmd with current character
        newcmd[pos++] = (char)curr_char;
        // Reallocate more space if needed
        if (pos == BUF_SIZE-1) {
            BUF_SIZE += BUF_SIZE;
            if((newcmd = reallocarray(newcmd, BUF_SIZE, sizeof(char))) == NULL) die("reallocarray() failed");
        }
    }
    // Put null terminating character at end of new command string
    newcmd[pos] = '\0';
    return newcmd;
}

int parseline(char* line, char** arglist, struct command_list *cmd_list, int *ARGLIST_SIZE) {

    // Parse user input into command arguments

    int pos = 0;
    bool no_pipe = true;

    // Check for pipe command
    char* line_copy = (char*)malloc(strlen(line) * sizeof(char));
    strcpy(line_copy, line);
    strsep(&line_copy, "|");
    if(line_copy == NULL) no_pipe = false;

    // No pipe
    while(!no_pipe) {
        // Add argument from user input to argument array
        // Arguments are seperated by whitespace
        arglist[pos++] = strsep(&line, " ");
        if(arglist[pos-1] == NULL) {
            return 0;
        }
        // Check for empty input
        if(strlen(arglist[pos-1]) == 0) pos--;
        // Reallocate more space if needed
        if(pos == *ARGLIST_SIZE-1) {
            *ARGLIST_SIZE += *ARGLIST_SIZE;
            if((arglist = reallocarray(arglist, *ARGLIST_SIZE, sizeof(char))) == NULL) {
                die("reallocarray() failed");
            }
        }
    }
    
    // Command with pipe
    while(1) {
        if((cmd_list[pos].argv = malloc(*ARGLIST_SIZE*sizeof(char**)))==NULL) die("malloc() failed");
        // Get first part before delimiter
        char *cmd_unparsed = strsep(&line, "|");
        // Last command
        if(line == NULL) {
            if(cmd_unparsed[0] != ' ') return -1;
            cmd_unparsed++;
            parseline(cmd_unparsed, cmd_list[pos].argv, cmd_list, ARGLIST_SIZE);
            cmd_list[pos+1].argv = NULL;
            return 1;
        }
        // Check syntax
        if(cmd_unparsed[strlen(cmd_unparsed) - 1] != ' ') return -1;
        // Remove last whitespace
        cmd_unparsed[strlen(cmd_unparsed) - 1] = '\0';
        if(pos == 0) {
            // Parse list of first commands before pipe into cmd_list array of string lists
            parseline(cmd_unparsed, cmd_list[pos].argv, cmd_list, ARGLIST_SIZE);
        }
        else {
            // Remove first whitespace
            if(cmd_unparsed[0] != ' ') return -1;
            cmd_unparsed++;
            parseline(cmd_unparsed, cmd_list[pos].argv, cmd_list, ARGLIST_SIZE);
        }
        pos++;
        // Reallocate more space if needed
        if(pos == *ARGLIST_SIZE-1) {
            *ARGLIST_SIZE += *ARGLIST_SIZE;
            if((cmd_list = reallocarray(cmd_list, *ARGLIST_SIZE, sizeof(*cmd_list))) == NULL) {
                die("reallocarray() failed");
            }
        }    
    }
}

void print_loading_screen(void) {
    printf(  
    "***LOADING SHELL...***\n\n"
    "            *     ,MMM8&&&.            *           \n"
    "                 MMMM88&&&&&    .                  \n"
    "                MMMM88&&&&&&&                      \n"
    "    *           MMM88&&&&&&&&                      \n"
    "                MMM88&&&&&&&&                      \n"
    "                'MMM88&&&&&&'                      \n"
    "                  'MMM8&&&'      *                 \n"
    "         |\\___/|                                   \n"
    "         )     (             .              '      \n"
    "        =\\     /=                                  \n"
    "          )===(       *                            \n"
    "         /     \\                                   \n"
    "         |     |                                   \n"
    "        /       \\                                  \n"
    "        \\       /                                  \n"
    " _/\\_/\\_/\\__  _/_/\\_/\\_/\\_/\\_/\\_/\\_/\\_/\\_/\\_       \n"
    " |  |  |  |( (  |  |  |  |  |  |  |  |  |  |       \n"
    " |  |  |  | ) ) |  |  |  |  |  |  |  |  |  |       \n"
    " |  |  |  |(_(  |  |  |  |  |  |  |  |  |  |       \n"
    " |  |  |  |  |  |  |  |  |  |  |  |  |  |  |       \n"
    " |  |  |  |  |  |  |  |  |  |  |  |  |  |  |       \n"
    );
    return;
}

void print_help(void) {
    printf(
        "HELP -- LISTING SUPPORTED SHELL COMMANDS:\n"
    );
    sleep(1);
    printf(
        "$cd\n"
        "$exit\n"
        "$reversi\n"
        "$help\n"
        "$pwd\n"
    );
    sleep(1);
    printf("THANK YOU...\n");
    sleep(1);
}

void run_command(int fdin, int fdout, char **argv) {
    
    // Child reads from prev. input, so make standard input read from
    // fdin so that prev. output is piped in to this command
    if(fdin != STDIN_FILENO) {
        if(dup2(fdin, STDIN_FILENO) == -1) die("dup2() failed");
        close(fdin);
    }
    // Child writes to standard output, so make it refer to the same file
    // descriptor as fdout so we can pipe output to next command
    if(fdout != STDOUT_FILENO) {
        if(dup2(fdout, STDOUT_FILENO) == -1) die("dup2() failed");
        close(fdout);
    }

    // Execute this command
    if(execvp(argv[0], argv) == -1) die("Fatal error, command was not recognized");
    
}
