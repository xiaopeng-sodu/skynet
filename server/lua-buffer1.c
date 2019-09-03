#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#define BUF_INIT_SIZE   1024
#define MAX_BUF_SIZE    4096

enum {write = 0, read = 1};
struct buffer {
	void * buf_start_;
	void * buf_cur_;
	void * buf_max_;

	int buf_size_;
	int mode_;
};

// 当前可使用的缓存是否足够用来存储大小为size的数据
static int 
check_size(struct buffer * buff, int size){
	if (buff->buf_cur_ + size > buff->buf_start_ + buff->buf_size_){
		int sz = buff->buf_size_ * 2;
		for(;buff->buf_cur_ + size > buff->buf_start_ + sz;){
			sz *= 2;
			if (sz > MAX_BUF_SIZE){
				return 1;
			}
		}

		void *nb = malloc(sz);
		memset(nb, 0, sizeof(sz));
		int len = buff->buf_cur_ - buff->buf_start_;
		memcpy(nb, buff->buf_start_, len);
		free(buff->buf_start_);
		buff->buf_start_ = nb;
		buff->buf_cur_ = buff->buf_start_ + len;
		buff->buf_max_ = nb;
		buff->buf_size_ = len;
		return 0;
	}
	return 0;
}


static int 
check_buff(struct buffer * buff, int size){
	return (buff->buf_cur_ + size > buff->buf_max_);
}


static int 
load_string(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	buff->buf_cur_ = buff->buf_start_;
	int len = 0;
	const char * msg = lua_tolstring(L, 2, &len);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	memcpy(buff->buf_cur_, msg, len);
	buff->buf_max_ = buff->buf_start_ + len;
	buff->mode_ = read;

	return 0;
}

static int
load(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	void * msg = lua_touserdata(L, 2);
	int len = luaL_checkinteger(L, 3);

	if (msg == NULL){
		return luaL_error(L, "invalid msg");
	}

	buff->buf_cur_ = buff->buf_start_;
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	memcpy(buff->buf_cur_, msg, len);
	buff->buf_max_ = buff->buf_start_ + len;
	buff->mode_ = read;

	return 0;
}

static int 
read_int8(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(char);
	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}
	char val = *buff->buf_cur_++;
	lua_pushvalue(L, val);
	return 1;
}

static int 
read_int16(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = size(short);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}
	short val = *buff->buf_cur_++;
	val = (val << 8) |  *buff->buf_cur_ ++;
	lua_pushvalue(L, val);
	return 1;
}

static int 
read_int32(lua_State *L ){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(int);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}
	int val = *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	lua_pushvalue(L, val);
	return 1;
}

static int 
read_int64(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(long long int);
	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}
	long long val = *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;
	val = (val << 8) | *buff->buf_cur_++;

	lua_pushvalue(L, val);
	return 1;
}

