#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stddef.h>
#include <signal.h>

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <regex>
#include <list>
#include <cerrno>


void  shell_parse_command (const std::string &command /*, std::list*/ )
{
  std::regex  my_regex ("([a-zA-Z.0-9-]+)|(&&|&|[|][|]|[|]|<|>)");
  std::regex  my_re_sp ("\t|\r|\n");
  std::string cmd = std::regex_replace (command, my_re_sp, " ");

  std::regex_iterator<std::string::iterator> re_it (cmd.begin (), cmd.end (), my_regex);
  std::regex_iterator<std::string::iterator> re_end;

  while ( re_it != re_end )
  {
    std::string s = re_it->str ();

         if ( s == "&&" ) { std::cout << "operator: and"  << std::endl; }
    else if ( s == "&"  ) { std::cout << "operator: fork" << std::endl; }
    else if ( s == "|"  ) { std::cout << "operator: pipe" << std::endl; }
    else if ( s == "||" ) { std::cout << "operator: or"   << std::endl; }
    else if ( s == ">"  ) { std::cout << "operator: stdout  to file erase"     << std::endl; }
    else if ( s == ">>" ) { std::cout << "operator: stdout  to file non-erase" << std::endl; }
    else if ( s == "<"  ) { std::cout << "operator: stdin from file erase"     << std::endl; }
    else if ( s == "<<" ) { std::cout << "operator: stdin from file non-erase" << std::endl; }
    else
    {
      std::cout << s << std::endl;
    }
    ++re_it;
  }
}

// конвеер
void  shell_pipeline (std::list<std::string> &programs)
{
  int pfd[2];
  pipe (pfd);

  if ( !fork () )
  {
    close  (STDOUT_FILENO);
    dup2   (pfd[1], STDOUT_FILENO);
    close  (pfd[0]); close (pfd[1]);
    execlp ("who", "who", NULL);
  }
  if ( !fork () )
  {
    close  (STDIN_FILENO);
    dup2   (pfd[0], STDIN_FILENO);
    close  (pfd[0]); close (pfd[1]);
    execlp ("wc", "wc", " - l", NULL);
  }
  
  close (pfd[0]);
  close (pfd[1]);
  wait (NULL);
  wait (NULL);
}

int  main (int argc, char** argv)
{
  std::string command_example (" head <file.txt|sort\t -n  || cat > file.out && echo ok\n");
  std::string command_line;

  shell_parse_command (command_example);
  std::cin >> command_line;

  int pid = 0; // fork ();
  if ( pid < 0 )
  {
    /* fork потерпел неудачу */
    // handle_error ();
    
    execvp (argv[1], &argv[1]);
    std::perror ("execvp");
    return 1; // Never get there normally
  }
  else if ( pid > 0 )
  {
    /* здесь располагается родительский код */
    int status;
    if ( wait (&status) == -1 )
    {
      std::perror ("wait");
      return 1;
    }
    if ( WIFEXITED (status) )
      std::printf ("Child terminated normally with exit code %i\n",
      WEXITSTATUS (status));
    if ( WIFSIGNALED (status) )
      std::printf ("Child was terminated by a signal #%i\n", WTERMSIG (status));
    if ( WCOREDUMP (status) )
      std::printf ("Child dumped core\n");
    if ( WIFSTOPPED (status) )
      std::printf ("Child was stopped by a signal #%i\n", WSTOPSIG (status));
  }
  else
  {
    /* здесь располагается дочерний код */
  }


  return 0;
}