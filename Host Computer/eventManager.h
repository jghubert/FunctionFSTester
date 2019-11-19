#ifndef __IAP2_EVENT_MANAGER_H__
#define __IAP2_EVENT_MANAGER_H__

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
	uint32_t events;
	pthread_mutex_t lock;
	pthread_cond_t cond_lock;
} EventManager;

typedef enum EventManagerOperations {
	EM_AND = 0,
	EM_AND_CLEAR,
	EM_OR,
	EM_OR_CLEAR
} EventManagerOperations_t;

#ifndef NO_WAIT
#define NO_WAIT 0x00
#endif

#ifndef WAIT_FOREVER
#define WAIT_FOREVER 0xFFFFFFFF
#endif

bool EventManagerCreate(EventManager* em);
void EventManangerDestroy(EventManager* em);
bool EventManagerGet(EventManager* em, uint32_t flag, EventManagerOperations_t getOption, uint32_t* flag_p, uint32_t waitOption);
bool EventManagerSet(EventManager* em, uint32_t flag, EventManagerOperations_t setOption);


#endif