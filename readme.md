## The Events Library ##

The events library provides a flexible event queue for scheduling events.

```
#include "events.h"
#include <stdio.h>

int main() {
    // creates a queue with the default size
    EventQueue queue;

    // events are simple callbacks
    queue.post(puts, "called immediately");
    queue.post_in(puts, "called in 2 seconds", 2000);
    queue.post_every(puts, "called every 1 seconds", 1000);

    // events are executed when dispatch is called
    queue.dispatch();
}
```

