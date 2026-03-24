/*
 * udb_mdbx_ochunk.c - MDBX-backed object chunk storage
 *
 * Replaces udb_ochunk.c when MDBX is selected as the backend.
 * Stores serialized Obj structures directly in MDBX as key-value pairs,
 * eliminating the bitmap allocator and flat .db file.
 *
 * Key:   Objname (unsigned int, 4 bytes)
 * Value: serialized Obj (header + attributes, same binary format as
 *        objtoFILE/objfromFILE but in a contiguous buffer)
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

extern int  FDECL(obj_siz, (Obj *));
extern void VDECL(fatal, (const char *,...));
extern void mush_logf();

/* On-disk sizes matching udb_obj.c */
#define OBJ_HEADER_SIZE   (sizeof(Objname) + sizeof(int))
#define ATTR_HEADER_SIZE  (sizeof(int) * 2)

#define DEFAULT_DBMCHUNKFILE "mudDB"

/* Geometry defaults */
#define OCHUNK_SIZE_LOWER   (1L << 20)        /*   1 MB  */
#define OCHUNK_SIZE_UPPER   (2L << 30)        /*   2 GB  */
#define OCHUNK_GROWTH_STEP  (16L << 20)       /*  16 MB  */
#define OCHUNK_SHRINK_THR   (256L << 20)      /* 256 MB  */

static char       *dbfile = NULL;
static int         bsiz = 256;  /* unused but kept for API compat */
static int         db_initted = 0;

static MDBX_env   *env = NULL;
static MDBX_dbi    dbi;

/* ------------------------------------------------------------------ */
/* Buffer-based serialization (equivalent to objtoFILE/objfromFILE)    */
/* ------------------------------------------------------------------ */

/*
 * Serialize an Obj into a contiguous buffer.
 * Returns malloc'd buffer and sets *outsiz.  Caller frees.
 * Format matches objtoFILE: Objname, at_count, then per-attr:
 *   size(int), attrnum(int), data(size bytes).
 */
static char *
obj_to_buf(Obj *o, int *outsiz)
{
    int siz, i, off;
    char *buf;
    Attrib *a;

    siz = obj_siz(o);
    buf = (char *)malloc(siz);
    if (!buf)
        return NULL;

    off = 0;

    /* Object header */
    memcpy(buf + off, &o->name, sizeof(Objname));
    off += sizeof(Objname);
    memcpy(buf + off, &o->at_count, sizeof(int));
    off += sizeof(int);

    /* Attributes */
    a = o->atrs;
    for (i = 0; i < o->at_count; i++) {
        memcpy(buf + off, &a[i].size, sizeof(int));
        off += sizeof(int);
        memcpy(buf + off, &a[i].attrnum, sizeof(int));
        off += sizeof(int);
        memcpy(buf + off, a[i].data, a[i].size);
        off += a[i].size;
    }

    *outsiz = siz;
    return buf;
}

/*
 * Deserialize an Obj from a contiguous buffer.
 * Returns a malloc'd Obj with malloc'd attribute data.
 * Caller frees via the normal Obj free path.
 */
static Obj *
obj_from_buf(const char *buf, int bufsiz)
{
    Obj *o;
    Attrib *a;
    int i, j, off, size;

    if (bufsiz < (int)OBJ_HEADER_SIZE)
        return NULL;

    o = (Obj *)malloc(sizeof(Obj));
    if (!o)
        return NULL;

    off = 0;

    /* Object header */
    memcpy(&o->name, buf + off, sizeof(Objname));
    off += sizeof(Objname);
    memcpy(&o->at_count, buf + off, sizeof(int));
    off += sizeof(int);

    if (o->at_count < 0) {
        free(o);
        return NULL;
    }

    /* Attribute array */
    a = o->atrs = (Attrib *)malloc(o->at_count * sizeof(Attrib));
    if (!a) {
        free(o);
        return NULL;
    }

    for (j = 0; j < o->at_count; ) {
        if (off + (int)ATTR_HEADER_SIZE > bufsiz)
            goto bail;

        memcpy(&size, buf + off, sizeof(int));
        off += sizeof(int);
        a[j].size = size;

        memcpy(&a[j].attrnum, buf + off, sizeof(int));
        off += sizeof(int);

        if (off + size > bufsiz)
            goto bail;

        a[j].data = (char *)malloc(size);
        if (!a[j].data)
            goto bail;

        j++;  /* increment before copy so bail knows how many to free */

        memcpy(a[j-1].data, buf + off, size);
        off += size;
    }

    return o;

bail:
    for (i = 0; i < j; i++)
        free(a[i].data);
    free(a);
    free(o);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* dddb_* API                                                          */
/* ------------------------------------------------------------------ */

void
dddb_var_init()
{
    dbfile = NULL;
    bsiz = 256;
    db_initted = 0;
}

int
dddb_init()
{
    char fnam[1024];
    MDBX_txn *txn;
    int rc;

    rc = mdbx_env_create(&env);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_create failed\n", (char *)0);
        return 1;
    }

    rc = mdbx_env_set_geometry(env,
                               OCHUNK_SIZE_LOWER, -1, OCHUNK_SIZE_UPPER,
                               OCHUNK_GROWTH_STEP, OCHUNK_SHRINK_THR, -1);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_set_geometry failed\n", (char *)0);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    snprintf(fnam, sizeof(fnam), "%.250s.mdbx",
             dbfile ? dbfile : DEFAULT_DBMCHUNKFILE);

    rc = mdbx_env_open(env, fnam, MDBX_NOSUBDIR | MDBX_NOTLS, 0600);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_env_open failed for ", fnam, "\n", (char *)0);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    rc = mdbx_txn_begin(env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_txn_begin failed\n", (char *)0);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    rc = mdbx_dbi_open(txn, NULL, MDBX_CREATE, &dbi);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_dbi_open failed\n", (char *)0);
        mdbx_txn_abort(txn);
        mdbx_env_close(env);
        env = NULL;
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_init: mdbx_txn_commit failed\n", (char *)0);
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
    /* Block size is not relevant for MDBX — retained for API compat. */
    return (bsiz = nbsiz);
}


