## The Events Library ##

The events library provides a flexible event queue for scheduling events.

``` cpp
#include "events.h"
#include <stdio.h>

int main() {
    // creates a queue with the default size
    EventQueue queue;

    // events are simple callbacks
    queue.post(printf, "called immediately\n");
    queue.post_in(printf, "called in 2 seconds\n", 2000);
    queue.post_every(printf, "called every 1 seconds\n", 1000);

    // executed by the dispatch method
    queue.dispatch();
}
```


### Usage ###

The core API of the events library is contained in the
[EventQueue](EventQueue.h) class. To perform efficient event allocation
in irq contexts, the EventQueue uses a fixed-size buffer that is configured
at construction time.

``` cpp
// Creates an event queue with enough buffer space for 32 events. Each event
// is allocated with enough space for at least a callback and a string.
EventQueue queue(32, sizeof(struct {
    Callback<void()> func;
    const char *str;
}));

// Enqueues events on the underlying event queue
queue.post(printf, "hi!\n");

// The dispatch method acts as the core entry point into the event loop
// A millisecond timeout can be provided to return from the event loop
queue.dispatch(500);
```

Additionally, the events library provides the [EventLoop](EventLoop.h) class,
which combines the EventQueue with a Thread.

``` cpp
// Creates a high priority event loop. By default events can contain
// only a Callback<void()>
EventLoop loop(osHighPriority, 12);

// Starts the loop in a separate thread
loop.start();

// Posting events is thread and irq safe
loop.post(doit);

// Stops the event loop cleanly
loop.stop();
```

The EventQueue and EvenLoop classes provide several post functions for
sending events. The post functions are thread and irq safe and don't need
the underlying loop to be running.

``` cpp
// Simple post function registers events to be called as soon as possible
queue.post(doit);
queue.post(printf, "called immediately\n");
queue.post(Callback<void(char)>(&serial, &Serial::printf), "hello\n");

// The post_in function registers events to be called after a delay
// specified in milliseconds
queue.post_in(doit_in_two_seconds, 2000);
queue.post_in(printf, "called in 0.3 seconds\n", 300);

// The post_every function registers events to be called repeatedly
// with a period specified in milliseconds
queue.post_every(doit_every_two_seconds, 2000);
queue.post_every(printf, "called every 0.4 seconds\n", 400);
```

All post calls return an integer id that uniquely represents the event
on the event queue. The post calls can not block, so a negative id is
returned if there is no memory or the queue's event size is exceeded.

``` cpp
// The event id is uniqueue to the queue
int id = queue.post_in(printf, "will this work?\n", 100);

// A negative id indicates an error
if (id < 0) {
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
