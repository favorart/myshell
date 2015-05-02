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

int ch_pid; // можно как-то передать pid в обработчик?
void sigint_handle (int sig)
{ // printf («proc interrapted\n»);
  kill (ch_pid, SIGINT);
}

void  shell           (std::list<std::string> &cmds)
{
  int i = 0;
  char** args = new char*[cmds.size () + 1];

  int in_and = 0, in_or = 0, in_pip = 0;

  for ( std::string &s : cmds )
  {
    
     if ( s == "|" )
    {
      in_pip = 1;
      // int n;
      // char ***args;
      // shell_pipeline (args, n);
    
    }
    else if ( s == "||" )
    {
      args[i] = NULL;
      if ( shell_cmd_lunch (i, args) )
        return;
      i = 0;
    }
    else if ( s == "&&" )
    {
      args[i] = NULL;
      if ( !shell_cmd_lunch (i, args) )
        return;
      i = 0;
    }
    else
    { args[i++] = (char*) s.c_str (); }

    // cmds.pop_front ();
  }

  delete[] args;
}
int   shell_cmd_lunch (int argc, char **argv)
{
  //--------------------
  pid_t pid = fork ();
  if ( pid == 0 )
  {
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
      int fd = open (out_file, O_WRONLY);

      // close (fd);
      // dup2 (stdout_copy, 1);
      // close (stdout_copy);
    }
    // Child process
    if ( execvp (argv[0], argv) == -1 )
    {
      perror ("execvp");
      exit (EXIT_FAILURE);
    }  
  }
  else if ( pid < 0 )
  {
    // Error forking
    perror ("fork");
    exit (EXIT_FAILURE);
  }
  else
  {
    // Parent process
    int status;
    do
    {
      pid_t wpid = waitpid (pid, &status, WUNTRACED);
    } while ( !WIFEXITED (status) && !WIFSIGNALED (status) );
  }
  return; // !!! return, what wait*()
}
int   shell_pipeline2 (char **args1, char **args2)
{
  int pfd[2];

  /* Create the pipe */
  if ( pipe (pfd) == -1 )
  {
    perror ("pipe");
    std::exit (1);
  }

  if ( !fork () )
  {
    // child1
    close (STDOUT_FILENO);
    dup2 (pfd[1], STDOUT_FILENO);
    close (pfd[0]);
    close (pfd[1]);
    execvp (args1[0], args1);
  }
  if ( !fork () )
  {
    // child2
    close (STDIN_FILENO);
    dup2 (pfd[0], STDIN_FILENO);
    close (pfd[0]);
    close (pfd[1]);
    execvp (args2[0], args2);
  }
  // parent
  close (pfd[0]);
  close (pfd[1]);
  wait (NULL);

  return wait (NULL);
}
void  shell_cmd_parse (const std::string &command, std::list<std::string> &cmd_parsed)
{
  std::regex  my_regex ("([a-zA-Z.0-9-]+)|(&&|&|[|][|]|[|]|<|>)");
  std::regex  my_re_sp ("\t|\r|\n");

  std::string cmd = std::regex_replace (command, my_re_sp, " ");

  std::regex_iterator<std::string::iterator> re_end, re_it (cmd.begin (), cmd.end (), my_regex);
  while ( re_it != re_end )
  {
    std::string s = re_it->str ();
    cmd_parsed.push_back (s);

    //      if ( s == "&&" ) { std::cout << ">>> op: and" << std::endl; }
    // else if ( s == "||" ) { std::cout << ">>> op: or"  << std::endl; }
    // else if ( s == "&"  ) { std::cout << ">>> op: background" << std::endl; }
    // else if ( s == "|"  ) { std::cout << ">>> op: pipeline"   << std::endl; }
    // else if ( s == ">"  ) { std::cout << ">>> op: stdout  to file erase" << std::endl; }
    // else if ( s == "<"  ) { std::cout << ">>> op: stdin from file erase" << std::endl; }
    // else
    // {
    //   std::cout << s << std::endl;
    // }
    ++re_it;
  }
}

