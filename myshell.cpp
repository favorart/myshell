#include "myshell.h"

#define  MYSHELL_PIPE_FDS_OFST 2
//----------------------------------------------------------------------------------
inline myshell_pipe_enum  operator|  (myshell_pipe_enum  a, myshell_pipe_enum b)
{ return static_cast<myshell_pipe_enum> (static_cast<char> (a) | static_cast<char> (b));  }
inline myshell_pipe_enum  operator|= (myshell_pipe_enum &a, myshell_pipe_enum b)
{ return a = (a | b); }
inline myshell_file_enum  operator|= (myshell_file_enum &a, myshell_file_enum b)
{ return a = static_cast<myshell_file_enum> (a | b); }
//----------------------------------------------------------------------------------
void  myshell_cmd_parse (std::list<std::string> &cmds, const std::string &line)
{
  std::regex  re_cmds ("([a-zA-Z0-9.$;-]+)|(&&|&|[|][|]|[|]|<|>)");
  std::regex  re_spcs ("[\t\r\n ]+");

  std::string cmds_parsed = std::regex_replace (line, re_spcs, " ");

  std::regex_iterator<std::string::iterator>  re_end,  re_it (cmds_parsed.begin (), cmds_parsed.end (), re_cmds);
  while ( re_it != re_end )
  {
    std::string s = re_it->str ();
    cmds.push_back (s);

    ++re_it;
  }
}
//----------------------------------------------------------------------------------
int   myshell_cmd_lunch (int *argc,  char **argv, 
                         myshell_file_enum  file_flags, const char *f_inc, const char *f_out,
                         myshell_pipe_enum  pipe_flags, fd_t  fds[MYSHELL_PIPE_FDS])
{
  pid_t pid = fork ();
  //--------------------
  if ( pid == 0 )
  {
    argv[*argc] = NULL;

    //------------------------------------
    if ( file_flags & MYSHELL_FILE_INC )
    {
      // fd_t stdin_copy = dup (STDIN_FILENO);
      close (STDIN_FILENO);
	    fd_t fd = open(f_inc, O_RDONLY, S_IRUSR | S_IWUSR);
      /* must be 0, because returns always the lowest value */

#ifdef _DEBUG
      std::cerr << "debug: fin= " << fd << std::endl;
#endif // _DEBUG

      // close (fd);
      // dup2  (stdin_copy, STDIN_FILENO);
      // close (stdin_copy);
    }
    if ( file_flags & MYSHELL_FILE_OUT )
    {
      // fd_t stdout_copy = dup (STDOUT_FILENO);
      close (STDOUT_FILENO);
	    fd_t fd = open(f_out, O_APPEND | O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
      /* the lowest available value */

#ifdef _DEBUG
      std::cerr << "debug: fout= " << fd << std::endl;
#endif // _DEBUG

      // close (fd);
      // dup2  (stdout_copy, STDOUT_FILENO);
      // close (stdout_copy);
    }
    //------------------------------------
    if ( pipe_flags & MYSHELL_PIPE_INC )
    {
      close (STDIN_FILENO);
      dup2  (fds[0], STDIN_FILENO);
    }
    if ( pipe_flags & MYSHELL_PIPE_OUT )
    {
      close (STDOUT_FILENO);
      dup2  (fds[1], STDOUT_FILENO);
    }
    
    /* To no pid flows away */
    if ( pipe_flags )
    {
      for ( int i = 0; i < MYSHELL_PIPE_FDS; ++i )
        close (fds[i]);
    }
    //------------------------------------
    /* Create the child process */
    if ( execvp (argv[0], argv) == -1 )
    { perror ("execvp");
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
    *argc = 0;
    //------------------------------------
    if ( pipe_flags & MYSHELL_PIPE_PID )
    {
      sigint_enable (pid);
      //------------------------------------
      /* Parent process */
      int child_status;
      if ( waitpid (pid, &child_status, 0) == -1 )
      { std::perror ("waitpid");
        std::exit (EXIT_FAILURE);
      }
      //------------------------------------
      sigint_disable ();
      //------------------------------------
      close (fds[0]); fds[0] = -1;
      //------------------------------------
      if ( WIFEXITED (child_status) )
      {
        //------------------------------------
        int ret = WEXITSTATUS (child_status);
        // printf ("Child terminated normally with exit code %i\n", ret_stat);
        std::cerr << "Process " << pid << " exited: " << ret << std::endl;
        return ret;
      }
      else
      {
        // ??? return, what wait*() */
        return EXIT_SUCCESS; // EXIT_FAILURE;
      }
      //------------------------------------
    }
    return pid;
  }
}
//----------------------------------------------------------------------------------
int   myshell_execution (std::list<std::string> &cmds, std::list<pid_t> &backgrounds)
{
  int result = EXIT_SUCCESS;
  int cmds_size = (cmds.size () + 1);
  //------------------------------------
  int     argc = 0, argsc = 0, is_pip = 0;
  char  **argv = new char* [cmds_size];
  char ***args = new char**[cmds_size];

  for ( int j = 0; j < cmds_size; ++j )
    args[j] = new char*[cmds_size];
  //------------------------------------
  myshell_file_enum  file_flags = MYSHELL_FILE_NOT;
  const char *f_inc = NULL, *f_out = NULL;
  //------------------------------------
  myshell_pipe_enum  pipe_flags = MYSHELL_PIPE_NOT;
  fd_t fds[MYSHELL_PIPE_FDS];
  memset (fds, -1, sizeof (fds));
  //------------------------------------
  // for ( std::string &s : cmds )
  for ( auto it = cmds.begin (); it != cmds.end (); ++it )
  {
    std::string &s = *it;

    if ( sigint_flag )
    {
      sigint_flag = false;
      result = EXIT_FAILURE;
      break;
    }
    //------------------------------------
#ifdef _DEBUG
    std::cerr << "debug: ";
    for ( int k = 0; k < argc; ++k )
      std::cerr << argv[k] << " ' ";
    std::cerr << std::endl;
#endif // _DEBUG

         if ( s == "<" )
    {
      ++it; s = *it;
      f_inc = s.c_str ();
      file_flags |= MYSHELL_FILE_INC;

#ifdef _DEBUG
      std::cerr << "debug file_io: " << f_inc << " " << f_out << std::endl;
#endif // _DEBUG
    }
    else if ( s == ">" )
    {
      ++it; s = *it;
      f_out = s.c_str ();
      file_flags |= MYSHELL_FILE_OUT;

#ifdef _DEBUG
      std::cerr << "debug file_io: " << f_inc << " " << f_out << std::endl;
#endif // _DEBUG
    }
    else if ( s == "|" )
    {
      //------------------------------------
      /* Create the pipe */
      if ( pipe (fds + MYSHELL_PIPE_FDS_OFST) == -1 ) // lfds[1] = fds[1] -> lfds[0] = next fds[0]
      { std::perror ("pipe");
        std::exit (EXIT_FAILURE);
      }

      std::swap (fds[1], fds[1 + MYSHELL_PIPE_FDS_OFST]);
      pipe_flags |= MYSHELL_PIPE_OUT;

      if ( !(pipe_flags & MYSHELL_PIPE_INC) )
      {
        std::swap (fds[0], fds[0 + MYSHELL_PIPE_FDS_OFST]);
      }
      //------------------------------------
      int  bg_pid = myshell_cmd_lunch (&argc, argv, file_flags, f_inc, f_out, pipe_flags, fds);
      backgrounds.push_back (bg_pid);

      file_flags = MYSHELL_FILE_NOT;
      f_inc = NULL; f_out = NULL;
      //------------------------------------
      if ( pipe_flags & MYSHELL_PIPE_INC )
      {
        close (fds[0]); fds[0] = -1;
        std::swap (fds[0], fds[0 + MYSHELL_PIPE_FDS_OFST]);
        // close (fds[1 + MYSHELL_PIPE_FDS_OFST]); // already closed
      }
      pipe_flags = MYSHELL_PIPE_INC;
      /* skip closing zero, because it last inc */
      close (fds[1]); fds[1] = -1;
      //------------------------------------
    }
    else if ( s == "||" )
    {
      pipe_flags |= MYSHELL_PIPE_PID;
	    if ( !myshell_cmd_lunch(&argc, argv, file_flags, f_inc, f_out, pipe_flags, fds) )
        break;

      file_flags = MYSHELL_FILE_NOT;
      f_inc = NULL; f_out = NULL;

      pipe_flags = MYSHELL_PIPE_NOT;
    }
    else if ( s == "&&" )
    {
      pipe_flags |= MYSHELL_PIPE_PID;
      if ( myshell_cmd_lunch (&argc, argv, file_flags, f_inc, f_out, pipe_flags, fds) )
        break;

      file_flags = MYSHELL_FILE_NOT;
      f_inc = NULL; f_out = NULL;

      pipe_flags = MYSHELL_PIPE_NOT;
    }
    else
    {
      argv[argc++] = (char*) s.c_str ();
    }   
  }
  //------------------------------------
  if ( argc )
  {
    pipe_flags |= MYSHELL_PIPE_PID;
    myshell_cmd_lunch (&argc, argv, file_flags, f_inc, f_out, pipe_flags, fds);

    file_flags = MYSHELL_FILE_NOT;
    f_inc = NULL; f_out = NULL;

    pipe_flags = MYSHELL_PIPE_NOT;
  }

  //------------------------------------
  for ( int j = 0; j < cmds_size; ++j )
    delete[] args[j];

  delete[] args;
  delete[] argv;
  //------------------------------------
  cmds.clear ();
  //------------------------------------
  return result;
}
//----------------------------------------------------------------------------------
