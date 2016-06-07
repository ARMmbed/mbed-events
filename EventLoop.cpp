#include "EventLoop.h"

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
    , _thread(priority, stack_size, stack_pointer) {
}

EventLoop::~EventLoop() {
    stop();
}

MBED_NORETURN static void run(EventLoop *loop) {
    loop->dispatch();
    MBED_UNREACHABLE;
}

osStatus EventLoop::start() {
    return _thread.start(this, run);
}

MBED_NORETURN static void halt(Semaphore *halted) {
    halted->release();
    while (1);
}

osStatus EventLoop::stop() {
    Semaphore halted(0);
    post(halt, &halted);
    halted.wait();

    return _thread.terminate();
}
