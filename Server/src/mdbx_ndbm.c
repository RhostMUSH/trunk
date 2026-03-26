/*
 * mdbx_ndbm.c - NDBM compatibility shim for libmdbx
 *
 * Provides the standard NDBM API (dbm_open, dbm_fetch, dbm_store, etc.)
 * backed by libmdbx.  Used by the chunk layer (udb_achunk.c, udb_ochunk.c)
 * as a drop-in replacement for QDBM's relic.h or GDBM's ndbm.h.
 *
 * Design notes:
 *  - Each DBM handle holds one MDBX environment with a single unnamed DBI.
 *  - MDBX_NOSUBDIR is used so the path maps to a single data file (plus a
 *    lock file), rather than requiring a subdirectory.
 *  - MDBX_NOTLS is used because RhostMUSH is single-threaded.
 *  - dbm_fetch returns a pointer into an internal buffer that is valid until
 *    the next dbm_fetch or dbm_close on the same handle (matches QDBM/GDBM
 *    semantics).
 *  - dbm_firstkey/dbm_nextkey hold an open read transaction + cursor for the
 *    lifetime of the iteration.  A new dbm_firstkey or dbm_close releases
 *    the previous one.
 */

#include "autoconf.h"

#ifdef MDBX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include "../hdrs/mdbx_ndbm.h"

/*
 * Geometry defaults for the MDBX environment.
 * These are intentionally generous — MDBX only allocates pages as needed.
 */
#define MDBX_NDBM_SIZE_LOWER   (1L << 20)        /*   1 MB  minimum */
#define MDBX_NDBM_SIZE_UPPER   (1L << 30)        /*   1 GB  maximum */
#define MDBX_NDBM_GROWTH_STEP  (16L << 20)       /*  16 MB  growth  */
#define MDBX_NDBM_SHRINK_THR   (256L << 20)      /* 256 MB  shrink  */
#define MDBX_NDBM_PAGESIZE     (-1)              /* let mdbx choose */

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * Free and NULL-ify the internal fetch buffer.
 */
static void
free_fetch_buf(DBM *db)
{
    if (db->fetch_buf) {
        free(db->fetch_buf);
        db->fetch_buf = NULL;
        db->fetch_len = 0;
    }
}

/*
 * Release any open iteration state (cursor + read txn).
 */
static void
iter_close(DBM *db)
{
    if (db->iter_cursor) {
        mdbx_cursor_close(db->iter_cursor);
        db->iter_cursor = NULL;
    }
    if (db->iter_txn) {
        mdbx_txn_abort(db->iter_txn);
        db->iter_txn = NULL;
    }
    if (db->iter_buf) {
        free(db->iter_buf);
        db->iter_buf = NULL;
        db->iter_len = 0;
    }
}

/* ------------------------------------------------------------------ */
/* NDBM API                                                            */
/* ------------------------------------------------------------------ */

