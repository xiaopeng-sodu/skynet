#include "skynet_mq.h"
#include "spinlock.h"

#include <malloc.h>
#include <string.h>
#include <assert.h>

#define IN_GLOBAL  1
#define MESSAGE_INIT_SIZE 64
#define MQ_OVERLOAD_THREASHOLD 64

struct skynet_message {
	int session ; 
	int source; 
	void *data;
	int size;
};

struct message_queue {
	struct spinlock_t lock; 
	int head; 
	int tail; 
	int cap;
	int overload; 
	int overload_threashold;
	int in_global; 
	int release; 
	int handle;
	struct skynet_message *queue; 
	struct message_queue * next;
};

struct global_queue {
	struct message_queue * head; 
	struct message_queue * tail; 
	struct spinlock_t lock;
};

struct global_queue * Q = NULL;

static struct message_queue * 
skynet_mq_create(int handle){
	struct message_queue * mq = malloc(sizeof(*mq));
	memset(mq, 0, sizeof(*mq));
	SPIN_INIT(mq)
	mq->head = 0;
	mq->tail = 0;
	mq->cap = MQ_OVERLOAD_THREASHOLD;
	mq->in_global = 0; 
	mq->overload = 0;
	mq->overload_threashold = MQ_OVERLOAD_THREASHOLD;
	mq->release = 0;
	mq->handle = handle; 
	mq->queue = malloc(sizeof(struct skynet_message) * MESSAGE_INIT_SIZE);
	memset(mq->queue, 0, sizeof(*mq->queue));
	mq->next = NULL;
	return mq;
}

inline void
expand_queue(struct message_queue *mq){
	struct skynet_message * new_queue = malloc(mq->cap * 2);
	memset(new_queue, 0, sizeof(*new_queue));
	int i; 
	for(i=0;i<mq->cap; i++){
		new_queue[i] = mq->queue[mq->head++ % mq->cap];
	}
	free(mq->queue);
	mq->queue = new_queue;
	mq->head = 0;
	mq->tail = mq->cap; 
	mq->cap *= 2; 
}

static void 
skynet_mq_push(struct message_queue *mq, struct skynet_message *message){
	SPIN_LOCK(mq)
	mq->queue[mq->tail++] = *message;
	if (mq->tail >= mq->cap){
		mq->tail = 0;
	}
	if(mq->head == mq->tail){
		expand_queue(mq);
	}
	if (mq->in_global == 0){
		mq->in_global = IN_GLOBAL;
		skynet_globalmq_push(mq);
	}
	SPIN_UNLOCK(mq)
}

static int 
skynet_mq_pop(struct message_queue *mq, struct skynet_message *message){
	SPIN_LOCK(mq)
	int ret = 1;
	if (mq->head != mq->tail){
		ret = 0;
		*message = mq->queue[mq->head++];
		if (mq->head >= mq->cap){
			mq->head = 0;
		}
		int len = mq->tail - mq->head; 
		if (len < 0){
			len = mq->cap - (mq->tail - mq->head);
		}
		while (len > mq->overload_threashold){
			mq->overload = len;
			mq->overload_threashold *=2; 
		}
	}else{
		mq->overload_threashold = MQ_OVERLOAD_THREASHOLD;
	}

	if (ret){
		mq->in_global = 0;
	}
	SPIN_UNLOCK(mq)
}

static void 
skynet_globalmq_push(struct message_queue *mq){
	struct global_queue * q = Q; 
	assert(q != NULL);

	SPIN_LOCK(q)
	if (q->tail){
		q->tail->next = mq;
		q->tail = q->tail->next;
	}else{
		assert(q->head == NULL);
		q->head = q->tail = mq;
	}
	q->tail->next = NULL;
	SPIN_UNLOCK(q)
}


static int 
skynet_mq_handle(struct message_queue * mq){
	SPIN_LOCK(mq)
	int handle = mq->handle;
	SPIN_UNLOCK(mq)
	return handle;
}

static int 
skynet_mq_length(struct message_queue * mq){
	SPIN_LOCK(mq)
	int len = mq->tail - mq->head;
	if (len < 0){
		len = mq->cap - (mq->head - mq->tail);
	}
	SPIN_UNLOCK(mq)
	return len;
}

static int 
skynet_mq_overload(struct message_queue * mq){
	if (mq->overload){
		int overload = mq->overload ;
		mq->overload = 0;
		return overload;
	}else{
		return 0;
	}
}

static struct message_queue * 
skynet_globalmq_pop(){
	struct global_queue * q = Q; 
	assert(q != NULL);

	SPIN_LOCK(q)
	struct message_queue * head = q->head; 
	if (head){
		q->head = q->head->next;
		head->next = NULL;
		if (q->tail == NULL){
			assert(q->head == NULL);
		}
	}else{
		assert(q->tail == NULL);
	}
	SPIN_UNLOCK(q)
	return head;
}


static void 
skynet_globalmq_init(){
	struct global_queue * q = malloc(sizeof(*q));
	memset(q, 0, sizeof(*q));

	q->head = NULL;
	q->tail = NULL;
	SPIN_INIT(q)

	Q = q;
}


void 
skynet_mq_mark_release(struct message_queue *mq){
	SPIN_LOCK(mq)
	assert(mq->release == 0);
	mq->release = 1;
	if (mq->in_global != IN_GLOBAL){
		skynet_gloablmq_push(mq);
	}
	SPIN_UNLOCK(mq)
}

void 
_release(struct message_queue *q){
	assert(q->next == NULL);
	SPIN_DESTROY(q)
	free(q->queue);
	free(q);
}

static void 
_drop_queue(struct message_queue * q, message_drop * drop_func, void * ud){
	struct skynet_message message;
	while(!skynet_mq_pop(q, &message)){
		drop_func(&message, ud);
	}
	_release(q);
}

static void 
skynet_mq_release(struct message_queue * mq, message_drop * drop_func, void * ud){
	SPIN_LOCK(mq)

	if (mq->release){
		SPIN_UNLOCK(mq)
		_drop_queue(q, drop_func, ud);
	}else{
		skynet_globalmq_push(mq);
		SPIN_UNLOCK(mq)
	}
}
