/* cmd_prefix.c — $command prefix index for fast softcode dispatch */

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "htab.h"
#include "mudconf.h"
#include "externs.h"
#include "flags.h"
#include "command.h"
#include "alloc.h"
#include "cmd_prefix.h"

OHTAB cmd_prefix_htab;

CMD_PREFIX_MATCH *cmd_regexp_list = NULL;
int cmd_regexp_count = 0;
int cmd_regexp_capacity = 0;

static char *extract_prefix(const char *buff)
{
    const char *p;
    int len;
    char *key;

    p = buff + 1;
    while (*p == ' ' || *p == '\t')
        p++;
    if (!*p || *p == ':' || *p == '*')
        return NULL;

    len = 0;
    while (p[len] && p[len] != ' ' && p[len] != '\t'
           && p[len] != '*' && p[len] != ':')
        len++;

    if (len == 0)
        return NULL;

    key = malloc(len + 1);
    if (!key)
        return NULL;
    for (int i = 0; i < len; i++)
        key[i] = tolower((unsigned char)p[i]);
    key[len] = '\0';
    return key;
}

static CMD_PREFIX_ENTRY *get_or_create_entry(const char *key)
{
    CMD_PREFIX_ENTRY *ent;
    int *data;

    data = (int *)ohtab_find(key, &cmd_prefix_htab);
    if (data)
        return (CMD_PREFIX_ENTRY *)data;

    ent = malloc(sizeof(CMD_PREFIX_ENTRY));
    if (!ent) return NULL;
    ent->count = 0;
    ent->capacity = 4;
    ent->matches = malloc(ent->capacity * sizeof(CMD_PREFIX_MATCH));
    if (!ent->matches) { free(ent); return NULL; }

    if (ohtab_add((char *)key, (int *)ent, &cmd_prefix_htab, 1) != 0) {
        free(ent->matches);
        free(ent);
        return NULL;
    }
    return ent;
}

void cmd_prefix_check_add(dbref thing, int atr, const char *buff, int flags)
{
    CMD_PREFIX_ENTRY *ent;
    char *key;
    CMD_PREFIX_MATCH *m;

    if (!buff || !*buff || buff[0] != '$')
        return;
    if ((atr < A_USER_START) && (atr < A_INLINE_START))
        return;

    if (flags & AF_REGEXP) {
        for (int i = 0; i < cmd_regexp_count; i++) {
            if (cmd_regexp_list[i].thing == thing
                && cmd_regexp_list[i].atr == atr)
                return;
        }
        if (cmd_regexp_count >= cmd_regexp_capacity) {
            int newcap = cmd_regexp_capacity ? cmd_regexp_capacity * 2 : 4;
            m = realloc(cmd_regexp_list,
                        newcap * sizeof(CMD_PREFIX_MATCH));
            if (!m) return;
            cmd_regexp_list = m;
            cmd_regexp_capacity = newcap;
        }
        cmd_regexp_list[cmd_regexp_count].thing = thing;
        cmd_regexp_list[cmd_regexp_count].atr = atr;
        cmd_regexp_count++;
        return;
    }

    key = extract_prefix(buff);
    if (!key)
        return;

    ent = get_or_create_entry(key);
    free(key);

    if (!ent)
        return;

    for (int i = 0; i < ent->count; i++) {
        if (ent->matches[i].thing == thing
            && ent->matches[i].atr == atr)
            return;
    }

    if (ent->count >= ent->capacity) {
        ent->capacity *= 2;
        m = realloc(ent->matches,
                    ent->capacity * sizeof(CMD_PREFIX_MATCH));
        if (!m) {
            ent->capacity /= 2;
            return;
        }
        ent->matches = m;
    }
    ent->matches[ent->count].thing = thing;
    ent->matches[ent->count].atr = atr;
    ent->count++;
}

CMD_PREFIX_ENTRY *cmd_prefix_lookup(const char *prefix)
{
    int *data;

    data = (int *)ohtab_find(prefix, &cmd_prefix_htab);
    if (!data)
        return NULL;
    return (CMD_PREFIX_ENTRY *)data;
}

