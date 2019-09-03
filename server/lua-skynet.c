
#include "lua.h"
#include "lauxlib.h"
#include <assert.h>
#include <string.h>
#include <malloc.h>


static int
lcommand(lua_State *L ){
	struct skynet_context * ctx = lua_touserdata(L, lua_upvalueindex(1));

	const char * cmd = lua_checkstring(L, 1);
	const char *param = NULL;
	const char *result = NULL
	if(lua_gettop(L) == 2){
		param = luaL_checklstring(L, 2);
	}
	result = skynet_command(L, cmd, param);
	if (result){
		lua_pushstring(L, result);
		result 1;
	}
	return 0;
}


static const char *
get_dest_string(lua_State *L, int index){
	const char * dest_string = lua_tostring(L, index)
	return dest_string;
}

static int 
lsend(lua_State *L){
	struct skynet_context * ctx = lua_touserdata(L, lua_upvalueindex(1));
	int dest = lua_tointeger(L, 1);
	const char * dest_string = NULL;
	if (dest == 0){
		if (lua_type(L, 1) == LUA_TNUMBER){
			return luaL_error("invalid lua_type");
		}
		dest_string = get_dest_string(L, 1);
	}

	int type = lua_checkinteger(L, 2);
	int session = 0;
	if (lua_isnil(L, 3)){
		type |= PTYPE_TAG_ALLACSESSION;
	}else{
		session = luaL_checkinteger(L, 3);
	}

	int mtype = lua_type(L, 4);
	switch(mtype){
		case LUA_TSTRING: {
			int len = 0;
			void *msg = lua_tolstring(L, 4, &len);
			if (len == 0){
				msg = NULL;
			}
			if (dest_string ){
				session = skynet_sendname(ctx, 0, dest_string, type,  session, msg, len);
			}else{
				session = skynet_send(ctx, 0, dest, type, session, msg, len);
			}
			break;
		}
		case LUA_TLIGHTUSERDATA:{
			void *msg = lua_touserdata(L, 4);
			int size = lua_tointeger(L, 5);
			if (dest_string ){
				session = skynet_sendname(ctx, 0, dest_string, type| PTYPE_TAG_DONTCOPY, session, msg, type);
			}else{
				session = skynet_send(ctx, 0, dest, type | PTYPE_TAG_DONTCOPY, session, msg, type);
			}
		}
		default:
	}

	if (session < 0){
		return 0;
	}

	lua_pushinteger(L, session);

	return 1;
}


static int 
lredirect(lua_State *L){
	struct skynet_context * ctx = lua_touserdata(L, lua_upvalueindex(1));
	int dest = lua_tointeger(L, 1);
	const char * dest_string = NULL;
	if (dest == 0){
		dest_string = get_dest_string(L, 1);
	}
	int source = luaL_checkinteger(L, 2);
	int type = luaL_checkinteger(L, 3);
	int session = luaL_checkinteger(L, 4);

	int mtype = lua_type(L, 5);
	switch (mtype){
		case LUA_TSTRING: {
			int len = 0;
			void * msg = (void*)lua_tolstring(L, 5, &len);
			if (len == 0){
				msg = NULL;
			}
			if (dest_string){
				session = skynet_sendname(ctx, source, dest_string, type, session, msg, len);
			}else{
				session = skynet_send(ctx, source, dest, type, session, msg, len);
			}
		}
		case LUA_TLIGHTUSERDATA: {
			void * msg = lua_touserdata(L, 5);
			int size = lua_checkinteger(L, 6);
			if (dest_string){
				session = skynet_sendname(ctx, source, dest_string, type | PTYPE_TAG_DONTCOPY, msg, size);
			}else{
				session = skynet_send(ctx, source, dest, type | PTYPE_TAG_DONTCOPY, msg, size);
			}
		}
		default:
			luaL_error(L, "invalid error type %s\n", lua_typename(L, mtype));
	}
	return 0;
}


int 
luaopen_skynet_core(lua_State *L){
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"send", lsend}, 
		{"redirect", lredirect},
		{NULL, NULL}, 
	};

	luaL_newlibtable(L, l);

	lua_getfield(L, LUA_REGISTRYINDEX, "skynet_context");
	struct skynet_context * context = lua_touserdata(L, -1);
	if (ctx == NULL){
		return luaL_error(L, "Init skynet_context server failed");
	}

	luaL_setfuncs(L, l, 1);

	return 1;
}