static int 
read_string(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	if (check_buff(buff, sizeof(unsigned short))){
		return luaL_error(L, "too long data %d", len);
	}
	unsigned short len = (*buff->buf_cur_[0] << 8) | (*buff->buf_cur_[1])
	buff->buf_cur_ += sizeof(unsigned short);

	if (check_buff(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	if (len == 0){
		lua_pushlstring(L, "", 0);
	}else{
		lua_pushlstring(L, (const char *)buff->buf_cur_, len);
		buff->buf_cur_ += len; 
	}
	return 1;
}

static int 
write_int8(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(char);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}

	char val = lua_tointeger(L, 2);
	*buff->buf_cur_++ = val;
	return 1;
}

static int 
write_int16(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(short);
	if (check_size(buff, len)){
		return luaL_error(L, "too long data %d", len);
	}
	short val = lua_tointeger(L, 2);
	*buff->buf_cur_++ = (val >> 8) & 0xff;
	*buff->buf_cur_++ = val & 0xff;
	return 1;
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
	*buff->buf_cur_++ = (val >> 16) & 0xff;
	*buff->buf_cur_++ = (val >> 8) & 0xff;
	*buff->buf_cur_++ = val & 0xff;

	return 1; 
}

static int 
write_int64(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int len = sizeof(long long);
	if (check_size(buff, len)){
		return luaL_error(L, " too long data %d ", len);
	}

	int val = lua_tointeger(L, 2);
	*buff->buf_cur_++ = (val >> 56) & 0xff;
	*buff->buf_cur_++ = (val >> 48) & 0xff;
	*buff->buf_cur_++ = (val >> 40) & 0xff;
	*buff->buf_cur_++ = (val >> 32) & 0xff;
	*buff->buf_cur_++ = (val >> 24) & 0xff;
	*buff->buf_cur_++ = (val >> 16) & 0xff;
	*buff->buf_cur_++ = (val >> 8) & 0xff;
	*buff->buf_cur_++ = (val) & 0xff;

	return 1;
}

static int 
write_string(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");
	int sz = 0;
	const char * msg = lua_tolstring(L, -1, &sz);
	if (msg  == NULL){
		*buff->buf_cur_++ = 0;
		*buff->buf_cur_++ = 0;
	}

	if (sz > MAX_BUF_SIZE || check_size(buff, sz + sizeof(unsigned short))){
		return luaL_error(L, "too long data %d", sz + sizeof(unsigned short));
	}

	*buff->buf_cur_++ = (sz >> 8) & 0xff;
	*buff->buf_cur_++ = sz & 0xff;
	memcpy(buff->buf_cur_ , msg , sz);
	buff->buf_cur_ += sz;

	return 1;
}

static int 
flush_pt_type(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	if (buff->buf_start_ == 0){
		return luaL_error(L, "buff not exist");
	}

	buff->buf_cur_ = buff->buf_start_ + 2;
	buff->buf_max_ = buff->buf_start_;
	buff->mode_ = write;

	unsigned short val = (short)lua_tointeger(L, 2);
	buff->buf_cur_[0] = (val >> 8) & 0xff;
	buff->buf_cur_[1] = val & 0xff;
	buff->buf_cur_ += sizeof(unsigned short);

	return 0;
}

static int 
get_buffer(lua_State *L){
	struct buffer * buff = luaL_checkudata(L, 1, "buffer");

	int sz = buff->buf_cur_ - buff->buf_start_ - 2;

	buff->buf_start_[0] = (sz >> 8) & 0xff;
	buff->buf_start_[1] = sz & 0xff;

	lua_pushlightuserdata(L, buff->buf_start_);
	lua_pushinteger(L, sz+2);

	void * tmp = malloc(BUF_INIT_SIZE);
	buff->buf_start_ = tmp;
	buff->buf_cur_ = tmp;
	buff->buf_max_ = tmp;
	buff->buf_size_ = buff->buf_size_;
	buff->mode_ = write;

	return 2;
}

static int 
new_buffer(lua_State *L){
	struct buffer * buff = lua_newuserdata(L, sizeof(*buff));
	void *tmp = malloc(BUF_INIT_SIZE);
	buff->buf_start_ = tmp;
	buff->buf_cur_ = tmp;
	buff->buf_max_ = tmp;
	buff->buf_size_ = BUF_INIT_SIZE;
	buff->mode_ = write;

	if (luaL_newmetatable(L, "buffer")){
		luaL_Reg funcs[] = {
			{"write_int8", write_int8}, 
			{"write_int16", write_int16},
			{"write_int32", write_int32}, 
			{"write_int64", write_int64},
			{"write_string", write_string},
			{"flush_pt_type", flush_pt_type},
			{"read_int8", read_int8}, 
			{"read_int16", read_int16},
			{"read_int32", read_int32},
			{"read_int64", read_int64},
			{"read_string", read_string}, 
			{"load_string", load_string}, 
			{"load", load}, 
			{NULL, NULL}
		};

		luaL_newlib(L, funcs);
		lua_setfield(L, -2, "__index");
		lua_pushcfuntion(L, tostring);
		lua_setfield(L, -2, "__tostring");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int 
luaopen_buffer(lua_State *L){
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"new",new_buffer}, 
		{NULL, NULL}, 
	};

	luaL_newlib(L, l);
	return 1;
}