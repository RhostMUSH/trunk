/*
 * udb_mdbx_acache.c - MDBX-optimized attribute cache layer
 *
 * Replaces udb_acache.c when MDBX is selected as the backend.
 *
 * Instead of a large pre-allocated LRU cache with 4 chains per bucket,
 * this uses a small write-buffer hash table for dirty-tracking only.
 *
 * Reads bypass the cache entirely — they go directly to MDBX via a
 * persistent read txn opened by dddb_read_begin()/dddb_read_end()
 * in the main game loop, enabling zero-copy from the mmap region.
 */

#include "autoconf.h"
#include "udb.h"
#include "udb_defs.h"
#include "mudconf.h"
#include "externs.h"

#include <stdlib.h>
#include <string.h>

/* extern declarations from the MDBX attribute backend */
extern Attr *		dddb_get(Aname *nam);
extern int		dddb_put(Attr *obj, Aname *nam);
extern int		dddb_check(Aname *nam);
extern int		dddb_del(Aname *nam, int flg);
extern int		dddb_txn_begin(void);
extern int		dddb_txn_commit(void);
extern int		dddb_txn_abort(void);
extern void		VDECL(log_db_err, (int, int, const char *));

/* ------------------------------------------------------------------ */
/* Cache stats (global variables for @list cache command)             */
/* ------------------------------------------------------------------ */

time_t	cs_ltime;
int	cs_writes;
int	cs_reads;
int	cs_dbreads;
int	cs_dbwrites;
int	cs_dels;
int	cs_checks;
int	cs_rhits;
int	cs_ahits;
int	cs_whits;
int	cs_fails;
int	cs_resets;
int	cs_syncs;
int	cs_objects;

/* ------------------------------------------------------------------ */
/* Write buffer — open-addressing hash table for in-flight writes     */
/* ------------------------------------------------------------------ */

#define WBUF_SIZE 256

typedef struct {
    Aname   key;
    Attr   *value;   /* malloc'd copy, NULL = deleted marker */
    int     in_use;
} WBEntry;

static WBEntry wbuf[WBUF_SIZE];
static int     wbuf_count;

static unsigned int
wbuf_hash(Aname *nam)
{
    return nam->object ^ (nam->attrnum << 7) ^ (nam->attrnum >> 3);
}

static WBEntry *
wbuf_lookup(Aname *nam)
{
    unsigned int idx = wbuf_hash(nam) % WBUF_SIZE;
    for (int i = 0; i < WBUF_SIZE; i++) {
        WBEntry *e = &wbuf[(idx + i) % WBUF_SIZE];
        if (!e->in_use)
            return NULL;
        if (e->key.object == nam->object && e->key.attrnum == nam->attrnum)
            return e;
    }
    return NULL;
}

static WBEntry *
wbuf_insert(Aname *nam)
{
    unsigned int idx = wbuf_hash(nam) % WBUF_SIZE;
    for (int i = 0; i < WBUF_SIZE; i++) {
        WBEntry *e = &wbuf[(idx + i) % WBUF_SIZE];
        if (!e->in_use) {
            e->key = *nam;
            e->value = NULL;
            e->in_use = 1;
            wbuf_count++;
            return e;
        }
        if (e->key.object == nam->object && e->key.attrnum == nam->attrnum)
            return e;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* cache_* API                                                         */
/* ------------------------------------------------------------------ */

void
cache_var_init(void)
{
    cs_writes = 0;
    cs_reads = 0;
    cs_dbreads = 0;
    cs_dbwrites = 0;
    cs_dels = 0;
    cs_checks = 0;
    cs_rhits = 0;
    cs_ahits = 0;
    cs_whits = 0;
    cs_fails = 0;
    cs_resets = 0;
    cs_syncs = 0;
    cs_objects = 0;
}

int
cache_init(int width, int depth)
{
    memset(wbuf, 0, sizeof(wbuf));
    wbuf_count = 0;
    return 0;
}

Attr *
cache_get(Aname *nam)
{
    if (nam == NNULL)
        return ANULL;

    /* Check write buffer for uncommitted writes first */
    WBEntry *e = wbuf_lookup(nam);
    if (e)
        return e->value;   /* NULL = deleted marker, else stable copy */

    /* Read directly from MDBX via persistent read txn (zero-copy) */
    return dddb_get(nam);
}

int
cache_put(Aname *nam, Attr *obj)
{
    if (obj == ANULL || nam == NNULL)
        return 1;

    WBEntry *e = wbuf_insert(nam);
    if (!e) {
        /* Write buffer full — flush then retry */
        if (cache_sync())
            return 1;
        return cache_put(nam, obj);
    }

    if (e->value)
        free(e->value);
    e->value = obj;
    return 0;
}

void
cache_del(Aname *nam)
{
    if (nam == NNULL)
        return;

    WBEntry *e = wbuf_insert(nam);
    if (!e) {
        cache_sync();
        cache_del(nam);
        return;
    }

    if (e->value)
        free(e->value);
    e->value = NULL;   /* NULL = deleted marker */
}

int
cache_check(Aname *nam)
{
    if (nam == NNULL)
        return 0;

    WBEntry *e = wbuf_lookup(nam);
    if (e)
        return 1;   /* in write buffer (even if value is NULL/deleted) */

    return dddb_check(nam);
}

int
cache_sync(void)
{
    if (wbuf_count == 0)
        return 0;

    if (dddb_txn_begin())
        return 1;

    for (int i = 0; i < WBUF_SIZE; i++) {
        WBEntry *e = &wbuf[i];
        if (!e->in_use)
            continue;

        if (e->value == NULL)
            dddb_del(&e->key, 0);
        else
            dddb_put(e->value, &e->key);

        if (e->value)
            free(e->value);
        e->in_use = 0;
    }
    wbuf_count = 0;

    return dddb_txn_commit();
}

void
cache_reset(int trim)
{
    /* No-op: there is no read cache to age out.
     * The write buffer persists until explicitly synced.
     * cache_sync() is the only way to clear it. */
}
