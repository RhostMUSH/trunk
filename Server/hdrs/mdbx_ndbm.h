/*
 * mdbx_ndbm.h - NDBM compatibility shim for libmdbx
 *
 * Provides the standard NDBM API (dbm_open, dbm_fetch, dbm_store, etc.)
 * backed by libmdbx. This header is included via redirect_ndbm.h when
 * MDBX is selected as the database backend.
 */

#ifndef MDBX_NDBM_H
#define MDBX_NDBM_H

#include <sys/types.h>
#include "../src/mdbx/mdbx.h"

/* NDBM datum type */
typedef struct {
    void  *dptr;
    size_t dsize;
} datum;

/* Opaque handle wrapping mdbx state */
typedef struct {
    MDBX_env    *env;
    MDBX_dbi     dbi;
    /* Iteration state (held open between firstkey/nextkey calls) */
    MDBX_cursor *iter_cursor;
    MDBX_txn    *iter_txn;
    char        *iter_buf;      /* copied key for current iteration step */
    size_t       iter_len;
    /* Fetch buffer (valid until next fetch or close) */
    char        *fetch_buf;
    size_t       fetch_len;
} DBM;

/* Store flags */
#define DBM_INSERT   0
#define DBM_REPLACE  1

/* Standard NDBM API */
extern DBM   *dbm_open(const char *name, int flags, mode_t mode);
extern void   dbm_close(DBM *db);
extern datum  dbm_fetch(DBM *db, datum key);
extern int    dbm_store(DBM *db, datum key, datum val, int flags);
extern int    dbm_delete(DBM *db, datum key);
extern datum  dbm_firstkey(DBM *db);
extern datum  dbm_nextkey(DBM *db);

#endif /* MDBX_NDBM_H */
