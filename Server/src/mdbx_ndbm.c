/*
 * mdbx_ndbm.c - NDBM compatibility shim for libmdbx
 *
 * Phase 1: stub implementations that return controlled errors.
 *          dbm_open returns NULL, fetch/firstkey/nextkey return empty
 *          datums, store/delete return -1.  A one-time warning is
 *          printed to stderr so the operator knows why startup failed.
 * Phase 2: real implementations backed by libmdbx.
 */

#include "autoconf.h"

#ifdef MDBX

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include "../hdrs/mdbx_ndbm.h"

static int mdbx_ndbm_warned = 0;

static void
mdbx_ndbm_warn(const char *func)
{
    if (!mdbx_ndbm_warned) {
        fprintf(stderr,
            "mdbx_ndbm: %s called but the MDBX shim is not yet implemented.\n"
            "mdbx_ndbm: The MDBX backend is not usable until Phase 2.\n"
            "mdbx_ndbm: Rebuild with QDBM or GDBM for a working backend.\n",
            func);
        mdbx_ndbm_warned = 1;
    }
}

DBM *
dbm_open(const char *name, int flags, mode_t mode)
{
    mdbx_ndbm_warn("dbm_open");
    return NULL;
}

void
dbm_close(DBM *db)
{
    mdbx_ndbm_warn("dbm_close");
}

datum
dbm_fetch(DBM *db, datum key)
{
    datum d = { NULL, 0 };
    return d;
}

int
dbm_store(DBM *db, datum key, datum val, int flags)
{
    return -1;
}

int
dbm_delete(DBM *db, datum key)
{
    return -1;
}

datum
dbm_firstkey(DBM *db)
{
    datum d = { NULL, 0 };
    return d;
}

datum
dbm_nextkey(DBM *db)
{
    datum d = { NULL, 0 };
    return d;
}

#endif /* MDBX */
