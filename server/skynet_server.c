#include "skynet_server.h"




struct skynet_context {
	struct skynet_module * mod; 
	void * instance; 
	FILE * logifle; 
	struct message_queue * queue; 
	int handle; 
	skynet_cb cb; 
	void * cb_ud; 
	char result[32];
	int ref; 
	int release; 
	struct spinlock_t lock; 
};


