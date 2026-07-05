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
extern void mush_logf(const char *fmt, ...);
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
static int         bsiz = 256;
static MDBX_txn   *active_txn = NULL;
static MDBX_txn   *read_txn = NULL;    /* persistent read txn for main loop */

/* Lightweight read cache: accelerates repeated attribute lookups within
 * the same read txn by caching B-tree leaf page IDs.
 * 1000 entries × 40 bytes = 40 KB. */
#define READ_CACHE_SIZE 1000

typedef struct {
    Aname              key;
    MDBX_cache_entry_t entry;
} ReadCacheSlot;

static ReadCacheSlot read_cache[READ_CACHE_SIZE];

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

    rc = mdbx_env_open(env, fnam, MDBX_NOSUBDIR | MDBX_NOSTICKYTHREADS, 0600);
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
    memset(read_cache, 0, sizeof(read_cache));
    return 0;
}


int
dddb_txn_begin()
{
    int rc;

    if (!db_initted)
        return 1;

    if (active_txn)
        return 1;

    rc = mdbx_txn_begin(env, NULL, 0, &active_txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_txn_begin: mdbx_txn_begin failed\n", NULL);
        active_txn = NULL;
        return 1;
    }

    return 0;
}


int
dddb_txn_commit()
{
    int rc;

    if (!active_txn)
        return 0;

    rc = mdbx_txn_commit(active_txn);
    active_txn = NULL;
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_txn_commit: mdbx_txn_commit failed\n", NULL);
        return 1;
    }

    return 0;
}


int
dddb_txn_abort()
{
    if (!active_txn)
        return 0;

    mdbx_txn_abort(active_txn);
    active_txn = NULL;
    return 0;
}


int
dddb_read_begin()
{
    if (!db_initted)
        return 1;
    if (read_txn)
        return 0;
    memset(read_cache, 0, sizeof(read_cache));
    return mdbx_txn_begin(env, NULL, MDBX_TXN_RDONLY, &read_txn);
}

int
dddb_read_end()
{
    if (!read_txn)
        return 0;
    int rc = mdbx_txn_abort(read_txn);
    read_txn = NULL;
    return rc;
}

int
dddb_get_is_active_read_txn()
{
    return read_txn != NULL;
}

int dddb_initted()
{
    return db_initted;
}

void
dddb_var_init()
{
    db_initted = 0;
    bsiz = 256;
    active_txn = NULL;
    read_txn = NULL;
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
    if (active_txn) {
        mdbx_txn_abort(active_txn);
        active_txn = NULL;
    }
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
    int local_txn = 0;

    if (!db_initted)
        return ANULL;

    if (read_txn) {
        txn = read_txn;

        /* Hash the key to find a read cache slot */
        unsigned int idx = (nam->object ^ nam->attrnum) % READ_CACHE_SIZE;
        ReadCacheSlot *slot = &read_cache[idx];

        mkey.iov_base = (void *)nam;
        mkey.iov_len  = sizeof(Aname);

        if (slot->entry.trunk_txnid != 0 &&
            slot->key.object == nam->object &&
            slot->key.attrnum == nam->attrnum) {
            /* Existing slot for this key — use cached B-tree info */
            MDBX_cache_result_t cres = mdbx_cache_get(txn, dbi, &mkey, &mval, &slot->entry);
            rc = (int)cres.errcode;
        } else {
            /* New key — initialize cache entry */
            slot->key = *nam;
            mdbx_cache_init(&slot->entry);
            MDBX_cache_result_t cres = mdbx_cache_get(txn, dbi, &mkey, &mval, &slot->entry);
            rc = (int)cres.errcode;
        }
    } else {
        rc = mdbx_txn_begin(env, NULL, MDBX_TXN_RDONLY, &txn);
        if (rc != MDBX_SUCCESS)
            return ANULL;
        local_txn = 1;

        mkey.iov_base = (void *)nam;
        mkey.iov_len  = sizeof(Aname);

        rc = mdbx_get(txn, dbi, &mkey, &mval);
    }
    if (rc != MDBX_SUCCESS) {
        if (local_txn)
            mdbx_txn_abort(txn);
        return ANULL;
    }

    if (mval.iov_len > 0 &&
        ((char *)mval.iov_base)[mval.iov_len - 1] == '\0') {
        /* NUL-terminated — safe for zero-copy from persistent txn */
        if (local_txn) {
            /* Fallback txn: must copy */
            ret = (Attr *)malloc(mval.iov_len);
            if (!ret) { mdbx_txn_abort(txn); return ANULL; }
            memcpy(ret, mval.iov_base, mval.iov_len);
            mdbx_txn_abort(txn);
            return ret;
        }
        return (Attr *)mval.iov_base;
    }

    /* No NUL terminator (old format) — must copy */
    ret = (Attr *)malloc(mval.iov_len + 1);
    if (ret == ANULL) {
        if (local_txn)
            mdbx_txn_abort(txn);
        return ANULL;
    }
    memcpy(ret, mval.iov_base, mval.iov_len);
    ret[mval.iov_len] = '\0';

    if (local_txn)
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
    int need_commit = 0;

    if (!db_initted)
        return 1;

    nsiz = strlen(obj);

    if (active_txn) {
        txn = active_txn;
    } else {
        rc = mdbx_txn_begin(env, NULL, 0, &txn);
        if (rc != MDBX_SUCCESS) {
            log_db_err(nam->object, nam->attrnum, "(Put) mdbx_txn_begin");
            return 1;
        }
        need_commit = 1;
    }

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);
    mval.iov_base = (void *)obj;
    mval.iov_len  = nsiz + 1;  /* include NUL terminator for zero-copy */

    rc = mdbx_put(txn, dbi, &mkey, &mval, 0);
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Put) mdbx_put");
        if (need_commit)
            mdbx_txn_abort(txn);
        return 1;
    }

    if (need_commit) {
        rc = mdbx_txn_commit(txn);
        if (rc != MDBX_SUCCESS) {
            log_db_err(nam->object, nam->attrnum, "(Put) mdbx_txn_commit");
            return 1;
        }
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

    rc = mdbx_txn_begin(env, NULL, MDBX_TXN_RDONLY, &txn);
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
    int need_commit = 0;

    if (!db_initted)
        return -1;

    if (active_txn) {
        txn = active_txn;
    } else {
        rc = mdbx_txn_begin(env, NULL, 0, &txn);
        if (rc != MDBX_SUCCESS)
            return -1;
        need_commit = 1;
    }

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Aname);

    rc = mdbx_del(txn, dbi, &mkey, NULL);
    if (rc == MDBX_NOTFOUND) {
        if (need_commit)
            mdbx_txn_abort(txn);
        return 0;
    }
    if (rc != MDBX_SUCCESS) {
        log_db_err(nam->object, nam->attrnum, "(Del) mdbx_del");
        if (need_commit)
            mdbx_txn_abort(txn);
        return 1;
    }

    if (need_commit) {
        rc = mdbx_txn_commit(txn);
        if (rc != MDBX_SUCCESS) {
            log_db_err(nam->object, nam->attrnum, "(Del) mdbx_txn_commit");
            return 1;
        }
    }

    return 0;
}
