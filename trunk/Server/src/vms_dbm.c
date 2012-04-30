/*
	Copyright (C) 1991, Andrew Molitor. All rights reserved.
*/
/*
#define DEBUG
*/
/*
	This a a dbm emulation module for VMS, using indexed files to do
the database thang. It emulates new dbm.

*/

#include	"autoconf.h"
#include	<ssdef.h>
#include	<rms.h>

#include	"vms_dbm.h"

static char	dbmbuff[MAX_RECORD];


DBM *
dbm_open(char *filename, int flags, int mode)
{
	DBM	*newdb;
	char	*name;
	int	status;

	/* Make a new DBM struct */
#ifdef DEBUG
	printf("Opening DB file %s\n",filename);
#endif
	newdb = (DBM *) malloc(sizeof(struct rmsblocks));
	if(newdb == (DBM *)0){
		return((DBM *)0);
	}

	name = (char *) malloc(strlen(filename)+1);
	if(name == (char *)0){
		free((char *)newdb);
		return((DBM *)0);
	}
	strcpy(name,filename);

	/* Fill it in with stuff */

	/* FAB */

	newdb->fab = cc$rms_fab;
	(newdb->fab).fab$l_fna = name;
	(newdb->fab).fab$l_xab = &(newdb->xab);
	(newdb->fab).fab$b_fns = strlen(name);
	(newdb->fab).fab$b_bks = BUCKET_SIZE;
	(newdb->fab).fab$b_org = FAB$C_IDX;
	(newdb->fab).fab$w_mrs = MAX_RECORD;
	(newdb->fab).fab$b_rfm = FAB$C_VAR;
	(newdb->fab).fab$l_fop = FAB$M_CIF;
	(newdb->fab).fab$b_fac = FAB$M_PUT|FAB$M_GET|FAB$M_UPD|FAB$M_DEL;

	/* RAB */

	newdb->rab = cc$rms_rab;
	(newdb->rab).rab$l_fab = &(newdb->fab);
	(newdb->rab).rab$b_rac = RAB$C_KEY;
	(newdb->rab).rab$l_rop = RAB$V_UIF | RAB$V_LIM;
	(newdb->rab).rab$l_ubf = dbmbuff;
	(newdb->rab).rab$w_usz = MAX_RECORD;

	/* XAB */

	newdb->xab = cc$rms_xabkey;
	(newdb->xab).xab$b_dtp = XAB$C_STG;
  	(newdb->xab).xab$w_pos = 0;
	(newdb->xab).xab$b_siz = KEY_SIZE;
	(newdb->xab).xab$b_ref = 0;

	/* Try to open it */

	status = sys$create(&(newdb->fab));
	if((status & 1) == 0){ /* Failed open, try to create it */
#ifdef DEBUG
		printf("$open failed, status %d.\n",status);
#endif
		goto bag;
	}

	/* We're poppin'. Connect the rab up. */

	status = sys$connect(&(newdb->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$connect failed, status %d\n",status);
#endif
		goto bag;
	}

	/* Ready to go, we hope */
#ifdef DEBUG
	printf("Succeeded.\n");
#endif
	return(newdb);
bag:
	free((char *)newdb);
	free((char *)name);
	return((DBM *)0);
}

dbm_close(DBM *db)
{
	sys$close(&(db->fab));
	free((db->fab).fab$l_fna);
	free((char *)db);
}

datum
dbm_fetch(DBM *db, datum key)
{
	int	status;
	int	i;
	datum	answer;
	char	keybuff[KEY_SIZE];
	char	*p,*s;

	/* Fill in the key part of the rab */

	p = keybuff; s = key.dptr;
	for(i = 0; i < KEY_SIZE && i < key.dsize; i++)
		*p++ = *s++;

	while(i++ < KEY_SIZE)
		*p++ = '\0';

	(db->rab).rab$l_kbf = keybuff;
	(db->rab).rab$b_ksz = KEY_SIZE;

	/* Now go get it */

	status = sys$get(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$get failed, status %d\n",status);
#endif
		goto bag;
	}

	answer.dsize = (db->rab).rab$w_rsz - KEY_SIZE;
	answer.dptr = malloc(answer.dsize);
	if(answer.dptr == (char *)0){
		goto bag;
	}
	bcopy((db->rab).rab$l_rbf+KEY_SIZE, answer.dptr, answer.dsize);
	return(answer);
bag:
	answer.dptr = (char *)0;
	answer.dsize = 0;
	return(answer);
}

dbm_store(DBM *db,datum key,datum content,int flags)
{
	int	status;
	int	i;
	char	*p,*s;

	/* Build the record */
#ifdef DEBUG
	printf("dbm_store: data size = %d\n",content.dsize);
#endif
	/* Fill in KEY_SIZE bytes of key */

	p = dbmbuff;s = key.dptr;
	for(i = 0; i < key.dsize && i < KEY_SIZE; i++)
		*p++ = *s++;
	while(i++ < KEY_SIZE)
		*p++ = '\0';

	bcopy(content.dptr, dbmbuff+KEY_SIZE, content.dsize);
	(db->rab).rab$l_rbf = dbmbuff;
	(db->rab).rab$w_rsz = content.dsize + KEY_SIZE;

	(db->rab).rab$l_kbf = dbmbuff;
	(db->rab).rab$b_ksz = KEY_SIZE;
#ifdef DEBUG
	printf("Searching for record %s, size = %d\n",
		dbmbuff,(db->rab).rab$w_rsz);
#endif
	status = sys$find(&(db->rab));
	if(status == RMS$_NORMAL){
		status = sys$update(&(db->rab));
	} else if(status == RMS$_RNF){
#ifdef DEBUG
		printf("Record not found. Trying $put %d\n",status);
#endif
		status = sys$put(&(db->rab));
	}
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$update/$put failed. status code %d\n",status);
#endif
		return(1);
	}
	return(0);
}

