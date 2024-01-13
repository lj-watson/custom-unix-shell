#ifndef SHS
#define SHS

struct command_list {
    char **argv;
};

void shs_init(void);
void shs_go(void);
void shs_cmd(char **argv);
void shs_exe(char **argv);
void shs_pipe(struct command_list *cmd_list);
int shs_bin(char **argv);
void die(const char* msg);
void clear(void);
void print_loading_screen(void);
char* readline(void);
int parseline(char* line, char ** arglist, struct command_list *cmd_list, int *ARGLIST_SIZE);
void print_help(void);
void reversi(void);
void run_command(int fdin, int fdout, char **argv);

#endif
