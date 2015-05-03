#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstdio>
#include <regex>
#include <list>
#include <cerrno>

sig_atomic_t flag_sigint;
pid_t  child_foreground_pid; // можно как-то передать pid в обработчик?
void sigint_handle (int sig)
{ 
  // printf («proc interrapted\n»);
  flag_sigint = 1;
  if( child_foreground_pid )
    kill (child_foreground_pid, SIGINT);
}

int   shell_cmd_lunch (int argc,  char  **argv)
{
  //--------------------
  pid_t pid = fork ();
  if ( pid == 0 )
  {
    //------------------------------------
         if ( argv[argc - 2] == "<" )
    {
      char *in_file = argv[argc - 1];
      argv[argc - 2] = NULL;

      // int  stdin_copy = dup ( STDIN_FILENO);
      close (STDIN_FILENO);
      /* must be 0, because returns always the lowest value */
      int fd = open (in_file, O_RDONLY);

      // close (fd);
      // dup2  (stdin_copy, STDIN_FILENO);
      // close (stdin_copy);
    }
    else if ( argv[argc - 2] == ">" )
    {
      char *out_file = argv[argc - 1];
      argv[argc - 2] = NULL;

      // int stdout_copy = dup (STDOUT_FILENO);
      close (STDOUT_FILENO);
      // the lowest available value
      int fd = open (out_file, O_APPEND | O_TRUNC | O_CREAT);

      // close (fd);
      // dup2 (stdout_copy, 1);
      // close (stdout_copy);
    }
    //------------------------------------
    /* Child process */
    if ( execvp (argv[0], argv) == -1 )
    {
      perror ("execvp");
      exit (EXIT_FAILURE);
    }
    //------------------------------------
  }
  else if ( pid < 0 )
  {
    //------------------------------------
    /* Error forking */
    perror ("fork");
    exit (EXIT_FAILURE);
  }
  else
  {
    child_foreground_pid = pid;
    //------------------------------------
    /* Parent process */
    int child_status;
    if ( waitpid (pid, &child_status, 0) == -1 )
    {
      std::perror ("waitpid");
      std::exit (EXIT_FAILURE);
    }
    //------------------------------------
    child_foreground_pid = 0;
    //------------------------------------
    if ( WIFEXITED (child_status) )
    {
      //------------------------------------
      int ret = WEXITSTATUS (child_status);
      // printf ("Child terminated normally with exit code %i\n", ret_stat);
      return ret;
    }
    else
    {
      // ???
      return EXIT_FAILURE;
    }
    /* !!! return, what wait*() */
    //------------------------------------
  }
}
int   shell_pipelineN (int argsc, char ***args)
{
  pid_t wpid;
  //------------------------------
  if ( argsc < 2 )
  {
    std::perror ("pipeline");
    std::exit (EXIT_FAILURE);
  }
  //------------------------------
  pid_t pfd_a[2];
  pid_t pfd_b[2];

  for ( int i = 0; i < (argsc - 1); ++i )
  {
    //------------------------------------
    /* Create the pipe */
    if ( pipe (pfd_a) == -1 )
    {
      std::perror ("pipe");
      std::exit (EXIT_FAILURE);
    }

    if ( !fork () )
    {
      //------------------------------------
      /* child1 */
      if ( i )
      {
        close (STDIN_FILENO);
        dup2  (pfd_b[0], STDIN_FILENO);
        close (pfd_b[0]);
        close (pfd_b[1]);
      }

      close  (STDOUT_FILENO);
      dup2   (pfd_a[1], STDOUT_FILENO);
      close  (pfd_a[0]);
      close  (pfd_a[1]);
      execvp (args[i][0], args[i]);
    }
    if ( !(wpid = fork ()) )
    {
      //------------------------------------
      /* child2 */
      close (STDIN_FILENO);
      dup2  (pfd_a[0], STDIN_FILENO);
      close (pfd_a[0]);
      close (pfd_a[1]);

      if ( i != (argsc - 2) )
      {
        if ( pipe (pfd_b) == -1 )
        {
          std::perror ("pipe");
          std::exit (EXIT_FAILURE);
        }

        close (STDOUT_FILENO);
        dup2  (pfd_b[1], STDOUT_FILENO);
        close (pfd_b[0]);
        close (pfd_b[1]);
      }
      execvp (args[i + 1][0], args[i + 1]);
    }

    /* parent */
    close (pfd_a[0]);
    close (pfd_a[1]);

    close (pfd_b[0]);
    close (pfd_b[1]);
  }
  //------------------------------------
  child_foreground_pid = wpid;
  //------------------------------------
  int child_status;
  if ( waitpid (wpid, &child_status, 0) == -1 )
  {
    std::perror ("waitpid");
    std::exit (EXIT_FAILURE);
  }
  //------------------------------------
  child_foreground_pid = 0;
  //------------------------------------
  if ( WIFEXITED (child_status) )
  {
    //------------------------------------
    int ret = WEXITSTATUS (child_status);
    // printf ("Child terminated normally with exit code %i\n", ret_stat);
    return ret;
  }
  else
  {
    // ???
    return EXIT_FAILURE;
  }
  //------------------------------------
}
int   shell_cmd_parse (const std::string &command, std::list<std::string> &cmd_parsed)
{
  std::regex  my_regex ("([a-zA-Z.0-9-]+)|(&&|&|[|][|]|[|]|<|>)");
  std::regex  my_re_sp ("\t|\r|\n");

  std::string cmd = std::regex_replace (command, my_re_sp, " ");

  std::regex_iterator<std::string::iterator> re_end, re_it (cmd.begin (), cmd.end (), my_regex);
  while ( re_it != re_end )
  {
    std::string s = re_it->str ();
    cmd_parsed.push_back (s);
    ++re_it;
  }
}
int   shell           (std::list<std::string> &cmds)
{
  int     argc = 0, argsc = 0, is_pip = 0;
  char  **argv = new char* [cmds.size ()];
  char ***args = new char**[cmds.size ()];

  for ( int j = 0; j < cmds.size (); ++j )
    args[j] = new char*[cmds.size ()];
  //------------------------------------
  for ( std::string &s : cmds )
  {
    if ( flag_sigint )
    {
      flag_sigint = 0;
      return EXIT_FAILURE;
    }
    //------------------------------------
    if ( s == "|" || is_pip )
    {
      is_pip = 1;
      memcpy (args[argsc], argv, argc);
      argc = 0;
      ++argsc;

      if ( is_pip == 2 )
      {
        char **tmp = args[argsc];
        args[argsc] = NULL;

        shell_pipelineN (argsc, args);

        args[argsc] = tmp;
        argsc = 0;

        is_pip = 0;
      }
    }
    else if ( s == "||" )
    {
      if ( is_pip )
      {
        is_pip = 2;
        continue;
      }

      argv[argc] = NULL;
      if ( shell_cmd_lunch (argc, argv) )
        return EXIT_SUCCESS;
      argc = 0;
    }
    else if ( s == "&&" )
    {
      if ( is_pip )
      {
        is_pip = 2;
        continue;
      }

      argv[argc] = NULL;
      if ( !shell_cmd_lunch (argc, argv) )
        return EXIT_SUCCESS;
      argc = 0;
    }
    //------------------------------------
    argv[argc++] = (char*) s.c_str ();
  }
  //------------------------------------
  if ( is_pip )
  {
    char **tmp = args[argsc];
    args[argsc] = NULL;

    shell_pipelineN (argsc, args);

    args[argsc] = tmp;
    argsc = 0;

    is_pip = 0;
  }

  if ( argc )
  {
    argv[argc] = NULL;
    shell_cmd_lunch (argc, argv);
    argc = 0;
  }
  //------------------------------------
  for ( int j = 0; j < cmds.size (); ++j )
    delete[] args[j];

  delete[] args;
  delete[] argv;
  //------------------------------------
  return EXIT_SUCCESS;
}
int  main (int argc, char** argv)
{
  child_foreground_pid = 0;
  //------------------------------------
  // std::string command_example (" head <file.txt|sort\t -n  || cat > file.out && echo ok\n");
  std::string command_line; // = command_example;

  std::list<std::string> cmds;
  std::list<pid_t> backgrounds;

  while ( true )
  {
    std::cout << "myshell: ";
    std::cout.flush ();

    std::getline (std::cin, command_line);
    shell_cmd_parse (command_line, cmds);

    if ( cmds.back () == "&" )
    {
      cmds.pop_back ();
      //------------------------------------
      /* in background */
      pid_t  pid = fork ();
      if ( pid < 0 )
      {
        std::perror ("fork");
        exit (EXIT_FAILURE);
      }
      else if ( pid > 0 )
      {
        //------------------------------------
        /* parent code */
        backgrounds.push_back (pid);
        std::cerr << "Spawned child process " << pid << std::endl;
      }
      else
      {
        //------------------------------------
        /* child code */
        shell (cmds);
        return EXIT_SUCCESS;
      }
    } // end if &
    else
    {
      //------------------------------------
      /* append signal handler */
      struct sigaction sa;
      sigset_t sigintset;
      sigemptyset (&sigintset);
      sigaddset (&sigintset, SIGHUP);
      sigaddset (&sigintset, SIGINT);
      /* ... */
      sigprocmask (SIG_BLOCK, &sigintset, 0);
      sa.sa_handler = sigint_handle;
      sigaction (SIGINT, &sa, 0);
      //------------------------------------
      shell (cmds);
      //------------------------------------
      /* remove signal handler */
      sigemptyset (&sigintset);
      sigaddset (&sigintset, SIGHUP);
      sigprocmask (SIG_BLOCK, &sigintset, 0);
      sa.sa_handler = NULL;
      sigaction (SIGINT, &sa, 0);
      //------------------------------------
    }
    
    for ( auto pid : backgrounds )
    {
      int child_status;
      /* check for all childs statuses */
      waitpid (pid, &child_status, WNOHANG);
      if ( WIFEXITED (child_status) )
      {
        //------------------------------------
        int ret = WEXITSTATUS (child_status);
        std::cerr << "Process " << pid << "exited: "<< ret << std::endl;
      }
      // else /* not yet */
    }
  }
  return EXIT_SUCCESS;
}