void cmd_prefix_remove_thing(dbref thing)
{
    CMD_PREFIX_ENTRY *ent;
    int *data;
    int w, r;

    for (data = (int *)ohtab_firstentry(&cmd_prefix_htab);
         data;
         data = (int *)ohtab_nextentry(&cmd_prefix_htab))
    {
        ent = (CMD_PREFIX_ENTRY *)data;
        w = 0;
        for (r = 0; r < ent->count; r++) {
            if (ent->matches[r].thing != thing)
                ent->matches[w++] = ent->matches[r];
        }
        ent->count = w;
    }

    w = 0;
    for (r = 0; r < cmd_regexp_count; r++) {
        if (cmd_regexp_list[r].thing != thing)
            cmd_regexp_list[w++] = cmd_regexp_list[r];
    }
    cmd_regexp_count = w;
}

void cmd_prefix_remove_match(dbref thing, int atr)
{
    CMD_PREFIX_ENTRY *ent;
    int *data;
    int w, r, i;

    for (i = 0; i < cmd_regexp_count; i++) {
        if (cmd_regexp_list[i].thing == thing
            && cmd_regexp_list[i].atr == atr) {
            cmd_regexp_list[i] = cmd_regexp_list[--cmd_regexp_count];
            break;
        }
    }

    for (data = (int *)ohtab_firstentry(&cmd_prefix_htab);
         data;
         data = (int *)ohtab_nextentry(&cmd_prefix_htab))
    {
        ent = (CMD_PREFIX_ENTRY *)data;
        w = 0;
        for (r = 0; r < ent->count; r++) {
            if (ent->matches[r].thing != thing
                || ent->matches[r].atr != atr)
                ent->matches[w++] = ent->matches[r];
        }
        if (w < ent->count) {
            ent->count = w;
            return;
        }
    }
}

int cmd_prefix_thing_matches(CMD_PREFIX_ENTRY *ent, dbref thing)
{
    dbref parent;
    int lev;

    if (!ent)
        return 0;

    ITER_PARENTS(thing, parent, lev) {
        for (int i = 0; i < ent->count; i++) {
            if (ent->matches[i].thing == parent)
                return 1;
        }
    }
    return 0;
}

int cmd_prefix_regexp_has_thing(dbref thing)
{
    dbref parent;
    int lev;

    if (cmd_regexp_count == 0)
        return 0;

    ITER_PARENTS(thing, parent, lev) {
        for (int i = 0; i < cmd_regexp_count; i++) {
            if (cmd_regexp_list[i].thing == parent)
                return 1;
        }
    }
    return 0;
}

void cmd_prefix_rebuild(void)
{
    dbref thing;
    int attr, aflags;
    dbref aowner;
    char *as, *buff;
    int *data;

    for (data = (int *)ohtab_firstentry(&cmd_prefix_htab);
         data;
         data = (int *)ohtab_nextentry(&cmd_prefix_htab))
    {
        CMD_PREFIX_ENTRY *ent = (CMD_PREFIX_ENTRY *)data;
        free(ent->matches);
        free(ent);
    }
    ohtab_flush(&cmd_prefix_htab, 0);

    free(cmd_regexp_list);
    cmd_regexp_list = NULL;
    cmd_regexp_count = 0;
    cmd_regexp_capacity = 0;

    for (thing = 0; thing <= mudstate_hot.db_top; thing++) {
        if (!Good_obj(thing))
            continue;
#ifdef ENABLE_COMMAND_FLAG
        if (!(Flags4(thing) & COMMANDS))
            continue;
#endif
        atr_push();
        for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
            buff = atr_get(thing, attr, &aowner, &aflags);
            if (buff && buff[0] == '$')
                cmd_prefix_check_add(thing, attr, buff, aflags);
            free_lbuf(buff);
        }
        atr_pop();
    }

    STARTLOG(LOG_STARTUP, "INI", "CMDP")
        log_text("$command prefix index rebuilt.");
    ENDLOG
}

void cmd_prefix_init(void)
{
    ohtab_init(&cmd_prefix_htab, 4095);
}
