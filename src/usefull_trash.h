#include <stdio.h>
#ifndef USEFULL_TRASH_H_INCLUDED
#define USEFULL_TRASH_H_INCLUDED
int compare (const void * a, const void * b);
int fork_n_times(int n);
void throwing_pipe_errors(int fd);
void throwing_fork_errors(int fk);
void fork_exec_with_inp_redir2(char *for_exe, int flag, const char * file);
void fork_exec_with_inp_redir(char **for_exe, int flag, const char * file);
int process_sc(const char * to_edit, char* outp, char * lookfor, char * replace);

#endif // USEFULL_TRASH_H_INCLUDED