/*
 *  int   shell_pipeline2 (char **args1, char **args2)
 *  {
 *  pid_t wpid;
 *  pid_t pfd[2];
 *  
 *  /* Create the pipe * /
 *  if ( pipe (pfd) == -1 )
 *  {
 *    perror ("pipe");
 *    std::exit (1);
 *  }
 *  
 *  if ( !fork () )
 *  {
 *    // child1
 *    close (STDOUT_FILENO);
 *    dup2 (pfd[1], STDOUT_FILENO);
 *    close (pfd[0]);
 *    close (pfd[1]);
 *    execvp (args1[0], args1);
 *  }
 *  if ( !fork () )
 *  {
 *    // child2
 *    close (STDIN_FILENO);
 *    dup2 (pfd[0], STDIN_FILENO);
 *    close (pfd[0]);
 *    close (pfd[1]);
 *    execvp (args2[0], args2);
 *  }
 *  
 *  
 *  // parent
 *  close (pfd[0]);
 *  close (pfd[1]);
 *  
 *  int ch_status, ret_stat = EXIT_FAILURE;
 *  if ( waitpid (wpid, &ch_status, 0) == -1 )
 *  {
 *    std::perror ("waitpid");
 *    std::exit (EXIT_FAILURE);
 *  }
 *  
 *  if ( WIFEXITED (ch_status) )
 *  {
 *    ret_stat = WEXITSTATUS (ch_status);
 *    // printf ("Child terminated normally with exit code %i\n", ret_stat);
 *  }
 *  return ret_stat;
 *  }
 */
