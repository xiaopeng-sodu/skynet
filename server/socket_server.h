#ifndef socket_server_h
#define socket_server_h 

struct socket_server * socket_server_create();



struct socket_object_interface{
	void *(*buffer)(void *);
	int (*size)(void *);
	void (*free)(void *);
};


#endif