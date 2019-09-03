
#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#define BUF_SIZE_INIT  1024
#define MAX_SIZE 0x10000

enum = {write = 0, read = 1};
struct buffer {
	void * buf_start_;
	void * buf_cur_;
	void * buf_max_;

	int buf_size_;
	char mod_;
};


static int 
check_size(struct buffer * buff, int size){
	if (buff->buf_cur_ + size > buff->buf_start_ + buff->buf_size_){
		int sz = buff->buf_size_ * 2;
		for(;buff->buf_cur_ + size > buff->buf_start_ + sz){
			if (sz > MAX_SIZE){
				return 1;
			}
		}
		void * tmp = malloc(sz);
		int len = buff->buf_cur_ - buff->buf_start_;
		memcpy(tmp, buff->buf_start_, len);
		free(buff->buf_start_);
		buff->buf_start_ = tmp;
		buff->buf_cur_ = buff->buf_start_ + len;
		buff->buf_max_ = tmp;
		buff->buf_size_ = sz;
		buff->mode_ = write;
		return 0;
	}
	return 0;
}

static int 
check_buff(struct buffer * buff, int size){
	return (buff->buf_cur_ + size) > buff->buf_max_;
}

static int 
write_int8(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(char);
	if (check_size(buff, len)){
		return luaL_error(L, "too long  data %d" , len );
	}

	char val = luaL_checkinteger(L, 2);
	*buff->buf_cur_++ = val;
	return 0;
}

static int 
write_int16(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(unsigned short);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	unsigned short val = lua_tointeger(L, 2);
	buff->buf_cur_[0] = (val >> 8) & 0xff;
	buff->buf_cur_[1] = val & 0xff;
	buff->buf_cur_ += sizeof(unsigned short);
	return 0;
}

static int
write_int32(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(int);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	int val = lua_tointeger(L, 2);
	*buff->buf_cur_++ = (val >> 24) & 0xff;
	*buff->buf_cur_++ = (val >> 12) & 0xff;
	*buff->buf_cur_++ = (val >> 8)  & 0xff;
	*buff->buf_cur_++ = val & 0xff;

	return 0;
}

static int 
write_int64(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(long long);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d ", len);
	}

	long long val = lua_tointeger(L, 2);
	*buff->buf_cur_ ++ = (val >> 56) & 0xff;
	*buff->buf_cur_ ++ = (val >> 48) & 0xff;
	*buff->buf_cur_ ++ = (val >> 40) & 0xff;
	*buff->buf_cur_ ++ = (val >> 32) & 0xff;
	*buff->buf_cur_ ++ = (val >> 24) & 0xff;
	*buff->buf_cur_ ++ = (val >> 16) & 0xff;
	*buff->buf_cur_ ++ = (val >> 8) & 0xff;
	*buff->buf_cur_ ++ = val & 0xff;

	return 0;
}

static int 
load(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	const char * ptr = luaL_checkstring(L, 2);
	int size = luaL_checkinteger(L, 3);

	buff->buf_cur_ = buff->buf_start_;
	if (check_size(buff, size )){
		return luaL_error(L, "too long data %d", size);
	}

	memcpy(buff->buf_cur_, ptr, size);
	buff->buf_cur_ = buff->buf_start_ + size;
	buff->mode_ = read;

	return 0;
}

static int 
load_string(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = 0;
	const char * ptr = luaL_checklstring(L, 2, &len);

	buff->buf_cur_ = buff->buf_start_;
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	memcpy(buff->buf_cur_ , ptr, len);
	buff->buf_cur_ = buff->buf_start_ + len; 
	buff->mode = read; 

	return 0;
}

static int 
read_int8(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(char);
	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	char val = *buff->buf_cur_ ++;
	lua_pushinteger(L, val);
	return 1;
}

static int 
read_int16(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(short);
	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	short val = (buff->buf_cur_[0] << 8) | (buff->buf_cur_[1]);
	lua_pushinteger(L, val);

	return 1;
}

static int 
read_int32(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(int);
	if (check_buff(buff, len)){
		return  luaL_error(L, "too long data %d", len);
	}

	int val = buff->buf_cur_[0];
	val = (val << 8) | (buff->buf_cur_[1]);
	val = (val << 8) | (buff->buf_cur_[2]);
	val = (val << 8) | (buff->buf_cur_[3]);

	buff->buf_cur_ += sizeof(int);
	lua_pushinteger(L, val);
	return 1;
}

static int 
read_string(lua_State *L){
	struct buffer *buff = luaL_checkudata(L, 1, "buffer");

	int len = sizeof(short);
	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d ", len);
	}

	short sz = (buff->buf_cur_[0] << 8) | (buff->buf_cur_[1]);	
	buff->buf_cur_ += sizeof(short);

	if (check_buff(buff, sz)){
		return luaL_error(L, "too  long data %d ", sz);
	}

	lua_pushstring(L, (const char *)buff->buf_cur_, sz);
	buff->buf_cur_ += sz;

	return 1;
}

static int 
new_buffer(lua_State *L){
	struct buffer * buff = lua_newuserdata(L, sizeof(*buffer));
	void * tmp = malloc(sizeof(*buffer));
	buff->buf_start_ = tmp;
	buff->buf_cur_ = tmp;
	buff->buf_max_ = tmp;
	buff->buf_size_ = BUF_SIZE_INIT;
	buff->mod_ = write;

	if (lua_newmetatable(L, "buffer")){
		luaL_Reg funcs[] = {
			{"write_int8", write_int8},
			{NULL, NULL}
		}

		luaL_newlib(L, funcs);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tostring);
		lua_setfield(L, -2, "__tostring")
	}

	lua_setmetatable(L, -2)
	return 0;
}

int luaopen_buffer(lua_State *L){
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{"new", new_buffer}, 
		{NULL, NULL}
	}

	luaL_newlib(L, l);

	return 1;
}