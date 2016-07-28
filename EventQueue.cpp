#include "EventQueue.h"

#include "events.h"
#include "mbed.h"


EventQueue::EventQueue(unsigned event_size, unsigned char *event_pointer) {
    if (!event_pointer) {
        equeue_create(&_equeue, event_size);
    } else {
        equeue_create_inplace(&_equeue, event_size, event_pointer);
    }
}

EventQueue::~EventQueue() {
    equeue_destroy(&_equeue);
}

void EventQueue::dispatch(int ms) {
    return equeue_dispatch(&_equeue, ms);
}

void EventQueue::break_() {
    return equeue_break(&_equeue);
}

unsigned EventQueue::get_tick() {
    return events_tick();
}

void EventQueue::cancel(int id) {
    return event_cancel(&_equeue, id);
}

