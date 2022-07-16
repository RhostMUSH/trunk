#ifndef _M_LUA_H
#define _M_LUA_H

#include "copyright.h"

typedef struct _luatype {
	void *state;
} lua_t;

extern lua_t *open_lua_interpreter(dbref run_as);
extern void close_lua_interpreter(lua_t *lua);
extern char *exec_lua_script(lua_t *lua, char *scriptbuf, int *len);

#endif /* _M_LUA_H */