int  main (int argc, char** argv)
{
  // std::string command_example (" head <file.txt|sort\t -n  || cat > file.out && echo ok\n");
  std::string command_line; // = command_example;

  std::list<std::string> cmds;
  std::list<pid_t> backgrounds;

  // ??
  // signal (SIGINT, &sigint_handle);
  // ??
  // struct sigaction sa;
  // sigset_t newset;
  // sigemptyset (&newset);
  // sigaddset (&newset, SIGHUP);
  // sigprocmask (SIG_BLOCK, &newset, 0);
  // sa.sa_handler = sigint_handle;
  // sigaction (SIGINT, &sa, 0);

  while ( true )
  {
    std::cout << "myshell: ";
    std::cout.flush ();

    std::getline (std::cin, command_line);
    shell_cmd_parse (command_line, cmds);

    if ( cmds.back () == "&" )
    {
      cmds.pop_back ();
      // in background
      pid_t  pid = fork ();
      if ( pid < 0 )
      {
        std::perror ("fork");
        exit (EXIT_FAILURE);
      }
      else if ( pid > 0 )
      {
        /* parent code */
        backgrounds.push_back (pid);
        std::cerr << "Spawned child process " << pid << std::endl;

        // int status;
        // if ( wait (&status) == -1 )
        // {
        //   std::perror ("wait");
        //   exit (EXIT_FAILURE);
        // }
        // 
        // if ( WIFEXITED (status) )
        //   std::printf ("Child terminated normally with exit code %i\n",
        //   WEXITSTATUS (status));
        // if ( WIFSIGNALED (status) )
        //   std::printf ("Child was terminated by a signal #%i\n", WTERMSIG (status));
        // if ( WCOREDUMP (status) )
        //   std::printf ("Child dumped core\n");
        // if ( WIFSTOPPED (status) )
        //   std::printf ("Child was stopped by a signal #%i\n", WSTOPSIG (status));
      }
      else
      {
        /* child code */
        shell (cmds);
        return EXIT_SUCCESS;
      }
    } // end if &
    else
    {
      shell (cmds);
    }
    
    for ( auto pid : backgrounds )
    {
      int chld_status;
      waitpid (pid, &chld_status, 0);
      if ( WIFEXITED (chld_status) )
      {
        chld_status = WEXITSTATUS (chld_status);
        std::cerr << "Process " << pid << "exited: "<< chld_status << std::endl;
      }
      // else /* not yet */
    }
  }
  return EXIT_SUCCESS;
}


typedef void (*sighandler_t)(int);
static char *my_argv[100], *my_envp[100];
static char *search_path[10];

void handle_signal (int signo)
{
  printf ("\n[MY_SHELL ] ");
  fflush (stdout);
}

void fill_argv (char *tmp_argv)
{
  char *foo = tmp_argv;
  int index = 0;
  char ret[100];
  bzero (ret, 100);
  while ( *foo != '\0' )
  {
    if ( index == 10 )
      break;

    if ( *foo == ' ' )
    {
      if ( my_argv[index] == NULL )
        my_argv[index] = (char *) malloc (sizeof (char) * strlen (ret) + 1);
      else
      {
        bzero (my_argv[index], strlen (my_argv[index]));
      }
      strncpy (my_argv[index], ret, strlen (ret));
      strncat (my_argv[index], "\0", 1);
      bzero (ret, 100);
      index++;
    }
    else
    {
      strncat (ret, foo, 1);
    }
    foo++;
    /*printf("foo is %c\n", *foo);*/
  }
  my_argv[index] = (char *) malloc (sizeof (char) * strlen (ret) + 1);
  strncpy (my_argv[index], ret, strlen (ret));
  strncat (my_argv[index], "\0", 1);
}

void copy_envp (char **envp)
{
  int index = 0;
  for ( ; envp[index] != NULL; index++ )
  {
    my_envp[index] = (char *) malloc (sizeof (char) * (strlen (envp[index]) + 1));
    memcpy (my_envp[index], envp[index], strlen (envp[index]));
  }
}

