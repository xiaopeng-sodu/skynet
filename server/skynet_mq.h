#ifndef skynet_mq_h
#define skynet_mq_h

static void skynet_globalmq_init();
static struct message_queue * skynet_mq_create(int handle);
static void skynet_globalmq_push(struct message_queue *mq);
static struct message_queue * skynet_globalmq_pop();
static void skynet_mq_push(struct message_queue *mq, struct skynet_message *message);
static int skynet_mq_pop(struct message_queue *mq, struct skynet_message *message);
static int skynet_mq_handle(struct message_queue * mq);
static int skynet_mq_length(struct message_queue * mq);
static int skynet_mq_overload(struct message_queue * mq);

void skynet_mq_mark_release(struct message_queue *mq);
typedef void (*message_drop)(struct skynet_message * , void *);
static void skynet_mq_release(struct message_queue * mq, message_drop * drop_func, void * ud);

#endif