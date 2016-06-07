/* 
 * System specific semaphore implementation
 */
#ifndef EVENTS_SEMA_H
#define EVENTS_SEMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>


// Semaphore type
//
// Optimal implementation is a binary semaphore,
// however a regular semaphore is sufficient.
typedef void *events_sema_t;

// Semaphore operations
int events_sema_create(events_sema_t *s);
void events_sema_destroy(events_sema_t *s);
void events_sema_release(events_sema_t *s);
bool events_sema_wait(events_sema_t *s, int ms);


#ifdef __cplusplus
}
#endif

#endif
