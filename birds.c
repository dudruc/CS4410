#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rthread.h"

#define WHISTLER        0
#define LISTENER        1

struct device {
    /* These three semaphores should always sum to 1.
     */
    rthread_sema_t mutex;
    rthread_sema_t whistlerSema;
    rthread_sema_t listenerSema;

    /* Variables to keep track of the state.
     */
    int nWhistlersEntered, nWhistlersWaiting;
    int nListenersEntered, nListenersWaiting;
};

/* Initialize semaphores and state.
 */
void dev_init(struct device *dev){
    rthread_sema_init(&dev->mutex, 1);
    rthread_sema_init(&dev->whistlerSema, 0);
    rthread_sema_init(&dev->listenerSema, 0);
    dev->nWhistlersEntered = dev->nWhistlersWaiting = 0;
    dev->nListenersEntered = dev->nListenersWaiting = 0;
}

/* Implementation of 'V hat'
 */
void dev_vacateOne(struct device *dev){
    /* If there are no listeners in the critical section and there are whistlers waiting,
     * release one of the waiting whistlers.
     */
    if (dev->nListenersEntered == 0 && dev->nWhistlersEntered >= 0 && 
		dev->nWhistlersWaiting > 0) {
        dev->nWhistlersWaiting--;
        rthread_sema_vacate(&dev->whistlerSema);
    }

    /* If there's nobody in the critical section and there are listeners waiting,
     * release one of the listeners.
     */
    else if (dev->nWhistlersEntered == 0 && dev->nListenersEntered >= 0 &&
                dev->nListenersWaiting > 0) {
        dev->nListenersWaiting--;
        rthread_sema_vacate(&dev->listenerSema);
    }

    /* If neither is the case, just stop protecting the shared variables.
     */
    else {
        rthread_sema_vacate(&dev->mutex);
    }
}

void dev_enter(struct device *dev, int which){
	if(which==1){
		rthread_sema_procure(&dev->mutex);

    		if (dev->nListenersEntered > 0) {
        		dev->nWhistlersWaiting++;
        		dev_vacateOne(dev);
        		rthread_sema_procure(&dev->whistlerSema);
    		}

    		assert(dev->nListenersEntered == 0);
    		dev->nWhistlersEntered++;
    		dev_vacateOne(dev);
	} else {
		rthread_sema_procure(&dev->mutex);

    		if (dev->nWhistlersEntered > 0) {
        	dev->nListenersWaiting++;
        	dev_vacateOne(dev);
        	rthread_sema_procure(&dev->listenerSema);
    		}

    		assert(dev->nWhistlersEntered == 0);
    		dev->nListenersEntered++;
    		dev_vacateOne(dev);
	}
}

void dev_exit(struct device *dev, int which){
	if(which==1){
		rthread_sema_procure(&dev->mutex);
    		assert(dev->nListenersEntered == 0);
    		dev->nWhistlersEntered--;
    		dev_vacateOne(dev);
	}else{
		rthread_sema_procure(&dev->mutex);
    		assert(dev->nWhistlersEntered == 0);
    		dev->nListenersEntered--;
    		dev_vacateOne(dev);
	}
}

#define NWHISTLERS        3
#define NLISTENERS        3
#define NEXPERIMENTS      2

char *whistlers[NWHISTLERS] = { "w1", "w2", "w3" };
char *listeners[NLISTENERS] = { "l1", "l2", "l3" };

void worker(void *shared, void *arg){
	struct device *dev = shared;
	char *name = arg;
	
	for (int i = 0; i < NEXPERIMENTS; i++) {
		printf("worker %s waiting for device\n", name);
		dev_enter(dev, name[0] == 'w');
		printf("worker %s has device\n", name);
		rthread_delay(random() % 3000);
		printf("worker %s releases device\n", name);
		dev_exit(dev, name[0] == 'w');
		rthread_delay(random() % 3000);
	}
	printf("worker %s is done\n", name);
}

int main(){
	struct device dev;

	dev_init(&dev);
	for (int i = 0; i < NWHISTLERS; i++) {
		rthread_create(worker, &dev, whistlers[i]);
	}
	for (int i = 0; i < NLISTENERS; i++) {
		rthread_create(worker, &dev, listeners[i]);
	}
	rthread_run();
	return 0;
}
