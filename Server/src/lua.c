/* lua.c - Lua engine */

/* This file implements code to instantiate a lua interpreter, interface it
 * with Rhost, and extract results
 */

#include "copyright.h"
#include "autoconf.h"
#include "db.h"
#include "alloc.h"
#include "externs.h"
#include "lua.h"

#include "lua/src/lua.h"
#include "lua/src/lauxlib.h"
#include "lua/src/lualib.h"

static const char *rhost_run_as_key = "rhost_run_as";

/* rhost_get() allows lua to request an attribute from a dbref
 * Arguments: integer dbref, string attribute
 */
static int
rhost_get(lua_State *L)
{
	size_t argv, size;
	char *attr, *result;
	lua_Integer thing;
	ATTR *atrp;
	dbref run_as, owner, flags;

	/* Check argument count */
	argv = lua_gettop(L);
	if(argv != 2) {
		lua_pushliteral(L, "rhost_get: Incorrect number of arguments");
		lua_error(L);
		lua_pop(L, 1);
		return 0;
	}

	/* Pull run_as in lua registry table */
	lua_pushlightuserdata(L, (void *)rhost_run_as_key);
	lua_gettable(L, LUA_REGISTRYINDEX);
	run_as = lua_tonumber(L, -1);
	lua_pop(L, 1); /* pops rhost_run_as_key */

	/* Second argument: attribute name */
	attr = alloc_lbuf("lua_rhost_get_attr");
	strncpy(attr, lua_tolstring(L, -1, &size), LBUF_SIZE >= size ? LBUF_SIZE : size);
	lua_pop(L, 1); /* pops attribute name */

	/* TODO: verify thing has permission to read dbref */
	atrp = atr_str4(attr);
	if(!atrp) {
		lua_pushliteral(L, "rhost_get: Invalid attribute");
		lua_error(L);
		lua_pop(L, 1);

		free_lbuf(attr);
		return 0;
	}

	/* First argument: dbref to query */
	thing = lua_tointeger(L, -1);
	lua_pop(L, 1); /* pops dbref */

	result = atr_get(thing, atrp->number, &owner, &flags);

	lua_pushstring(L, result);

	free_lbuf(result);
	free_lbuf(attr);

	return 1;
}

lua_t *
open_lua_interpreter(dbref run_as)
{
	lua_t *lua;

	lua = malloc(sizeof(lua_t));
	lua->state = luaL_newstate();
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

	/* Create 'rhost' table containing rhost functions */
	lua_newtable(lua->state);

	/* Add Rhost functions */
	lua_pushcfunction(lua->state, rhost_get);
	lua_setfield(lua->state, -2, "get");

	/* Stick rhost table into the global */
	lua_setfield(lua->state, -2, "rhost");

	lua_pop(lua->state, 1); /* pops global */

	/* Stores run_as dbref in lua registry table for us to retrieve later */
	lua_pushlightuserdata(lua->state, (void *)rhost_run_as_key);
	lua_pushinteger(lua->state, run_as);
	lua_settable(lua->state, LUA_REGISTRYINDEX);

	return lua;
}

void
close_lua_interpreter(lua_t *lua)
{
	lua_close(lua->state);
	free(lua);
}

char *
exec_lua_script(lua_t *lua, char *scriptbuf)
{
	char *res;
	const char *raw;
	size_t size;
	int error;

	res = alloc_lbuf("lua_exec_lua_script_res");

	if(LUA_OK == (error = luaL_dostring(lua->state, scriptbuf))) {
		raw = lua_tolstring(lua->state, -1, &size);
		if(raw) {
			strncpy(res, lua_tolstring(lua->state, -1, &size), LBUF_SIZE >= size ? LBUF_SIZE : size);
		}
	}

	return res;
}
