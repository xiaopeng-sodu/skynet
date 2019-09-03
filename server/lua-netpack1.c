#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <malloc.h>
#include <assert.h>
#include <string.h>

#define QUEUESIZE 1024
#define HASHSIZE 4096
#define SMALLSTRING 2048

#define TYPE_DATA  1
#define TYPE_MORE  2
#define TYPE_ERROR 3
#define TYPE_OPEN  4
#define TYPE_CLOSE 5
#define TYPE_WARNING 6 

struct netpack {
	int fd; 
	int size;
	void * buffer;
};

struct uncomplete{
	struct netpack pack;
	struct uncomplete * next;
	int read; 
	int header;
};

struct queue {
	int cap; 
	int head; 
	int tail;
	struct uncomplete * hash[]
};



static struct uncomplete *
find_uncomplete(struct queue * q, int fd){

}

static int 
filter_data_(lua_State *L, int fd, char * buffer, int size){
	struct queue * q = lua_touserdata(L, 1);


}

static int 
filter_data(lua_State *L, int fd, char * buffer, int size){
	int ret = filter_data_(L, fd, buffer, size);
	free(buffer);
	return ret;
}

static int 
lfilter(lua_State *L){
	struct skynet_socket_message *message = lua_touserdata(L, 2);
	int size = luaL_checkinteger(L, 3);
	char * buffer = message->buffer;
	if (buffer == NULL){
		buffer = (char *)(message + 1);
		size -= sizeof(*message);
	}else{
		size = -1;
	}

	lua_settop(L, -1);

	switch(message->type){
		case SKYENT_SOCKET_TYPE_DATA: 
			assert(size == -1);
			return filter_data(L, message->id, (char *)buffer, message->ud);
		case SKYENT_SOCKET_TYPE_CONNECT: 


	}

}


static int 
luaopen_netpack(lua_State *L){
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"pop", lpop},
		{NULL, NULL}
	};

	luaL_newlib(L, l);

	lua_pushliteral(L, "data");
	lua_pushliteral(L, "more");
	lua_pushliteral(L, "error");
	lua_pushliteral(L, "open");
	lua_pushliteral(L, "close");
	lua_pushliteral(L, "warning");


	lua_pushcclosure(L, lfilter, 6);
	lua_setfield(L, -2 , "filter");

	return 1;
}