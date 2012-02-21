#include <stdlib.h>
#define __DEBUGMON_INCLUDE__ 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <autoconf.h>
#include "debug.h"

Debugmem *debugmem = NULL;

Debugmem * shmConnect(int debug_id, int bCreate, int *pShmid) {
  char *shm = NULL, *env_var = NULL;
  int exitCode = 0;
  int mask = 0700;

  if (bCreate) mask |= IPC_CREAT | IPC_EXCL;

#ifdef DEBUG
  fprintf(stderr, "** %s shared memory segment on debug_id '%d' **\n",
	  (bCreate ? "Creating" : "Connecting to"), debug_id);
  fprintf(stderr, "** Parent PID <%d> : Child PID <%d> **\n", getppid(), getpid());
#endif

  (*pShmid) = shmget(debug_id, sizeof(Debugmem), mask );
  env_var = getenv("DEBUG_OVERRIDE");
  if ( ((*pShmid) < 0) && (env_var != NULL) && (strcmp(env_var, "1") == 0) ) {
     (*pShmid) = shmget(debug_id, sizeof(Debugmem), (0700 | IPC_CREAT) );
     if ( (*pShmid) < 0 ) {
        fprintf(stderr, "** Warning - debug_id '%d' not able to be attached to ** \n", debug_id);
     } else {
        exitCode = shmctl((*pShmid), IPC_RMID, NULL);
        if ( exitCode < 0 )
           fprintf(stderr, "** Warning - debug_id '%d' was unable to be cleared **\n", debug_id);
        else
           fprintf(stderr, "** Warning - debug_id '%d' overridden and cleared **\n", debug_id);
        exitCode = 0;
     }
     (*pShmid) = shmget(debug_id, sizeof(Debugmem), mask );
  }
  if ((*pShmid) < 0 ) {
    if (errno == EEXIST) {
      fprintf(stderr, 
	      "** Error - possible clash detected for debug_id **\n"
	      "** Shared memory region already exists for '%d' **\n", debug_id);
    } else if (errno == ENOENT) {
      fprintf(stderr,
	      "** No shared memory region could be found for debug_id '%d'**\n",
	      debug_id);
    } else {
      perror("** IPC Error (shmget)");
      if (!bCreate) fprintf(stderr, "** Try changing debug_id? ** \n");
    }
    shm = NULL;
    exitCode = 1;
  } else {
    if((shm = shmat((*pShmid), NULL, 0)) == (char *) -1) {
      perror("** IPC Error (shmat)");
      shmdt(shm);
      shmctl((*pShmid), IPC_RMID, NULL);
      shm = NULL;
      exitCode = 2;
    }
  }

  if (shm || !bCreate) {
    return (Debugmem *) shm;
  } else {
    exit(exitCode);
  }
}

