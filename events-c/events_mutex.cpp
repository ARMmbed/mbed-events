#include "events_mutex.h"

#include "cmsis.h"


// Mutex operations
int events_mutex_create(events_mutex_t *m) { return 0; }
void events_mutex_destroy(events_mutex_t *m) {}

void events_mutex_lock(events_mutex_t *m) {
    *m = __get_PRIMASK();
    __disable_irq();
}

void events_mutex_unlock(events_mutex_t *m) {
    __set_PRIMASK(*m);
}
