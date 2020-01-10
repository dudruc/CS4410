#include "rthread.h"

#define MIDDLE 0
#define HIGH 1
#define NLANES 7

// My solution is based on the comparision between the level of the people in the pool
// and the level of the first person in the queue. In the entry function,
// firstly I put every person into the queue. If the pool is not empty 
// and the comparision above is different, the person would call wait. 
// In the exit function, if the person in the pool exits and the pool is empty,
// the first people can enter the pool and also the following people 
// that has the same level as the first person. 
// My solution can keep the order between high school and middle school in the queue,
// but not the order within the high schooler and middle schooler.
// For example, if now m1 in the pool and h1 m2 h2 in the queue. 
// When m1 left. the notify would only notify the high schooler type. 
// So it is possible that h2 get notified instead of h1. In this situation, 
// h2 would get into the pool first and h1 would take h2's spot in the queue. 
// In all, although my solution can't make sure the sequence of h1 and h2, 
// it can make sure the order of getting into the pool would be h, m and h.

struct pool
{
    rthread_lock_t lock;
    // you can add more monitor variables here
    int queue[NLANES*2];
    int in;
    int out;
    int cnt;
    int lanecnt;
    int ppllevel;
    rthread_cv_t ppl_cv;
    rthread_cv_t high_cv;
    rthread_cv_t mdl_cv; 
};

void pool_init(struct pool *pool)
{
    memset(pool, 0, sizeof(*pool));
    rthread_lock_init(&pool->lock);
    // initialize your monitor variables here
    rthread_cv_init(&pool->ppl_cv, &pool->lock);
    rthread_cv_init(&pool->high_cv, &pool->lock);
    rthread_cv_init(&pool->mdl_cv, &pool->lock);

    pool->in = pool->out = pool->cnt = pool->lanecnt = pool->ppllevel = 0;
}
void pool_enter(struct pool *pool, int level)
{   
    rthread_with(&pool->lock)
    {
        // write the code here to enter the pool
	pool->queue[pool->in] = level;
	pool->in = (pool->in + 1) % (NLANES * 2);
	pool->cnt++;
	while (pool->lanecnt != 0 && pool->ppllevel != pool->queue[pool->out]){
		if (level == MIDDLE){
			rthread_cv_wait(&pool->mdl_cv);
		} else if (level == HIGH){
			rthread_cv_wait(&pool->high_cv);
		}
	}
	if (pool->lanecnt==0){
		pool->ppllevel = pool->queue[pool->out];
	}
	pool->out = (pool->out + 1) % (NLANES*2);
	pool->cnt--;
	pool->lanecnt++;
	rthread_cv_notify(&pool->ppl_cv);
    }
}
void pool_exit(struct pool *pool, int level)
{ 
    int ppl; 
    rthread_with(&pool->lock){
        // write the code here to exit the pool
	while(pool->lanecnt==0){
		rthread_cv_wait(&pool->ppl_cv);
	}
	pool->lanecnt--;
	if (pool->lanecnt == 0 && pool->cnt!=0){
		ppl = pool->queue[pool->out];
		int i=0;
		while (pool->cnt - i!=0 && ppl == pool->queue[(pool->out + i)%(NLANES*2)]){
			if (ppl == MIDDLE){
				rthread_cv_notify(&pool->mdl_cv);
			} else if (ppl == HIGH) {
				rthread_cv_notify(&pool->high_cv);
			}
			i++;
		}
	}
    }
}
