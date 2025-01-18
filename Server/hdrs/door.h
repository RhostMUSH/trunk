#ifndef _M_DOOR_H_
#define _M_DOOR_H_

/* The function pointer definitions for the door code */
typedef int (*doorInit_t)   (void);
typedef int (*doorOpen_t)   (DESC *d, int nargs, char *cargs[], int id);
typedef int (*doorClose_t)  (DESC *d);
typedef int (*doorOutput_t) (DESC *d, char *pText);
typedef int (*doorInput_t)  (DESC *d, char *pText);

/* A couple of things used for housekeeping and other such
 * fun stuff
 */

typedef enum _doorStatus {
  ONLINE_e, OFFLINE_e, OPEN_e, CLOSED_e, INTERNAL_e, SERVER_e
} doorStatus_t;

typedef enum _doorEnactor {
  PLAYER_e = -1, ROOM_e = -2, EXIT_e = -3, dINTERNAL_e = -4
} doorEnactor_t;

/* The door code will store valid doors in an array of the following: */

typedef struct _door {
  char *pName;
  int  switchNum;
  doorInit_t   pFnInitDoor;
  doorOpen_t   pFnOpenDoor;
  doorClose_t  pFnCloseDoor;
  doorInput_t  pFnReadDoor;
  doorOutput_t pFnWriteDoor;
  doorStatus_t doorStatus;
  int          nConnections;
  int          locTrig;
  int          bittype;
  DESC         *pDescriptor;
} door_t;

/* Public prototypes */
extern void door_raw_input(DESC *, char *);
extern void door_raw_output(DESC *, char *);
extern int door_tcp_connect(char *, char *, DESC *, int, int);
extern int door_connect(char *, char *, DESC *, int);
extern int process_door_output(DESC *);
extern int process_door_input(DESC *);
extern void save_door(DESC *, char *);
extern void initDoorSystem(void);
extern void modifyDoorStatus(dbref, char *, char *);
extern void openDoor(dbref, dbref, char *, int, char *[], int);
extern void closeDoor(DESC *, char *);
extern void closeDoorWithId(DESC *, int);
extern void listDoors(dbref, char *, int full);
extern const char * returnDoorName(int d);
extern void queue_door_string(DESC *, const char *, int);
extern void queue_door_write(DESC *, const char *, int);
extern void door_registerInternalDoorDescriptors(fd_set *, fd_set *, int *);
extern void door_checkInternalDoorDescriptors(fd_set *, fd_set *);
extern void door_processInternalDoors(void);
extern int process_output(DESC * d);
#endif

