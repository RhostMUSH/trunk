/* lua.c - Lua engine */

/* This file implements code to instantiate a lua interpreter, interface it
 * with Rhost, and extract results
 */

#include "copyright.h"
#include "autoconf.h"
#include "db.h"
#include "alloc.h"
#include "externs.h"
#include "functions.h"
#include "lua.h"

#include "lua/src/lua.h"
#include "lua/src/lauxlib.h"
#include "lua/src/lualib.h"

extern FUNCTION(fun_strfunc);
extern int FDECL(alarm_msec, (double));
extern int FDECL(next_timer, ());

static const char *rhost_run_as_key = "rhost_run_as";

static void lua_timer_hook(lua_State *L, lua_Debug *ar)
{
    if(mudstate.alarm_triggered) {
        luaL_error(L, "Alarm triggered");
        mudstate.alarm_triggered = 2;
    }
}

static void
lua_stack_to_lbuf(lua_State *L, char *dest, int src_index)
{
    size_t size;
    int argv;

    /* log_text("lua_stack_to_lbuf : src_index : "); log_number(src_index); end_log(); */
    argv = lua_gettop(L);
    /* log_text("lua_stack_to_lbuf : gettop : "); log_number(argv); end_log(); */

    if((argv > 0 && src_index > argv) || (argv < 0 && -src_index > argv) || src_index == 0) {
        /* log_text("lua_stack_to_lbuf : Bad src_index"); end_log(); */
        dest[0] = 0;
        return;
    }

    const char *ret = lua_tolstring(L, src_index, &size);
    if(!ret) {
        /* log_text("lua_stack_to_lbuf : ret : IS NULL"); end_log(); */
        dest[0] = 0;
        return;
    }
    strncpy(dest, lua_tolstring(L, src_index, &size), size >= (LBUF_SIZE - 1) ? LBUF_SIZE - 1 : size);
}

static dbref
lua_pull_run_as(lua_State *L)
{
    dbref run_as;

    /* Pull run_as in lua registry table */
    lua_pushlightuserdata(L, (void *)rhost_run_as_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    run_as = lua_tonumber(L, -1);
    lua_pop(L, 1); /* pops rhost_run_as_key */
    return run_as;
}

static int
lua_check_read_perms(dbref player, dbref thing, ATTR *attr,
                     int aowner, int aflags)
{
    int see_it;

    /* If we have explicit read permission to the attr, return it */

    if (See_attr_explicit(player, thing, attr, aowner, aflags))
       return 1;

    /* If we are nearby or have examine privs to the attr and it is
     * visible to us, return it.
     */

    if ( thing == GOING || thing == AMBIGUOUS || !Good_obj(thing))
        return 0;

    see_it = See_attr(player, thing, attr, aowner, aflags, 0);
    if ((Examinable(player, thing) || nearby(player, thing))) {
        if ( ((attr)->flags & (AF_INTERNAL)) ||
             (((attr)->flags & (AF_DARK)) && !Immortal(player)) ||
             (((attr)->flags & (AF_MDARK)) && !Wizard(player)) )
           return 0;
        else
           return 1;
    }
    /* For any object, we can read its visible attributes, EXCEPT
     * for descs, which are only visible if read_rem_desc is on.
     */

    if (see_it) {
       if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
          return 0;
       } else {
          return 1;
       }
    }
    return 0;
}

/* rhost_get() allows lua to request an attribute from a dbref
 * Arguments: integer dbref, string attribute
 */
static int
rhost_get(lua_State *L)
{
    size_t argv;
    char *attr, *result;
    lua_Integer thing;
    ATTR *atrp;
    dbref run_as, owner, flags;

    /* Check argument count */
    argv = lua_gettop(L);
    /* log_text("rhost_get : gettop : "); log_number(argv); end_log(); */

    if(argv != 2) {
        lua_pushliteral(L, "rhost_get: Incorrect number of arguments");
        lua_error(L);
        lua_pop(L, 1);
        return 0;
    }

    /* Pull run_as in lua registry table */
    run_as = lua_pull_run_as(L);
    /* log_text("rhost_get : run_as : "); log_number(run_as); end_log(); */

    /* Second argument: attribute name */
    attr = alloc_lbuf("lua_rhost_get_attr");
    lua_stack_to_lbuf(L, attr, -1);
    /* log_text("rhost_get : attr : "); log_text(attr); end_log(); */
    lua_pop(L, 1); /* pops attribute name */

    atrp = atr_str4(attr);
    if(!atrp) {
        lua_pushliteral(L, "rhost_get: Invalid object or attribute");
        lua_error(L);
        lua_pop(L, 1);

        free_lbuf(attr);
        return 0;
    }

    /* First argument: dbref to query */
    thing = lua_tointeger(L, -1);
    /* log_text("rhost_get : thing : "); log_number(thing); end_log(); */
    lua_pop(L, 1); /* pops dbref */

    result = atr_get(thing, atrp->number, &owner, &flags);

    if(!lua_check_read_perms(run_as, thing, atrp, owner, flags)) {
        lua_pushliteral(L, "rhost_get: Permission Denied");
        lua_error(L);
        lua_pop(L, 1);

        free_lbuf(attr);
        return 1;
    }

    lua_pushstring(L, result);

    free_lbuf(result);
    free_lbuf(attr);

    return 1;
}

