#ifdef MBED_CONF_RTOS_PRESENT
#include "EventLoop.h"

#include "events.h"
#include "rtos.h"
#include "mbed.h"


EventLoop::EventLoop(
        osPriority priority,
        unsigned event_count,
        unsigned event_size,
        unsigned char *event_pointer,
        uint32_t stack_size,
        unsigned char *stack_pointer)
    : EventQueue(event_count, event_size, event_pointer)
    , _thread(priority, stack_size, stack_pointer)
    , _running(false) {
}

EventLoop::~EventLoop() {
    stop();
}

static void run(EventLoop *loop) {
    loop->dispatch();
}

osStatus EventLoop::start() {
    if (_running) {
        return osOK;
    }

    osStatus status = _thread.start(this, run);
    _running = (status == osOK);
    return status;
}

osStatus EventLoop::stop() {
    if (!_running) {
        return osOK;
    }

    break_();
    osStatus status = _thread.join();
    _running = false;
    return status;
}

#endif
