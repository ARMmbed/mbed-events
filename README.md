## The Events Library ##

The events library provides a flexible event queue for scheduling events.

``` cpp
#include "mbed_events.h"
#include <stdio.h>

int main() {
    // creates a queue with the default size
    EventQueue queue;

    // events are simple callbacks
    queue.call(printf, "called immediately\n");
    queue.call_in(2000, printf, "called in 2 seconds\n");
    queue.call_every(1000, printf, "called every 1 seconds\n");

    // executed by the dispatch method
    queue.dispatch();
}
```


### Usage ###

The core API of the events library is contained in the
[EventQueue](EventQueue.h) class.

``` cpp
// Creates an event queue with 2048 bytes of buffer space to use
// for enqueueing events. With no argument, the default buffer is
// allocated with enough space for 32 Callback classes.
EventQueue queue(2048);

// Enqueues events on the underlying event queue
queue.call(printf, "hi!\n");

// The dispatch method acts as the core entry point into the event loop
// A millisecond timeout can be provided to return from the event loop
queue.dispatch(500);
```

Additionally, the events library provides the [EventLoop](EventLoop.h) class,
which combines the EventQueue with a Thread.

``` cpp
// Creates a high priority event loop.
EventLoop loop(osHighPriority);

// Starts the loop in a separate thread
loop.start();

// Posting events is thread and irq safe
loop.call(doit);

// Stops the event loop cleanly
loop.stop();
```

The EventQueue and EvenLoop classes provide several call functions for
sending events. The call functions are thread and irq safe and don't need
the underlying loop to be running.

``` cpp
// Simple call function registers events to be called as soon as possible
queue.call(doit);
queue.call(printf, "called immediately\n");
queue.call(Callback<void(char)>(&serial, &Serial::printf), "hello\n");

// The call_in function registers events to be called after a delay
// specified in milliseconds
queue.call_in(2000, doit_in_two_seconds);
queue.call_in(300, printf, "called in 0.3 seconds\n");

// The call_every function registers events to be called repeatedly
// with a period specified in milliseconds
queue.call_every(2000, doit_every_two_seconds);
queue.call_every(400, printf, "called every 0.4 seconds\n");
```

All call calls return an integer id that uniquely represents the event
on the event queue. The call calls can not block, so 0 is returned if
there is no memory or the queue's event size is exceeded.

``` cpp
// The event id is uniqueue to the queue
int id = queue.call_in(100, printf, "will this work?\n");

// An id of 0 indicates an error
if (id) {
    error("oh no!");
}

// Events can also be cancelled
queue.cancel(id);
```


### Porting ###

The events library actually provides a C++ and C API. To port to a different
platform, the events library only needs implementations of the following
files:

- [events_tick.h](events-c/events_tick.h) - Monotonic counter with millisecond precision
- [events_mutex.h](events-c/events_mutex.h) - IRQ safe critical section
- [events_sema.h](events-c/events_sema.h) - Binary semaphore with timeout
