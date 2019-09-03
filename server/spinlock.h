#ifndef spinlock_h
#define spinlock_h


struct spinlock_t {
	int lock; 
};

static void
spinlock_init(struct spinlock_t * lock){
	lock->lock = 0;
}

static void 
spinlock_lock(struct spinlock_t * lock){
	while(__sync_lock_test_and_set(&lock->lock, 1)){}
}

static int
spinlock_trylock(struct spinlock_t * lock){
	return __sync_lock_test_and_set(&lock->lock, 1) == 0;
}

static void
spinlock_unlock(struct spinlock_t * lock){
	__sync_lock_release(&lock->lock);
}

static void 
spinlock_destroy(struct spinlock_t * lock){
	(void)lock;
}

#endif