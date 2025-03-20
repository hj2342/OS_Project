#ifndef EXECUTOR_H
#define EXECUTOR_H

int execute_command(char *cmd);
int execute_piped_commands(char **commands, int cmd_count);

#endif