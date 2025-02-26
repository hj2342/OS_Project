#ifndef EXECUTOR_H
#define EXECUTOR_H

void execute_command(char *cmd);
void execute_piped_commands(char **commands, int cmd_count);

#endif