DBM *
dbm_open(const char *name, int flags, mode_t mode)
{
    DBM *db;
    MDBX_txn *txn;
    unsigned env_flags;
    int rc;

    db = calloc(1, sizeof(*db));
    if (!db)
        return NULL;

    rc = mdbx_env_create(&db->env);
    if (rc != MDBX_SUCCESS) {
        fprintf(stderr, "mdbx_ndbm: mdbx_env_create failed: %s\n",
                mdbx_strerror(rc));
        free(db);
        return NULL;
    }

    /* Single data file, no subdirectory. Single-threaded. */
    env_flags = MDBX_NOSUBDIR | MDBX_NOTLS;
    if ((flags & O_ACCMODE) == O_RDONLY)
        env_flags |= MDBX_RDONLY;

    rc = mdbx_env_set_geometry(db->env,
                               MDBX_NDBM_SIZE_LOWER,  /* lower bound  */
                               -1,                     /* current size */
                               MDBX_NDBM_SIZE_UPPER,  /* upper bound  */
                               MDBX_NDBM_GROWTH_STEP, /* growth step  */
                               MDBX_NDBM_SHRINK_THR,  /* shrink thr   */
                               MDBX_NDBM_PAGESIZE);   /* page size    */
    if (rc != MDBX_SUCCESS) {
        fprintf(stderr, "mdbx_ndbm: mdbx_env_set_geometry failed: %s\n",
                mdbx_strerror(rc));
        mdbx_env_close(db->env);
        free(db);
        return NULL;
    }

    /*
     * MDBX_NOSUBDIR means the path is used directly as the data file name.
     * The chunk layer passes names like "mudDB" or "db_dbm"; MDBX will
     * create "mudDB" (data) and "mudDB-lck" (lock).
     */
    rc = mdbx_env_open(db->env, name, env_flags, mode);
    if (rc != MDBX_SUCCESS) {
        fprintf(stderr, "mdbx_ndbm: mdbx_env_open(\"%s\") failed: %s\n",
                name, mdbx_strerror(rc));
        mdbx_env_close(db->env);
        free(db);
        return NULL;
    }

    /* Open the unnamed (default) DBI.
     * Read-only mode: use a read txn and skip MDBX_CREATE.
     * Read-write mode: use a write txn with MDBX_CREATE.
     */
    {
        unsigned txn_flags = (env_flags & MDBX_RDONLY) ? MDBX_RDONLY : 0;
        unsigned dbi_flags = (env_flags & MDBX_RDONLY) ? 0 : MDBX_CREATE;

        rc = mdbx_txn_begin(db->env, NULL, txn_flags, &txn);
        if (rc != MDBX_SUCCESS) {
            fprintf(stderr, "mdbx_ndbm: mdbx_txn_begin (dbi open) failed: %s\n",
                    mdbx_strerror(rc));
            mdbx_env_close(db->env);
            free(db);
            return NULL;
        }

        rc = mdbx_dbi_open(txn, NULL, dbi_flags, &db->dbi);
    }
    if (rc != MDBX_SUCCESS) {
        fprintf(stderr, "mdbx_ndbm: mdbx_dbi_open failed: %s\n",
                mdbx_strerror(rc));
        mdbx_txn_abort(txn);
        mdbx_env_close(db->env);
        free(db);
        return NULL;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        fprintf(stderr, "mdbx_ndbm: mdbx_txn_commit (dbi open) failed: %s\n",
                mdbx_strerror(rc));
        mdbx_env_close(db->env);
        free(db);
        return NULL;
    }

    return db;
}


void
dbm_close(DBM *db)
{
    if (!db)
        return;

    iter_close(db);
    free_fetch_buf(db);
    mdbx_env_close(db->env);
    free(db);
}


/*
 * Fetch a value by key.
 *
 * Returns a datum pointing to an internal buffer that is valid until the
 * next dbm_fetch or dbm_close on this handle.  Returns { NULL, 0 } on
 * not-found or error.
 */
datum
dbm_fetch(DBM *db, datum key)
{
    datum result = { NULL, 0 };
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    int rc;

    if (!db)
        return result;

    free_fetch_buf(db);

    rc = mdbx_txn_begin(db->env, NULL, MDBX_RDONLY, &txn);
    if (rc != MDBX_SUCCESS)
        return result;

    mkey.iov_base = key.dptr;
    mkey.iov_len  = key.dsize;

    rc = mdbx_get(txn, db->dbi, &mkey, &mval);
    if (rc == MDBX_SUCCESS) {
        /* Copy out — the pointer is only valid within this txn. */
        db->fetch_buf = malloc(mval.iov_len);
        if (db->fetch_buf) {
            memcpy(db->fetch_buf, mval.iov_base, mval.iov_len);
            db->fetch_len  = mval.iov_len;
            result.dptr  = db->fetch_buf;
            result.dsize = mval.iov_len;
        }
    }

    mdbx_txn_abort(txn);
    return result;
}


/*
 * Store a key-value pair.
 *
 * Returns 0 on success, -1 on error, 1 if DBM_INSERT and key exists.
 */