void get_path_string (char **tmp_envp, char *bin_path)
{
  int count = 0;
  char *tmp;
  while ( 1 )
  {
    tmp = strstr (tmp_envp[count], "PATH");
    if ( tmp == NULL )
    {
      count++;
    }
    else
    {
      break;
    }
  }
  strncpy (bin_path, tmp, strlen (tmp));
}

void insert_path_str_to_search (char *path_str)
{
  int index = 0;
  char *tmp = path_str;
  char ret[100];

  while ( *tmp != '=' )
    tmp++;
  tmp++;

  while ( *tmp != '\0' )
  {
    if ( *tmp == ':' )
    {
      strncat (ret, "/", 1);
      search_path[index] = (char *) malloc (sizeof (char) * (strlen (ret) + 1));
      strncat (search_path[index], ret, strlen (ret));
      strncat (search_path[index], "\0", 1);
      index++;
      bzero (ret, 100);
    }
    else
    {
      strncat (ret, tmp, 1);
    }
    tmp++;
  }
}

int attach_path (char *cmd)
{
  char ret[100];
  int index;
  int fd;
  bzero (ret, 100);
  for ( index = 0; search_path[index] != NULL; index++ )
  {
    strcpy (ret, search_path[index]);
    strncat (ret, cmd, strlen (cmd));
    if ( (fd = open (ret, O_RDONLY)) > 0 )
    {
      strncpy (cmd, ret, strlen (ret));
      close (fd);
      return 0;
    }
  }
  return 0;
}

void call_execve (char *cmd)
{
  int i;
  printf ("cmd is %s\n", cmd);
  if ( fork () == 0 )
  {
    i = execve (cmd, my_argv, my_envp);
    printf ("errno is %d\n", errno);
    if ( i < 0 )
    {
      printf ("%s: %s\n", cmd, "command not found");
      exit (1);
    }
  }
  else
  {
    wait (NULL);
  }
}

void free_argv ()
{
  int index;
  for ( index = 0; my_argv[index] != NULL; index++ )
  {
    bzero (my_argv[index], strlen (my_argv[index]) + 1);
    my_argv[index] = NULL;
    free (my_argv[index]);
  }
}

int main (int argc, char *argv[], char *envp[])
{
  char c;
  int i, fd;
  char *tmp      = (char *) malloc (sizeof (char) * 100);
  char *path_str = (char *) malloc (sizeof (char) * 256);
  char *cmd      = (char *) malloc (sizeof (char) * 100);

  signal (SIGINT, SIG_IGN);
  signal (SIGINT, handle_signal);

  copy_envp (envp);
  get_path_string (my_envp, path_str);
  insert_path_str_to_search (path_str);

  if ( fork () == 0 )
  {
    execve ("/usr/bin/clear", argv, my_envp);
    exit (1);
  }
  else
  {
    wait (NULL);
  }
  printf ("[MY_SHELL ] ");
  fflush (stdout);
  while ( c != EOF )
  {
    c = getchar ();
    switch ( c )
    {
      case '\n': if ( tmp[0] == '\0' )
      {
        printf ("[MY_SHELL ] ");
      }
                 else
                 {
                   fill_argv (tmp);
                   strncpy (cmd, my_argv[0], strlen (my_argv[0]));
                   strncat (cmd, "\0", 1);
                   if ( index (cmd, '/') == NULL )
                   {
                     if ( attach_path (cmd) == 0 )
                     {
                       call_execve (cmd);
                     }
                     else
                     {
                       printf ("%s: command not found\n", cmd);
                     }
                   }
                   else
                   {
                     if ( (fd = open (cmd, O_RDONLY)) > 0 )
                     {
                       close (fd);
                       call_execve (cmd);
                     }
                     else
                     {
                       printf ("%s: command not found\n", cmd);
                     }
                   }
                   free_argv ();
                   printf ("[MY_SHELL ] ");
                   bzero (cmd, 100);
                 }
                 bzero (tmp, 100);
                 break;
      default: strncat (tmp, &c, 1);
        break;
    }
  }
  free (tmp);
  free (path_str);
  for ( i = 0; my_envp[i] != NULL; i++ )
    free (my_envp[i]);
  for ( i = 0; i<10; i++ )
    free (search_path[i]);
  printf ("\n");
  return 0;
}