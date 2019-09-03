#include "socket_sever.h"
#include "skynet_epoll.h"

#include <sys/epoll.h>
#include <assert.h>
#include <sys/select.h>
#include <unistd.h>
#include <socket.h>
#include <sys/types.h>
#include <stdio.h>

#define MAX_SOCKET_P 16
#define MAX_SOCKET (1 << MAX_SOCKET_P)

#define UDP_ADDRESS_SIZE   19
#define MAX_UDP_PACKAGE   65535
#define MAX_EVENT 64
#define MAX_INFO 128


struct write_buffer {
	struct write_buffer * next;
	void * buffer; 
	char * ptr; 
	int sz; 
	bool userobject; 
	char udp_address[UDP_ADDRESS_SIZE];
};

struct wb_list{
	struct write_buffer * head; 
	struct write_buffer * tail; 
};

struct socket {
	struct wb_list high; 
	struct wb_list low; 
	int opaque; 
	int wb_sz; 
	int fd; 
	int id; 
	int protocol; 
	int type; 
	union {
		int size; 
		char udp_address[UDP_ADDRESS_SIZE];
	}p;
};

struct socket_server {
	int epfd; 
	int recvctrl_fd; 
	int writectrl_fd; 
	int checkctrl; 
	int alloc_id; 
	int event_index;
	int event_n; 
	struct fd_set rfds; 
	struct socket slot[MAX_SOCKET];
	struct event evs[MAX_EVENT];
	struct socket_object_interface obj; 
	struct buffer[MAX_INFO];
	char udpbuffer[MAX_UDP_PACKAGE];
};

struct request_listen{
	int id;
	int fd; 
	int opaque;
	char host[1];
};

struct request_start{
	int id; 
	int opaque;
};

struct request_package {
	char header[8];
	union {
		char buffer[256];
		struct request_listen listen; 
		struct request_start start; 

	}u;
	char dummy[256];
};

static inline void
clear_wb_list(struct wb_list * list){
	list->head = NULL;
	list->tail = NULL; 
}

struct socket_server * 
socket_server_create(){
	struct socket_server * ss = malloc(sizeof(*ss));
	memset(ss, 0, sizeof(*ss));

	int fd[2];
	if (pipe(fd)){
		fprintf(stderr, "pipe create failed ");
		return NULL;
	}
	int epfd = sp_create();
	if (sp_invalid(epfd)){
		fprintf(stderr, "epoll create failed");
		return NULL;
	}
	if (sp_add(epfd, fd[0], NULL)){
		fprintf(stderr, "fd[0] add epoll failed");
		return NULL;
	}

	ss->epfd = epfd;
	ss->recvctrl_fd = fd[0];
	ss->writectrl_fd = fd[1];
	ss->checkctrl = 1;
	ss->event_n = 0;
	ss->event_index = 0;
	ss->alloc_id = 0;

	int i; 
	for(i=0;i<MAX_SOCKET;i++){
		struct socket * s = &ss->slot[i];
		s->type = SOCKET_TYPE_INVALID; 
		clear_wb_list(&s->high);
		clear_wb_list(&s->low);
	}

	memset(&ss->soi, 0, sizeof(ss->soi));
	FD_ZERO(&ss->rfds);
	assert(ss->recvctrl_fd < FD_SETSIZE);

	return ss;
}

struct socket_server *
socket_server_create(){
	int fd[2];
	int epfd; 
	if (pipe(fd)){
		fprintf(stderr, "create pipe failed");
		return NULL;
	}
	epfd = sp_create()
	if (sp_invalid(epfd)){
		fprintf(stderr, "create epfd failed ");
		return NULL;
	}
	if (sp_add(epfd, fd[0], NULL)){
		fprintf(stderr, "sp_add fd[0] failed ");
		return NULL;
	}
	struct socket_server * ss = malloc(sizeof(*ss));
	memset
}
