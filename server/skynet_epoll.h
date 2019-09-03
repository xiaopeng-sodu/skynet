#ifndef skynet_epoll_h
#define skynet_epoll_h

#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

struct event {
	void * data;
	bool write; 
	bool read; 
};

int 
sp_create(){
	return epoll_create(1024);
}

int 
sp_invalid(int epfd){
	if (epfd <= 0){
		return 1;
	}
	return 0;
}

void
sp_releae(int sock){
	close(sock);
}

int 
sp_add(int epfd, int sock, void *ud){
	struct epoll_event event; 
	memset(&event, 0, sizeof(event));
	event.events = EPOLLIN; 
	event.data.ptr = ud;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event)){
		return -1;
	}
	return 0;
}

int 
sp_write(int epfd, int sock, void *ud, bool enable){
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.events = EPOLLIN | (enable ? EPOLLOUT : 0);
	event.data.ptr = ud;
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &event)){
		return -1;
	}
	return 0;
}

void
sp_del(int epfd, int sock){
	epoll_ctl(epfd, EPOLL_CTL_DEL, sock , NULL);
}

int
sp_wait(int epfd, struct event * evs, int max){
	struct epoll_event events[max];
	int n = epoll_wait(epfd, events, max, -1);
	if (n > 0){
		int i;
		for(i=0;i<n;i++){
			evs[i].data = events[i].data.ptr;
			unsigned char flag = events[i].events;
			evs[i].write = (flag & EPOLLOUT) != 0;
			evs[i].read = (flag & EPOLLIN) != 0;
		}
	}
	return n; 
}

void
sp_nonblocking(int sock){
	int flag = fcntl(sock, F_GETFL, 0);
	if (flag == -1){
		return;
	}

	flag |= O_NONBLOCK; 
	
	fcntl(sock, F_SETFL, flag);
}


#endif