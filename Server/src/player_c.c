/* player_c.c -- Player cache routines */

#include "copyright.h"
#include "autoconf.h"

#include "mudconf.h"
#include "htab.h"
#include "externs.h"
#include "alloc.h"
#include "attrs.h"
#include "db.h"

#ifndef STANDALONE

typedef struct player_cache {
	dbref	player;
	int	money;
	int	queue;
	int	qmax;
	int	cflags;
	struct player_cache *next;
} PCACHE;

NHSHTAB	pcache_htab;
PCACHE	*pcache_head;

#define	PF_DEAD		0x0001
#define	PF_REF		0x0002
#define	PF_MONEY_CH	0x0004
#define	PF_QMAX_CH	0x0008

void pcache_init(void)
{
	pool_init(POOL_PCACHE, sizeof(PCACHE));
	nhashinit(&pcache_htab, 23);
	pcache_head = NULL;
}

static void pcache_reload1(dbref player, PCACHE *pp)
{
char	*cp;

	cp = atr_get_raw(player, A_MONEY);
	if (cp && *cp)
		pp->money = atoi(cp);
	else
		pp->money = 0;

	cp = atr_get_raw(player, A_QUEUEMAX);
	if (cp && *cp)
		pp->qmax = atoi(cp);
	else if (!Builder(player))
		pp->qmax = mudconf.queuemax;
	else
		pp->qmax = -1;
}


PCACHE *pcache_find(dbref player)
{
PCACHE	*pp;

	if (!Good_obj(player) || !OwnsOthers(player))
		return NULL;
	pp = (PCACHE *)nhashfind(player, &pcache_htab);
	if (pp) {
		pp->cflags |= PF_REF;
		return pp;
	}
	pp = alloc_pcache("pcache_find");
	pp->queue = 0;
	pp->cflags = PF_REF;
	pp->player = player;
	pcache_reload1(player, pp);
	pp->next = pcache_head;
	pcache_head = pp;
	nhashadd(player, (int *)pp, &pcache_htab);
	return pp;
}

void pcache_reload(dbref player)
{
PCACHE	*pp;

	pp = pcache_find(player);
	if (!pp)
		return;
	pcache_reload1(player, pp);
}

static void pcache_save(PCACHE *pp)
{
IBUF	tbuf;

	if (pp->cflags & PF_DEAD)
		return;
	if (pp->cflags & PF_MONEY_CH) {
		sprintf(tbuf, "%d", pp->money);
		atr_add_raw(pp->player, A_MONEY, tbuf);
	}
	if (pp->cflags & PF_QMAX_CH) {
		sprintf(tbuf, "%d", pp->qmax);
		atr_add_raw(pp->player, A_QUEUEMAX, tbuf);
	}
	pp->cflags &= ~(PF_MONEY_CH|PF_QMAX_CH);
}

void pcache_trim(void)
{
PCACHE	*pp, *pplast, *ppnext;

	pp = pcache_head;
	pplast = NULL;
	while (pp) {
		if (!(pp->cflags & PF_DEAD) &&
		     (pp->queue || (pp->cflags & PF_REF))) {
			pp->cflags &= ~PF_REF;
			pplast = pp;
			pp = pp->next;
		} else {
			ppnext = pp->next;
			if (pplast)
				pplast->next = ppnext;
			else
				pcache_head = ppnext;
			if (!(pp->cflags & PF_DEAD)) {
				pcache_save(pp);
				nhashdelete(pp->player, &pcache_htab);
			}
			free_pcache(pp);
			pp = ppnext;
		}
	}
}

void pcache_sync(void)
{
PCACHE	*pp;

	pp = pcache_head;
	while(pp) {
		pcache_save(pp);
		pp = pp->next;
	}
}

void pcache_purge(dbref player)
{
PCACHE	*pp;

	pp = (PCACHE *)nhashfind(player, &pcache_htab);
	if (!pp)
		return;
	pp->cflags = PF_DEAD;
	nhashdelete(pp->player, &pcache_htab);
}

int a_Queue(dbref player, int adj)
{
PCACHE	*pp;

	if (OwnsOthers(player)) {
		pp = pcache_find(player);
		if (pp) {
			pp->queue += adj;
			return pp->queue;
		}
	}
	return 0;
}

void s_Queue(dbref player, int val)
{
PCACHE	*pp;

	if (OwnsOthers(player)) {
		pp = pcache_find(player);
		if (pp)
			pp->queue = val;
	}
}

int QueueMax(dbref player)
{
   PCACHE *pp;
   int m;

   m = 0;
   if (OwnsOthers(player)) {
      pp = pcache_find(player);
      if (pp) {
         if (pp->qmax >= 0) {
            m = pp->qmax;
         } else {
            m = mudstate.db_top + 1;
            if ( Good_obj(player) && Builder(player) ) {
               if (m < mudconf.wizqueuemax)
                  m = mudconf.wizqueuemax;
            } else {
               if (m < mudconf.queuemax)
                  m = mudconf.queuemax;
            }
         }
      }
   }
   return m;
}
#endif

int Pennies(dbref obj)
{
char	*cp;
int	val;

#ifndef STANDALONE
PCACHE	*pp;

	if (!Recover(obj) && OwnsOthers(obj)) {
		pp = pcache_find(obj);
		if (pp)
			return pp->money;
	}
#endif
	cp = atr_get_raw(obj, A_MONEY);
	if (cp)
	  val = atoi(cp);
	else
	  val = 0;
	return (val);
}

void s_Pennies(dbref obj, int howfew)
{
IBUF	tbuf;

#ifndef STANDALONE
PCACHE	*pp;

	if (OwnsOthers(obj)) {
		pp = pcache_find(obj);
		if (pp) {
			pp->money = howfew;
			pp->cflags |= PF_MONEY_CH;
			return;
		}
	}
#endif
	sprintf(tbuf, "%d", howfew);
	atr_add_raw(obj, A_MONEY, tbuf);
}
