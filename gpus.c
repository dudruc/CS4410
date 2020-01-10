#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"

#define NGPUS    10

rthread_lock_t print_lock;

struct gpu_info {
	rthread_sema_t mutex;
	rthread_sema_t allocateSema;
	rthread_sema_t freeSema;

	int nWaiting;
	int allocated[NGPUS];
	unsigned int nfree;
};

void gi_init(struct gpu_info *gi){
	memset(gi->allocated, 0, sizeof(gi->allocated));
	gi->nfree=NGPUS;
	rthread_sema_init(&gi->mutex, 1);
	rthread_sema_init(&gi->allocateSema, 0);
	rthread_sema_init(&gi->freeSema, 0);
	gi->nWaiting=0;	
}

void gi_vacate(struct gpu_info *gi){
//	if(gi->nfree<NGPUS && gi->nWaiting==0){
//		rthread_sema_vacate(&gi->allocateSema);
//	}
//
//	else if (gi->nWaiting>0 && gi->nfree<NGPUS){
//		gi->nWaiting--;
//		rthread_sema_vacate(&gi->freeSema);
//	}
//	else {
//		rthread_sema_vacate(&gi->mutex);
//	}
	if (gi->nfree == NGPUS){
		rthread_sema_vacate(&gi->mutex);
	}else{
		rthread_sema_vacate(&gi->allocateSema);
	}
}

void gi_alloc(struct gpu_info *gi,
unsigned int ngpus,
/* OUT */ unsigned int gpus[]){
	rthread_sema_procure(&gi->mutex);
	
	if (gi->nfree<ngpus){
		gi->nWaiting++;
		//gi_vacate(gi);
		rthread_sema_vacate(&gi->allocateSema);
		rthread_sema_procure(&gi->allocateSema);
	}

	assert(ngpus <= gi->nfree);
	gi->nfree-=ngpus;

	unsigned int g = 0;
	for (unsigned int i = 0; g < ngpus; i++) {
		assert(i < NGPUS);

		if (!gi->allocated[i]) {
		gi->allocated[i] = 1;
		gpus[g++] = i;
		}
	}
	//gi_free(gi);
	rthread_sema_vacate(&gi->freeSema);
}

void gi_release(struct gpu_info *gi,
unsigned int ngpus,
/* IN */ unsigned int gpus[]){
	rthread_sema_procure(&gi->freeSema);
	assert(gi->nfree+ngpus<=NGPUS);
	for (unsigned int g = 0; g < ngpus; g++) {
		assert(gpus[g] < NGPUS);
		assert(gi->allocated[gpus[g]]);
		gi->allocated[gpus[g]] = 0;
	}
	gi->nfree += ngpus;
	assert(gi->nfree <= NGPUS);
	gi_vacate(gi);
}

void gi_free(struct gpu_info *gi){
}

void gpu_user(void *shared, void *arg) {
	struct gpu_info *gi = shared;
	unsigned int gpus[NGPUS];

	for (int i = 0; i < 5; i++) {
		rthread_delay(random() % 3000);
		unsigned int n = 1 + (random() % NGPUS);
		rthread_with(&print_lock)
			printf("%s %d wants %u gpus\n", arg, i, n);
		gi_alloc(gi, n, gpus);
		rthread_with(&print_lock) {
			printf("%s %d allocated:", arg, i);
			for (int i = 0; i < n; i++)
				printf(" %d", gpus[i]);
			printf("\n");
		}
		rthread_delay(random() % 3000);
		rthread_with(&print_lock)
			printf("%s %d releases gpus\n", arg, i);
		gi_release(gi, n, gpus);
	}
}

int main() {
	rthread_lock_init(&print_lock);
	struct gpu_info gi;
	gi_init(&gi);
	rthread_create(gpu_user, &gi, "Jane");
	rthread_create(gpu_user, &gi, "Joe");
	rthread_run();
	gi_free(&gi);
	return 0;
}
