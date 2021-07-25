#ifndef _M_LOCAL_H_
#define _M_LOCAL_H_

/******************************************************************
 *
 *                  LOCAL CALLBACK FUNCTIONS
 *
 * Author  : Lensman
 *
 * Purpose : People writing 3rd party code for Rhost may need to
 *           insert hooks into parts of the source such that funcs
 *           within their code get called at the appropriate time.
 *           This is to help :)
 *
 * V1
 *******************************************************************/

void local_startup(void);
void local_shutdown(void);
void local_dump_reboot(void);
void local_load_reboot(void);
void local_dump(int bPanicDump);
void local_dbck(void);
void local_tick(void);
void local_tick(void);
void local_player_connect(dbref player, int bNew);
void local_player_disconnect(dbref player);
void local_second(void);
#endif
