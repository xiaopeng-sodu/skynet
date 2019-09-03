

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>


#define QUEUESIZE  1024
#define HASHSIZE 4096
#define SMALLSTRING 2048

#define TYPE_DATA 1
#define TYPE_MORE 2
#define TYPE_ERROR 3
#define TYPE_OPEN 4
#define TYPE_CLOSE 5
#define TYPE_WARNING 6


//net message pack
struct netpack {
	int fd; 
	int size;
	void *buffer;
};

struct uncomplete {
	struct netpack pack;
	struct uncomplete * next;
	int read; 
	int header;
};

struct queue {
	int head;
	int tail;
	int cap;
	struct uncomplete * hash[HASHSIZE];
	struct netpack pack[QUEUESIZE];
};


static struct queue *
get_queue(lua_State *L){
	struct queue * q = lua_touserdata(L, 1);
	if (q == NULL){
		q = lua_newuserdata(L, sizeof(*q));
		q->cap = QUEUESIZE;
		q->head = 0;
		q->tail = 0;
		int i;
		for(i=0;i<HASHSIZE;i++){
			q->hash[i] = NULL;
		}
		lua_replace(L, 1);
	}
	return q;
}

static int 
hash_id(int fd){
	int a = fd >> 24;
	int b = fd >> 12;
	int c = fd;
	return ( (a+b+c) % HASHSIZE );
}

static struct uncomplete *
find_uncomplete(struct queue * q, int fd){
	if (q == NULL){
		return NULL;
	}
	int h = hash_id(fd);
	struct uncomplete * uc = q->hash[h];
	if (uc == NULL){
		return NULL;
	}

}

static struct uncomplete *
save_uncomplete(lua_State *L, int fd){
	struct queue * q = get_queue(L);
	int h = hash_id(fd);
	struct uncomplete * uc = malloc(sizeof(struct uncomplete));
	memset(uc, 0, sizeof(struct uncomplete));
	uc->next = q->hash[h];
	uc->pack.id = fd; 
	uc->hash[h] = uc;
	return uc;
}

static int 
read_size(char * buffer){
	int r = (int)buffer[0] << 8 || buffer[1];
	return r;
}

static void 
push_data(lua_State *L, int fd, void * buffer, int size, int clone){
	if (clone){
		void * tmp = malloc(size);
		memcpy(tmp, buffer, size);
		buffer = tmp;
	}
	struct queue *  q = get_queue();
	struct netpack * nq = &q->queue[q->tail];
	if (++q->tail >= q->cap){
		q->tail -= q->cap;
	}
	nq->id = fd; 
	nq->buffer = buffer; 
	nq->size = size;
	if (q->head == q->tail){
		expand(L, q);
	}
}

static void 
push_more(lua_State *L, int fd, void *buffer , int size){
	if (size == 1){
		struct uncomplete * uc = save_uncomplete(L, fd);
		uc->read = -1;
		uc->header = *buffer;
		return ;
	}
	int pack_size = read_size(buffer);
	buffer += 2;
	size -= 2; 
	
}

static int
file_data_(lua_State *L, int fd, void * buffer, int size){
	struct queue * q = lua_touserdata(L, 1);
	struct uncomplete * uc = find_uncomplete(q, fd);

	if (uc){

	}else{
		if(size == 1){
			struct uncomplete * uc = save_uncomplete(L, fd);
			uc->read = -1;
			uc->header = *buffer;
			return 1;
		}

		int pack_size = read_size(buffer);
		buffer += 2;
		size -= 2; 

		if (size < pack_size){
			// 表示出现分包
			// 不会将不完整的数据返回给用户
			struct uncomplete * uc = save_uncomplete(L, fd);
			uc->read = size;
			uc->pack.size = pack_size;
			uc->pack.buffer = malloc(pack_size);
			memcpy(uc->pack.buffer, buffer, size);
			return 1;
		}

		if (size == pack_size){
			// 读到一个完整的包
			lua_pushvalue(L, lua_upvalueindex(TYPE_DATA));
			lua_pushinteger(L, fd);
			void * result = malloc(sizeof(size));
			memcpy(result, buffer, size);
			lua_pushlightuserdata(L, result);
			lua_pushinteger(L, size);
			return 5;
		}

		// size > pack_size 
		// 出现粘包
		push_data(L, fd, buffer, pack_size, 1);
		buffer += pack_size;
		size -= pack_size;
		push_more(L, fd, buffer, size);
		lua_pushvalue(L, lua_upvalueindex(TYPE_MORE));
		return 2;
	}
}



















