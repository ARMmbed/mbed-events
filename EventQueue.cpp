#include "EventQueue.h"
#include "events-c/events.h"
#include "mbed.h"


EventQueue::EventQueue(unsigned event_count, unsigned event_size,
        unsigned char *event_pointer) {
    if (event_size < sizeof(Callback<void()>)) {
        event_size = sizeof(Callback<void()>);
    }

    if (!event_pointer) {
        equeue_create(&_equeue, event_count, event_size);
    } else {
        equeue_create_inplace(&_equeue, event_count, event_size, event_pointer);
    }
}

EventQueue::~EventQueue() {
    equeue_destroy(&_equeue);
}

void EventQueue::dispatch(int ms) {
    return equeue_dispatch(&_equeue, ms);
}

unsigned EventQueue::get_tick() {
    return events_tick();
}

int EventQueue::wait(int id, int ms) {
    return event_wait(&_equeue, id, ms);
}

void EventQueue::cancel(int id) {
    return event_cancel(&_equeue, id);
}

