
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>

#include "skynet_socket.h"

#define BACKLOG  32


static int 
llisten(lua_State *L){
	const char * host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	int backlog = luaL_optinteger(L, 3);

	struct skynet_context * context = lua_touserdata(L, lua_upvalueindex(1));

	int id = skynet_socket_listen(context, host, port, backlog);

	if (id < 0){
		return luaL_error(L, " listen error");
	}

	lua_pushinteger(L, id);
	return 1;
}

static int 
lstart(lua_State *L){
	int id = luaL_checkinteger(L, 1);
	struct skynet_context * context = lua_touserdata(L, lua_upvalueindex(1));
	skynet_socket_start(context, id);
	return 0;
}



int 
luaopen_skynet_core(lua_State *L){
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{"listen", llisten},
		{"start", lstart},
		{NULL, NULL}
	};

	lua_newlib(L, l);

	return 0;
}