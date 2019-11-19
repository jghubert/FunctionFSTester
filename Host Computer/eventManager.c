#include "eventManager.h"
#include <errno.h>
#include <pthread.h>
#include <time.h>


bool EventManagerCreate(EventManager* em)
{
	em->events = 0;

	if (pthread_mutex_init(&(em->lock), NULL) != 0) {
		return false;
	}
	if (pthread_cond_init(&(em->cond_lock), NULL) != 0) {
		pthread_mutex_destroy(&(em->lock));
		return false;
	}
	return true;
}

void EventManangerDestroy(EventManager* em)
{
	pthread_mutex_lock(&(em->lock));

	em->events = 0xFFFFFFFF;   // Should wake up all blocked threads
	pthread_cond_broadcast(&(em->cond_lock));

	pthread_cond_destroy(&(em->cond_lock));

	pthread_mutex_unlock(&(em->lock));
	pthread_mutex_destroy(&(em->lock));

}

bool EventManagerGet(EventManager* em, uint32_t flag, EventManagerOperations_t getOption, uint32_t* flag_p, uint32_t waitOption)
{
	pthread_mutex_lock(&(em->lock));

	bool ret = false;
	// long unsigned timeCount = 0;
	uint32_t status = 0;
	struct timespec timeToWait = {
		.tv_sec = 0,
		.tv_nsec = 1000
	};

	do {
		status = em->events & flag;
		if (getOption == EM_AND || getOption == EM_AND_CLEAR)
			ret = (status == flag);
		else if (getOption == EM_OR || getOption == EM_OR_CLEAR)
			ret = (status > 0);

		if (!ret) {
			if (waitOption == NO_WAIT) {
				pthread_mutex_unlock(&em->lock);
				return false;
			}
			else {
				if (waitOption == WAIT_FOREVER)
					pthread_cond_wait(&em->cond_lock, &em->lock);
				else {
					timeToWait.tv_sec = time(NULL) + (long)(waitOption / 1000000.0);
					timeToWait.tv_nsec = 1000 * waitOption % 1000000;
					if (pthread_cond_timedwait(&em->cond_lock, &em->lock, &timeToWait) == ETIMEDOUT) {
					// if (!q->count) {
						pthread_mutex_unlock(&em->lock);
						return false;
					}
				}
			}
		}
	} while (!ret);

	// do {
	// 	status = em->events & flag;
	// 	if (getOption == EM_AND || getOption == EM_AND_CLEAR)
	// 		ret = (status == flag);
	// 	else if (getOption == EM_OR || getOption == EM_OR_CLEAR)
	// 		ret = (status > 0);

	// 	if (!ret && waitOption) {
	// 		timeCount = (timeCount+1) % 0xFFFFFFFF;
	// 		nanosleep(&timeToWait, NULL);
	// 	}

	// }while (!ret && timeCount < waitOption);

	if (flag_p)
		(*flag_p) = em->events;
	if (ret && (getOption == EM_AND_CLEAR || getOption == EM_OR_CLEAR)) {
		em->events &= !flag;
	}

	pthread_mutex_unlock(&(em->lock));
	return ret;
}

bool EventManagerSet(EventManager* em, uint32_t flag, EventManagerOperations_t setOption)
{
	pthread_mutex_lock(&(em->lock));

	if (setOption == EM_AND) 
		em->events = flag;
	else if (setOption == EM_OR)
		em->events |= flag;

	pthread_mutex_unlock(&(em->lock));
	pthread_cond_broadcast(&(em->cond_lock));
	return true;
}