/* rhost_strfunc() allows lua to call any function
 * Arguments: string function name, string arguments
 */
static int
rhost_strfunc(lua_State *L)
{
    char *fargs[3], *ret, *retptr;
    int argv;
    dbref run_as;

    /* Check argument count */
    argv = lua_gettop(L);
    /* log_text("rhost_strfunc : gettop : "); log_number(argv); end_log(); */
    if(argv < 2 || argv > 3) {
        lua_pushliteral(L, "rhost_strfunc: Incorrect number of arguments");
        lua_error(L);
        lua_pop(L, 1);
        return 0;
    }

    /* Third argument: delimiter */
    if(argv > 2) {
        fargs[2] = alloc_lbuf("lua_rhost_strfunc_delim");
        lua_stack_to_lbuf(L, fargs[2], -1);
        /* log_text("rhost_strfunc : fargs[2] : "); log_text(fargs[2]); end_log(); */
        lua_pop(L, 1); /* pops args */
    }

    /* Second argument: args */
    fargs[1] = alloc_lbuf("lua_rhost_strfunc_args");
    lua_stack_to_lbuf(L, fargs[1], -1);
    /* log_text("rhost_strfunc : fargs[1] : "); log_text(fargs[1]); end_log(); */
    lua_pop(L, 1); /* pops args */

    /* First argument: func */
    fargs[0] = alloc_lbuf("lua_rhost_strfunc_func");
    lua_stack_to_lbuf(L, fargs[0], -1);
    /* log_text("rhost_strfunc : fargs[0] : "); log_text(fargs[0]); end_log(); */
    lua_pop(L, 1); /* pops func*/

    run_as = lua_pull_run_as(L);
    /* log_text("rhost_strfunc : run_as : "); log_number(run_as); end_log(); */

    ret = alloc_lbuf("lua_rhost_strfunc_ret");
    retptr = ret;
    fun_strfunc(ret, &retptr, run_as, run_as, run_as, fargs, argv, NULL, 0);

    /* log_text("rhost_strfunc : got back : "); log_text(ret); end_log(); */

    lua_pushstring(L, ret);

    free_lbuf(fargs[0]);
    free_lbuf(fargs[1]);
    if(argv > 2) {
        free_lbuf(fargs[2]);
    }
    free_lbuf(ret);
    return 1;
}

/* rhost_parseansi() allows lua to parse to ANSI output
 * Arguments: string text
 */
static int
rhost_parseansi(lua_State *L)
{
    char *fargs[1], *ascii, *asciip, *accents, *accentsp, *utf8, *utf8p;
    int argv;

    /* Check argument count */
    argv = lua_gettop(L);
    /* log_text("rhost_parseansi : gettop : "); log_number(argv); end_log(); */
    if(argv < 1) {
        lua_pushliteral(L, "rhost_parseansi: Incorrect number of arguments");
        lua_error(L);
        lua_pop(L, 1);
        return 0;
    }

    /* First argument: str */
    fargs[0] = alloc_lbuf("lua_rhost_parseansi_str");
    lua_stack_to_lbuf(L, fargs[0], -1);
    /* log_text("rhost_parseansi : fargs[0] : "); log_text(fargs[0]); end_log(); */
    lua_pop(L, 1); /* pops str */

    asciip = ascii = alloc_lbuf("lua_rhost_parseansi_ascii");
    accentsp = accents = alloc_lbuf("lua_rhost_parseansi_ascii");
    utf8p = utf8 = alloc_lbuf("lua_rhost_parseansi_utf8");
#ifdef ZENTY_ANSI
    parse_ansi(fargs[0], ascii, &asciip, accents, &accentsp, utf8, &utf8p);
#else
    /* take raw, zenty ansi not enabled */
    strcpy(utf8, fargs[0]);
#endif

    /* log_text("rhost_parseansi : got back ascii: "); log_text(ascii); end_log(); */
    /* log_text("rhost_parseansi : got back accents : "); log_text(accents); end_log(); */
    /* log_text("rhost_parseansi : got back utf8 : "); log_text(utf8); end_log(); */

    lua_pushstring(L, utf8);

    free_lbuf(fargs[0]);
    free_lbuf(ascii);
    free_lbuf(accents);
    free_lbuf(utf8);
    return 1;
}

