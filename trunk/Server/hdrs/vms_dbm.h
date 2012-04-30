#ifndef _M_VMS_DBM_H_
#define _M_VMS_DBM_H_

/*
	Structure definitions to hold a dbm database.
*/

#include <rms.h>

/*
	VMS deals poorly with variable length keys, so we use fixed length ones 
made by padding the small variable length ones out. KEY_SIZE should, therefore,
be large enough to hold the largest key you plan to use. Add that to
the size of the largest datum you will associate with a key to compute
MAX_RECORD.

	In this application, the data stored are all 8 bytes.

*/

#define MAX_RECORD	32
#define KEY_SIZE	24

/*
	This should be 2 or 3 times MAX_RECORD, at least, but counted in
blocks. Here, one block is generally fine.

*/

#define BUCKET_SIZE	1


typedef struct rmsblocks {
	struct	FAB	fab;
	struct	RAB	rab;
	struct	XABKEY	xab;
	char	currkey[KEY_SIZE];
} DBM;


/* Flags for dbm. Ignored by this package, incidentally. */

#define DBM_REPLACE	0

typedef struct {
	char	*dptr;
	int	dsize;
} datum;

extern	DBM	*dbm_open();
extern	int	dbm_close();
extern	datum	dbm_fetch();
extern	int	dbm_store();
extern	int	dbm_delete();
extern	datum	dbm_firstkey();
extern	datum	dbm_nextkey();

#endif
