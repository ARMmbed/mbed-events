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
    struct event e;
    Semaphore halted;
};

MBED_NORETURN static void halt(void *p) {
    Semaphore *halted = static_cast<Semaphore*>(p);
    halted->release();
    while (true) {
        Thread::wait(osWaitForever);
    }
}

osStatus EventLoop::stop() {
    if (!_running) {
        return osOK;
    }

    // Freeze the thread in a safe state
    struct hcontext context = {{0}, Semaphore(0)};
    event_delay(&context.halted, 0);
    event_post(&_equeue, halt, &context.halted);
    context.halted.wait();

    // Kill the thread
    osStatus status = _thread.terminate();
    _running = false;
    return status;
}

#endif
