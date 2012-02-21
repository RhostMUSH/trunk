#ifndef _M_DOOR_PROTOTYPES_
#define _M_DOOR_PROTOTYPES_

/**********************************************************************
 * DOOR_PROTOTYPES.H                                    Lensy, 12/04/03
 *
 * This file is where you should stick any function prototypes for 
 * door implementations you want to use.
 *
 */

#ifdef EXAMPLE_DOOR_CODE
/* These prototypes are for the example door implementations 
 * that ship with Rhost pl15.
 * See the README.DOOR file for in depth information.
 */

/* The check email door (door_mail.c) */
int mailDoorOutput(DESC *d, char *pText);
int mailDoorOpen(DESC *d, int nArgs, char *args[], int id);
int mailDoorInput(DESC *d, char *pText);
int mailDoorClose(DESC *d);
/* The mush connection door (door_mush.c) */ 
int mushDoorOpen(DESC *d, int nArgs, char *args[], int id);
/* example code: END */
#endif

int testDoorOpen(DESC *d, int nArgs, char *args[], int id);
int testDoorOutput(DESC *d, char *pText);
int testDoorInput(DESC *d, char *pText);
int testDoorInit(void);
int testDoorClose(void);

int empire_init(DESC *d, int nargs, char *args[], int id);
int empire_from_empsrv(DESC *d, char *text);
#endif
