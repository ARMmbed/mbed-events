#include "events_sema.h"

#include "rtos.h"


// Semaphore operations
static inline Semaphore *sema(events_sema_t *s) {
    return static_cast<Semaphore*>(*s);
}

int events_sema_create(events_sema_t *s) {
    *s = new Semaphore(0);
    return sema(s) ? 0 : -1;
}

void events_sema_destroy(events_sema_t *s) {
    delete sema(s);
}

void events_sema_release(events_sema_t *s) {
    sema(s)->release();
}

bool events_sema_wait(events_sema_t *s, int ms) {
    int t = sema(s)->wait(ms < 0 ? osWaitForever : ms);
    return t > 0;
}