dbm_delete(DBM *db, datum key)
{
	int	status;
	int	i;
	datum	answer;
	char	keybuff[KEY_SIZE];
	char	*p,*s;

	/* Fill in the key part of the rab */

	p = keybuff; s = key.dptr;
	for(i = 0; i < KEY_SIZE && i < key.dsize; i++)
		*p++ = *s++;

	while(i++ < KEY_SIZE)
		*p++ = '\0';

	(db->rab).rab$l_kbf = keybuff;
	(db->rab).rab$b_ksz = KEY_SIZE;

	/* Now go KILL it */

	status = sys$find(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$find in delete() failed, status %d\n",status);
#endif
		return(1);
	}
	status = sys$delete(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$delete failed, status %d\n",status);
#endif
		return(1);
	}
	return(0);
}

datum
dbm_firstkey(DBM *db)
{
	int	status;
	datum	answer;

	status = sys$rewind(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$rewind failed, status == %d\n",status);
#endif
		goto bag;
	}
	/* Set the file to sequential access */

	(db->rab).rab$b_rac = RAB$C_SEQ;
	status = sys$get(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$get in firstkey() failed. status = %d\n",status);
#endif
		goto bag;
	}
	/* Save the key away */
	bcopy((db->rab).rab$l_rbf,db->currkey,KEY_SIZE);

	/* Put the file back to random access. */

	(db->rab).rab$b_rac = RAB$C_KEY;

	answer.dptr = db->currkey;
	answer.dsize = strlen(db->currkey);
	return(answer);
bag:
	(db->rab).rab$b_rac = RAB$C_KEY;
	answer.dptr = (char *)0;
	answer.dsize = 0;
	return(answer);
}

datum
dbm_nextkey(DBM *db)
{
	int status;
	datum answer;

	(db->rab).rab$l_kbf = db->currkey;
	(db->rab).rab$b_ksz = KEY_SIZE;

	/* Seek over to the current record */

	status = sys$find(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$find in nextkey() failed. Status = %d\n",status);
#endif
		goto bag;
	}

	/* set file sequential */
	(db->rab).rab$b_rac = RAB$C_SEQ;

	/* Skip one */
	status = sys$find(&(db->rab));
	if(status == RMS$_EOF){ /* We're done! */
#ifdef DEBUG
		printf("EOF detected in nextkey.\n");
#endif
		status = sys$rewind(&(db->rab));
#ifdef DEBUG
		if((status & 1) == 0){
			printf("$rewind at EOF, in nextkey(), failed. status = %d\n",status);
		}
#endif
		goto bag;
	}
	status = sys$get(&(db->rab));
	if((status & 1) == 0){
#ifdef DEBUG
		printf("$get in nextkey() failed. status = %d\n",status);
#endif
		goto bag;
	}
	/* Save the key away */
	bcopy((db->rab).rab$l_rbf,db->currkey,KEY_SIZE);

	/* Put the file back to random access. */
	(db->rab).rab$b_rac = RAB$C_KEY;

	answer.dptr = db->currkey;
	answer.dsize = strlen(db->currkey);
	return(answer);
bag:
	answer.dptr = (char *)0;
	answer.dsize = 0;
	/* Put the file back to random access. */
	(db->rab).rab$b_rac = RAB$C_KEY;
	return(answer);
}