lua_t *
open_lua_interpreter(dbref run_as)
{
    lua_t *lua;
    char *filename;

    filename = alloc_lbuf("lua_filename");

    lua = malloc(sizeof(lua_t));
    lua->state = luaL_newstate();

    /* Add base libraries */
    luaL_requiref(lua->state, "base", luaopen_base, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "string", luaopen_string, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "utf8", luaopen_utf8, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "table", luaopen_table, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "math", luaopen_math, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "coroutine", luaopen_coroutine, 1);
    lua_pop(lua->state, 1);
    luaL_requiref(lua->state, "os", luaopen_os, 1);
    lua_pop(lua->state, 1);

    /* We're going to make changes to the global table */
    lua_pushglobaltable(lua->state);

    /* Bring in JSON Support */
    snprintf(filename, LBUF_SIZE - 1, "%s/%s", mudconf.txt_dir, "dkjson.lua");
    if(luaL_dofile(lua->state, filename)) {
        lua_pop(lua->state, 1);
        log_text("LUA : Error loading dkjson.lua");
        end_log();
    } else {
        lua_setfield(lua->state, -2, "json");
    }

    /* Remove naughty functions from basic */
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "assert");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "collectgarbage");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "error");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "dofile");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "getfenv");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "load");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "loadfile");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "print");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "warn");

    /* Remove naughty functions from os */
    lua_getglobal(lua->state, "os");
    lua_pushnil(lua->state);
    lua_setfield(lua->state, -2, "getenv");
    lua_pop(lua->state, 1); /* pops os */

    /* Bring in rhost table */
    snprintf(filename, LBUF_SIZE - 1, "%s/%s", mudconf.txt_dir, "rhost.lua");
    if(luaL_dofile(lua->state, filename)) {
        lua_pop(lua->state, 1);
        log_text("LUA : Error loading rhost.lua");
        end_log();
    } else {
        lua_setfield(lua->state, -2, "rhost");
    }

    /* Add Rhost functions */
    lua_getglobal(lua->state, "rhost");
    lua_pushcfunction(lua->state, rhost_get);
    lua_setfield(lua->state, -2, "get");
    lua_pushcfunction(lua->state, rhost_strfunc);
    lua_setfield(lua->state, -2, "strfunc");
    lua_pushcfunction(lua->state, rhost_parseansi);
    lua_setfield(lua->state, -2, "parseansi");

    lua_pop(lua->state, 1); /* pops rhost */

    /* Stores run_as dbref in lua registry table for us to retrieve later */
    lua_pushlightuserdata(lua->state, (void *)rhost_run_as_key);
    lua_pushinteger(lua->state, run_as);
    lua_settable(lua->state, LUA_REGISTRYINDEX);

    free_lbuf(filename);

    lua_sethook(lua->state, &lua_timer_hook, LUA_MASKCOUNT, 1);

    return lua;
}

void
close_lua_interpreter(lua_t *lua)
{
    lua_close(lua->state);
    free(lua);
}

char *
exec_lua_script(lua_t *lua, char *scriptbuf, int *len)
{
    char *res = 0;
    const char *raw;
    size_t size;
    int error;

    alarm_msec(5);
    if(LUA_OK == (error = luaL_dostring(lua->state, scriptbuf))) {
        raw = lua_tolstring(lua->state, -1, &size);
    } else {
        raw = lua_tolstring(lua->state, -1, &size);
        size = strlen(raw);

        log_text("LUA : Error ");
        log_text((char *)raw);
        end_log();
    }

    if(raw && !mudstate.alarm_triggered) {
        res = malloc(size + 1);
        strncpy(res, raw, size + 1);
        res[size] = 0; /* Just coding defensively here */
    }

    if(res) {
        *len = strlen(res);
    } else {
        *len = 0;
    }

    return res;
}
