#pragma once

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <utility>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <regex>
#include <list>

typedef int fd_t;
//----------------------------------------------------------------------------------
extern volatile sig_atomic_t  sigint_flag;

void  sigint_handle  (int sig);
void  sigint_enable  (pid_t child_pid);
void  sigint_disable (void);
//----------------------------------------------------------------------------------
int  set_cloexec_flag (int desc, bool set);
//----------------------------------------------------------------------------------
enum myshell_file_enum : char
{
  MYSHELL_FILE_NOT = 0U,
  MYSHELL_FILE_INC = 1U,
  MYSHELL_FILE_OUT = 2U
};
enum myshell_pipe_enum : char
{
  MYSHELL_PIPE_NOT = 0U,
  MYSHELL_PIPE_INC = 1U,
  MYSHELL_PIPE_OUT = 2U,
  MYSHELL_PIPE_PID = 4U
};

#define  MYSHELL_PIPE_FDS 4

int   myshell_cmd_lunch (int *argc, char **argv, myshell_pipe_enum  pipe, fd_t fds[MYSHELL_PIPE_FDS]);
void  myshell_cmd_parse (std::list<std::string> &cmds, const std::string &line);
int   myshell_execution (std::list<std::string> &cmds, std::list<pid_t> &backgrounds);
//----------------------------------------------------------------------------------
// #define _DEBUG
// #define _DEBUG_PARSE

