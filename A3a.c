#include "rthread.h"

#define MIDDLE 0
#define HIGH 1
#define NLANES 7

struct pool
{
    rthread_lock_t lock;
    // you can add more monitor variables here
    int counter;
    int ppllevel;
    rthread_cv_t lane_cv;
    rthread_cv_t ppl_cv;
    rthread_cv_t level_cv;
};

void pool_init(struct pool *pool)
{
    memset(pool, 0, sizeof(*pool));
    rthread_lock_init(&pool->lock);
    // initialize your monitor variables here
    pool->counter = pool->ppllevel = 0;
    rthread_cv_init(&pool->lane_cv, &pool->lock);
    rthread_cv_init(&pool->ppl_cv, &pool->lock);
    rthread_cv_init(&pool->level_cv, &pool->lock);
}

void pool_enter(struct pool *pool, int level)
{
    rthread_with(&pool->lock)
    {
        // write the code here to enter the pool
	while(pool->counter == NLANES)
		rthread_cv_wait(&pool->lane_cv);
	while(pool->counter!=0 && pool->ppllevel!=level)
		rthread_cv_wait(&pool->level_cv);
	if(pool->counter == 0)
		pool->ppllevel = level;
	pool->counter++;
	rthread_cv_notify(&pool->ppl_cv);
    }
}

void pool_exit(struct pool *pool, int level)
{
    rthread_with(&pool->lock)
    {
        // write the code here to exit the pool
	while(pool->counter==0)
		rthread_cv_wait(&pool->ppl_cv);
	pool->counter--;
	rthread_cv_notify(&pool->lane_cv);
	if(pool->counter==0)
		rthread_cv_notify(&pool->level_cv);
    }
}
