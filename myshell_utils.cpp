#include "myshell.h"


//----------------------------------------------------------------------------------
volatile sig_atomic_t  sigint_flag;
volatile pid_t         pid_mychild; // можно ли как-то передать pid в обработчик?
//----------------------------------------------------------------------------------
void  sigint_handle (int sig)
{
  sigint_flag = true;
  // printf ("proc interrapted\n");
  kill (pid_mychild, SIGINT);
}
//----------------------------------------------------------------------------------
void  sigint_enable  (pid_t child_pid)
{ struct sigaction sa;
  sigset_t sigintset;

  pid_mychild = child_pid;
  //------------------------------------
  /* append signal handler */
  sa.sa_handler = sigint_handle;
  sigaction (SIGINT, &sa, 0);
  //------------------------------------
  /* Stop blocking the signal */
  sigemptyset (&sa.sa_mask);
  sigaddset   (&sigintset, SIGINT);
  sigprocmask (SIG_UNBLOCK, &sa.sa_mask, 0);
  //------------------------------------
}
void  sigint_disable (void)
{ struct sigaction sa;
  sigset_t sigintset;

  pid_mychild = 0;
  //------------------------------------
  /* remove signal handler */
  sa.sa_handler = SIG_DFL;
  sigaction (SIGINT, &sa, 0);
  //------------------------------------
  /* Blocking the signal */
  sigemptyset (&sigintset);
  sigaddset   (&sigintset, SIGINT);
  sigprocmask (SIG_BLOCK, &sigintset, 0);
  //------------------------------------
}
//----------------------------------------------------------------------------------
int   set_cloexec_flag (int desc, bool set)
{
  int oldflags = fcntl (desc, F_GETFD, 0);
  /* If reading the flags failed, return error indication now. */
  if ( oldflags < 0 )
    return oldflags;
  /* Set just the flag we want to set. */
  if ( set )
    oldflags |= FD_CLOEXEC;
  else
    oldflags &= ~FD_CLOEXEC;
  /* Store modified flag word in the descriptor. */
  return  fcntl (desc, F_SETFD, oldflags);
}
//----------------------------------------------------------------------------------
