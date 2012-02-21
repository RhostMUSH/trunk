#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef SYSWAIT
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <autoconf.h>

#include "debug.h"

char *programname;

void printdebug( void )
{
  int idx;

  fprintf(stderr, "%s: debug stack depth = %d\n", programname,
          debugmem->stacktop);
  
  for( idx = 0; idx < debugmem->stacktop; idx++) {
      fprintf(stderr, "%s: stackframe %d = %s, line %d\n", programname,
              idx, aDebugFilenameArray[debugmem->callstack[idx].filenum], 
              debugmem->callstack[idx].linenum);
  }

  fprintf(stderr, "\n%s: last db fetch was for #%d\n", programname,
          debugmem->lastdbfetch);
}

void handle_usr1(int sig)
{
   fprintf(stderr, "\n%s: caught SIGUSR1\n", programname);
   printdebug();

   signal(SIGUSR1, handle_usr1);
}


int main( int argc, char *argv[] )
{
  int status;
  int pid;
  int debug_id;
  int shmid;

  int foo;

  programname = argv[0];

  if( argc < 3 ) {
    printf("usage: %s <shared mem key> <prog name>\n", programname);
    exit(1);
  }

  foo = sscanf(argv[1], "%d", &debug_id);
  if ( foo < 0) {
    fprintf(stderr, "** debug_id must be an integer, not '%s' **\n", argv[1]);
    exit(1);
  }

  debugmem = shmConnect(debug_id, 1, &shmid);
  debugmem->debugport = getpid();
  debugmem->mushport = -1;
  fprintf(stderr, "Master IPC Debugging stack created w/ key = %d [shared memory id: %d]\n", 
          debug_id, shmid);

  INITDEBUG(debugmem);

  /* ok, debugging stack is set up in shared memory, go ahead and launch
     the program that we're monitoring */

  fprintf(stderr, "%s: initiating %s\n", programname, argv[2]);

  if( (pid = fork()) < 0 ) {
    perror("fork");
    exit(1);
  }

  if( pid == 0 ) {
    /* child code */
    execv(argv[2], argv + 2);

    perror("execv");
    exit(1);
  }

  /* parent code */

  signal(SIGUSR1, handle_usr1);

  while( wait(&status) != pid ) {
    /* wait */
  }

  fprintf(stderr, "%s: %s exited with status %d\n", programname, 
                  argv[2], status);

  printdebug();

#ifdef SOLARIS
  shmdt((char *)debugmem);
#else
  shmdt((const void *)debugmem);
#endif
  shmctl(shmid, IPC_RMID, NULL);
  fprintf(stderr, "%s: exiting\n", programname);
  
  return status;
}