int dddb_setfile(char *fil)
{
    char *xp;

    if (db_initted)
        return 1;

    xp = (char *)malloc((unsigned)strlen(fil) + 1);
    if (xp == (char *)0)
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
    if (dbfile != (char *)0) {
        free(dbfile);
        dbfile = (char *)0;
    }
    db_initted = 0;
    return 0;
}


Obj *
dddb_get(Objname *nam)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    Obj *ret;
    int rc;

    if (!db_initted)
        return (Obj *)0;

    rc = mdbx_txn_begin(env, NULL, MDBX_RDONLY, &txn);
    if (rc != MDBX_SUCCESS)
        return (Obj *)0;

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Objname);

    rc = mdbx_get(txn, dbi, &mkey, &mval);
    if (rc != MDBX_SUCCESS) {
        mdbx_txn_abort(txn);
        return (Obj *)0;
    }

    ret = obj_from_buf((const char *)mval.iov_base, (int)mval.iov_len);
    if (ret == (Obj *)0)
        mush_logf("db_get: cannot decode object\n", (char *)0);

    mdbx_txn_abort(txn);
    return ret;
}


int
dddb_put(Obj *obj, Objname *nam)
{
    MDBX_txn *txn;
    MDBX_val mkey, mval;
    char *buf;
    int bufsiz, rc;

    if (!db_initted)
        return 1;

    buf = obj_to_buf(obj, &bufsiz);
    if (!buf) {
        mush_logf("db_put: cannot serialize object\n", (char *)0);
        return 1;
    }

    rc = mdbx_txn_begin(env, NULL, 0, &txn);
    if (rc != MDBX_SUCCESS) {
        free(buf);
        mush_logf("db_put: mdbx_txn_begin failed\n", (char *)0);
        return 1;
    }

    mkey.iov_base = (void *)nam;
    mkey.iov_len  = sizeof(Objname);
    mval.iov_base = buf;
    mval.iov_len  = bufsiz;

    rc = mdbx_put(txn, dbi, &mkey, &mval, 0);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_put: mdbx_put failed\n", (char *)0);
        mdbx_txn_abort(txn);
        free(buf);
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    free(buf);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_put: mdbx_txn_commit failed\n", (char *)0);
        return 1;
    }

    return 0;
}


/*
 * Note: the original udb_ochunk.c uses strlen(nam)+1 as the key size here,
 * which is inconsistent with all other operations that key on sizeof(Objname).
 * In the MDBX backend all records are keyed by Objname, so we treat the
 * pointer as an Objname* to match.  The char* signature is retained for
 * API compatibility with the cache layer declaration.
 */
int
dddb_check(char *nam)
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
    mkey.iov_len  = sizeof(Objname);

    rc = mdbx_get(txn, dbi, &mkey, &mval);
    mdbx_txn_abort(txn);

    return (rc == MDBX_SUCCESS) ? 1 : 0;
}


int
dddb_del(Objname *nam)
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
    mkey.iov_len  = sizeof(Objname);

    rc = mdbx_del(txn, dbi, &mkey, NULL);
    if (rc == MDBX_NOTFOUND) {
        mdbx_txn_abort(txn);
        return 0;
    }
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_del: mdbx_del failed\n", (char *)0);
        mdbx_txn_abort(txn);
        return 1;
    }

    rc = mdbx_txn_commit(txn);
    if (rc != MDBX_SUCCESS) {
        mush_logf("db_del: mdbx_txn_commit failed\n", (char *)0);
        return 1;
    }

    return 0;
}
