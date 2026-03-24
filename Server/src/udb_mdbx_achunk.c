/*
 * udb_mdbx_achunk.c - MDBX-backed attribute chunk storage
 *
 * Replaces udb_achunk.c when MDBX is selected as the backend.
 * Stores attribute data directly in MDBX as key-value pairs,
 * eliminating the bitmap allocator and flat .db file.
 *
 * Key:   Aname struct (8 bytes: object + attrnum)
 * Value: raw attribute data (NUL-terminated string)
 */

#include "autoconf.h"
#include "udb.h"
#include "udb_defs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/mdbx/mdbx.h"

extern void VDECL(fatal, (const char *,...));
extern void mush_logf();
extern void FDECL(log_db_err, (int, int, const char *));

/* Default database name */
#define DEFAULT_DBMCHUNKFILE "db_dbm"

/* Geometry defaults */
#define ACHUNK_SIZE_LOWER   (1L << 20)        /*   1 MB  */
#define ACHUNK_SIZE_UPPER   (2L << 30)        /*   2 GB  */
#define ACHUNK_GROWTH_STEP  (16L << 20)       /*  16 MB  */
#define ACHUNK_SHRINK_THR   (256L << 20)      /* 256 MB  */

static const char *dbfile = DEFAULT_DBMCHUNKFILE;
static int         db_initted = 0;

static MDBX_env   *env = NULL;
static MDBX_dbi    dbi;

int
dddb_init()
{
    char fnam[1024];
    MDBX_txn *txn;
    int rc;

    rc = mdbx_env_create(&env);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_create failed\n", NULL);
        return 1;
    }

    rc = mdbx_env_set_geometry(env,
                               ACHUNK_SIZE_LOWER, -1, ACHUNK_SIZE_UPPER,
                               ACHUNK_GROWTH_STEP, ACHUNK_SHRINK_THR, -1);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_set_geometry failed\n", NULL);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    /* Use the dbfile name directly with MDBX_NOSUBDIR.
     * Append .mdbx to distinguish from legacy .db/.dir/.pag files.
     */
    snprintf(fnam, sizeof(fnam), "%.250s.mdbx", dbfile);

    rc = mdbx_env_open(env, fnam, MDBX_NOSUBDIR | MDBX_NOTLS, 0600);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_open failed for ", fnam, "\n", NULL);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    /* Open the unnamed DBI */
    rc = mdbx_txn_begin(env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_txn_begin failed\n", NULL);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    rc = mdbx_dbi_open(txn, NULL, MDBX_CREATE, &dbi);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_dbi_open failed\n", NULL);
        mdbx_txn_abort(txn);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_txn_commit failed\n", NULL);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    db_initted = 1;
    return 0;
}


int dddb_initted()
{
    return db_initted;
}


int dddb_setbsiz(int nbsiz)
{
    /* Block size is not relevant for MDBX — ignored. */
    return nbsiz;
}


int dddb_setfile(char *fil)
{
    char *xp;

    if (db_initted)
        return 1;

    xp = (char *)malloc((unsigned)strlen(fil) + 1);
    if (xp == NULL)
        return 1;
    strcpy(xp, fil);
    dbfile = xp;
    return 0;
}


int
dddb_close()
{
    if (env != NULL) {
        mdbx_env_close(env);
        env = NULL;
    }
    db_initted = 0;
    return 0;
}


Attr *
dddb_get(Aname *nam)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    Attr *ret;
    int rc;

    if (!db_initted)
        return ANULL;

    rc = mdbx_txn_begin(env, NULL, MDBX_RDONLY, &txn);
    if (rc != MDBX_SUCCESS)
        return ANULL;

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);

    rc = mdbx_get(txn, dbi, &mkey, &mval);
    if (rc != MDBX_SUCCESS) {
        mdbx_txn_abort(txn);
        return ANULL;
    }

    /* Copy the data out — the pointer is only valid within the txn. */
    ret = (Attr *)malloc(mval.iov_len + 1);
    if (ret == ANULL) {
        mdbx_txn_abort(txn);
        return ANULL;
    }
    memcpy(ret, mval.iov_base, mval.iov_len);
    ret[mval.iov_len] = '\0';

    mdbx_txn_abort(txn);
    return ret;
}


int
dddb_put(Attr *obj, Aname *nam)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    int nsiz;
    int rc;

    if (!db_initted)
        return 1;

    nsiz = strlen(obj);

    rc = mdbx_txn_begin(env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Put) mdbx_txn_begin");
        return 1;
    }

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);
    mval.iov_base = (void *)obj;
    mval.iov_len  = nsiz;

    rc = mdbx_put(txn, dbi, &mkey, &mval, 0);
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Put) mdbx_put");
        mdbx_txn_abort(txn);
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Put) mdbx_txn_commit");
        return 1;
    }

    return 0;
}


int
dddb_check(Aname *nam)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    int rc;

    if (!db_initted)
        return 0;

    rc = mdbx_txn_begin(env, NULL, MDBX_RDONLY, &txn);
    if (rc != MDBX_SUCCESS)
        return 0;

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);

    rc = mdbx_get(txn, dbi, &mkey, &mval);
    mdbx_txn_abort(txn);

    return (rc == MDBX_SUCCESS) ? 1 : 0;
}


int
dddb_del(Aname *nam, int flg)
{
    MDBX_txn *txn;
    MDBX_val mkey;
    int rc;

    if (!db_initted)
        return -1;

    rc = mdbx_txn_begin(env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS)
        return -1;

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);

    rc = mdbx_del(txn, dbi, &mkey, NULL);
    if (rc == MDBX_NOTFOUND) {
        mdbx_txn_abort(txn);
        return 0;
    }
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Del) mdbx_del");
        mdbx_txn_abort(txn);
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Del) mdbx_txn_commit");
        return 1;
    }

    return 0;
}
