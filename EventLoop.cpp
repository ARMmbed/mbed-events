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

MBED_NORETURN static void run(EventLoop *loop) {
    loop->dispatch();
    MBED_UNREACHABLE;
}

osStatus EventLoop::start() {
    if (_running) {
        return osOK;
    }

    osStatus status = _thread.start(this, run);
    _running = (status == osOK);
    return status;
}


struct hcontext {
    Semaphore *halted;
};

MBED_NORETURN static void halt(void *p) {
    hcontext *context = static_cast<hcontext*>(p);
    context->halted->release();
    while (1);
}

osStatus EventLoop::stop() {
    if (!_running) {
        return osOK;
    }

    // Freeze the thread in a safe state
    Semaphore halted(0);
    hcontext *context = static_cast<hcontext*>(
            event_alloc(&_equeue, sizeof(hcontext)));
    context->halted = &halted;
    event_post(&_equeue, halt, context);
    halted.wait();

    // Kill the thread and clean up missed memory
    _thread.terminate();
    event_dealloc(&_equeue, context);

    osStatus status = _thread.terminate();
    _running = false;
    return status;
}
