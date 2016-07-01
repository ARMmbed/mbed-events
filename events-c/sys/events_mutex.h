/* 
 * System specific mutex implementation
 */
#ifndef EVENTS_MUTEX_H
#define EVENTS_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis.h"


// Mutex type
//
// If this type is safe in interrupt contexts, then
// the associated event queue will also be safe in
// interrupt contexts.
typedef unsigned events_mutex_t;

// Mutex operations
static inline int events_mutex_create(events_mutex_t *m) { return 0; }
static inline void events_mutex_destroy(events_mutex_t *m) {}

static inline void events_mutex_lock(events_mutex_t *m) {
    *m = __get_PRIMASK();
    __disable_irq();
}

static inline void events_mutex_unlock(events_mutex_t *m) {
    __set_PRIMASK(*m);
}


#ifdef __cplusplus
}
#endif

#endif
