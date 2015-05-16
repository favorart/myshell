#include "myshell.h"


//----------------------------------------------------------------------------------
int  main (int argc, char** argv)
{
  sigint_disable ();
  //------------------------------------
  std::string  command_line;

  std::list<std::string> cmds;
  std::list<pid_t> backgrounds;

#ifdef _DEBUG
  std::fstream fin ("/home/kill/ts_k.golikov/p4/4-shell/tests/6.sh");
  while ( std::getline (fin, command_line, '\n') ) 
#else // end _DEBUG
  // while ( !std::cin.eof() )
  while ( std::getline (std::cin, command_line, '\n') )
#endif 
  {

#ifdef _DEBUG
    std::cerr  << "debug: " << command_line;
    std::cerr.flush ();
#else // end _DEBUG
    if ( isatty (fileno (stdout)) )
    {
      std::cout << "myshell: ";
      std::cout.flush ();
    }
#endif

    myshell_cmd_parse (cmds, command_line);

    if ( cmds.size () <= 0 )
      continue;

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
        cmds.clear ();
      }
      else
      {
        //------------------------------------
        /* child code */
        myshell_execution (cmds, backgrounds);
        return EXIT_SUCCESS;
      }
    } // end if &
    else
    {
      //------------------------------------
      myshell_execution (cmds, backgrounds);
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
        std::cerr << "Process " << pid << " exited: "<< ret << std::endl;
      }
      // else /* not yet */
    }
  }
  return EXIT_SUCCESS;
}
//----------------------------------------------------------------------------------
