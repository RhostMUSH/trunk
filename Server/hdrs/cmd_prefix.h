#ifndef _M_CMD_PREFIX_H
#define _M_CMD_PREFIX_H

#include "config.h"
#include "db.h"
#include "htab.h"

typedef struct {
    dbref thing;
    int   atr;
} CMD_PREFIX_MATCH;

typedef struct {
    int   count;
    int   capacity;
    CMD_PREFIX_MATCH *matches;
} CMD_PREFIX_ENTRY;

extern OHTAB cmd_prefix_htab;
extern CMD_PREFIX_MATCH *cmd_regexp_list;
extern int cmd_regexp_count;
extern int cmd_regexp_capacity;

void NDECL(cmd_prefix_init);
void cmd_prefix_check_add(dbref thing, int atr, const char *buff, int flags);
CMD_PREFIX_ENTRY *cmd_prefix_lookup(const char *prefix);
void cmd_prefix_remove_thing(dbref thing);
void cmd_prefix_remove_match(dbref thing, int atr);
void cmd_prefix_rebuild(void);
int cmd_prefix_thing_matches(CMD_PREFIX_ENTRY *ent, dbref thing);
int cmd_prefix_regexp_has_thing(dbref thing);

#endif