int
dbm_store(DBM *db, datum key, datum val, int flags)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    unsigned put_flags = 0;
    int rc;

    if (!db)
        return -1;

    if (flags == DBM_INSERT)
        put_flags = MDBX_NOOVERWRITE;

    rc = mdbx_txn_begin(db->env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS)
        return -1;

    mkey.iov_base = key.dptr;
    mkey.iov_len  = key.dsize;
    mval.iov_base = val.dptr;
    mval.iov_len  = val.dsize;

    rc = mdbx_put(txn, db->dbi, &mkey, &mval, put_flags);
    if (rc == MDBX_KEYEXIST) {
        mdbx_txn_abort(txn);
        return 1;
    }
    if (rc != MDBX_SUCCESS) {
        mdbx_txn_abort(txn);
        return -1;
    }

    rc = mdbx_txn_commit(txn);
    return (rc == MDBX_SUCCESS) ? 0 : -1;
}


/*
 * Delete a key.
 *
 * Returns 0 on success, -1 on error or not-found.
 */
int
dbm_delete(DBM *db, datum key)
{
    MDBX_txn *txn;
    MDBX_val mkey;
    int rc;

    if (!db)
        return -1;

    rc = mdbx_txn_begin(db->env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS)
        return -1;

    mkey.iov_base = key.dptr;
    mkey.iov_len  = key.dsize;

    rc = mdbx_del(txn, db->dbi, &mkey, NULL);
    if (rc != MDBX_SUCCESS) {
        mdbx_txn_abort(txn);
        return -1;
    }

    rc = mdbx_txn_commit(txn);
    return (rc == MDBX_SUCCESS) ? 0 : -1;
}


/*
 * Begin iteration.  Returns the first key in the database.
 *
 * Opens a read-only txn + cursor that is held open until the next
 * dbm_firstkey or dbm_close.
 */
datum
dbm_firstkey(DBM *db)
{
    datum result = { NULL, 0 };
    MDBX_val mkey, mval;
    int rc;

    if (!db)
        return result;

    /* Release any prior iteration. */
    iter_close(db);

    rc = mdbx_txn_begin(db->env, NULL, MDBX_RDONLY, &db->iter_txn);
    if (rc != MDBX_SUCCESS)
        return result;

    rc = mdbx_cursor_open(db->iter_txn, db->dbi, &db->iter_cursor);
    if (rc != MDBX_SUCCESS) {
        mdbx_txn_abort(db->iter_txn);
        db->iter_txn = NULL;
        return result;
    }

    rc = mdbx_cursor_get(db->iter_cursor, &mkey, &mval, MDBX_FIRST);
    if (rc != MDBX_SUCCESS) {
        iter_close(db);
        return result;
    }

    /* Copy the key into a buffer that survives until the next iteration call. */
    db->iter_buf = malloc(mkey.iov_len);
    if (db->iter_buf) {
        memcpy(db->iter_buf, mkey.iov_base, mkey.iov_len);
        db->iter_len  = mkey.iov_len;
        result.dptr  = db->iter_buf;
        result.dsize = mkey.iov_len;
    }

    return result;
}


/*
 * Continue iteration.  Returns the next key.
 *
 * Returns { NULL, 0 } when there are no more keys, and releases the
 * iteration state.
 */
datum
dbm_nextkey(DBM *db)
{
    datum result = { NULL, 0 };
    MDBX_val mkey, mval;
    int rc;

    if (!db || !db->iter_cursor)
        return result;

    /* Free the previous key buffer. */
    if (db->iter_buf) {
        free(db->iter_buf);
        db->iter_buf = NULL;
        db->iter_len = 0;
    }

    rc = mdbx_cursor_get(db->iter_cursor, &mkey, &mval, MDBX_NEXT);
    if (rc != MDBX_SUCCESS) {
        iter_close(db);
        return result;
    }

    db->iter_buf = malloc(mkey.iov_len);
    if (db->iter_buf) {
        memcpy(db->iter_buf, mkey.iov_base, mkey.iov_len);
        db->iter_len  = mkey.iov_len;
        result.dptr  = db->iter_buf;
        result.dsize = mkey.iov_len;
    }

    return result;
}

#endif /* MDBX */
