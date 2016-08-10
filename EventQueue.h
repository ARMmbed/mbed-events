/* events
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "equeue/equeue.h"
#include "Callback.h"
#include <cstddef>
#include <new>

namespace events {


/** EVENTS_EVENT_SIZE
 *  Minimum size of an event
 *  This size fits a Callback<void()> at minimum
 */
#define EVENTS_EVENT_SIZE \
    (EQUEUE_EVENT_SIZE - 2*sizeof(void*) + sizeof(mbed::Callback<void()>))

/** EVENTS_QUEUE_SIZE
 *  Default size of buffer for events
 */
#define EVENTS_QUEUE_SIZE (32*EVENTS_EVENT_SIZE)


/** EventQueue
 *
 *  Flexible event queue for dispatching events
 */
class EventQueue {
public:
    /** EventQueue lifetime
     *
     *  Create and destroy event queues. The event queue either allocates
     *  a buffer of the specified size with malloc or uses the user provided
     *  buffer.
     *
     *  @param size     Size of buffer to use for events in bytes
     *                  (default to EVENTS_QUEUE_SIZE)
     *  @param buffer   Pointer to buffer to use for events
     *                  (default to NULL)
     */
    EventQueue(unsigned size=EVENTS_QUEUE_SIZE, unsigned char *buffer=NULL);
    ~EventQueue();

    /** Dispatch events
     *
     *  Executes events until the specified milliseconds have passed.
     *  If ms is negative, the dispatch function will dispatch events
     *  indefinitely or until break_dispatch is called on this queue.
     *
     *  When called with a finite timeout, the dispatch function is garunteed
     *  to terminate. When called with a timeout of 0, the dispatch function
     *  does not wait and is irq safe.
     *
     *  @param ms       Time to wait for events in milliseconds, a negative
     *                  value will dispatch events indefinitely
     *                  (default to -1)
     */
    void dispatch(int ms);
    void dispatch() { dispatch(-1); }

    /** Break out of a running event loop
     *
     *  Forces the specified event queue's dispatch loop to terminate. Pending
     *  events may finish executing, but no new events will be executed.
     */
    void break_dispatch();

    /** Millisecond counter
     *
     *  Returns the underlying tick of the event queue represented as the 
     *  number of milliseconds that have passed since an arbitrary point in
     *  time. Intentionally overflows to 0 after 2^32-1.
     *
     *  @return         The underlying tick of the event queue in milliseconds
     */
    unsigned tick();

    /** Cancel an in-flight event
     *
     *  Attempts to cancel an event referenced by the unique id returned from
     *  one of the call functions. It is safe to call cancel after an event
     *  has already been dispatched.
     *
     *  The cancel function is irq safe.
     *
     *  If called while the event queue's dispatch loop is active, the cancel
     *  function does not garuntee that the event will not execute after it
     *  returns, as the event may have already begun executing.
     *
     *  @param id       Unique id of the event
     */
    void cancel(int id);

    /** Background an event queue onto a single-shot timer
     *
     *  The provided update function will be called to indicate when the queue
     *  should be dispatched. A negative timeout will be passed to the update
     *  function when the time is no longer needed.
     *
     *  Passing a null update function disables the existing timre.
     *
     *  The background function allows an event queue to take advantage of
     *  hardware timers or even other event loops, allowing an event queue to
     *  be effectively backgrounded.
     *
     *  @param update   Function called to indicate when the queue should be
     *                  dispatched
     */
    void background(mbed::Callback<void(int)> update);

    /** Chain an event queue onto another event queue
     *
     *  After chaining a queue to a target, calling dispatch on the target
     *  queue will also dispatch events from this queue. The queues use
     *  their own buffers and events must be handled independently.
     *
     *  A null queue as the target will unchain the existing queue.
     *
     *  The chain function allows multiple event queuest to be composed,
     *  sharing the context of a dispatch loop while still being managed
     *  independently
     *
     *  @param target   Queue that will dispatch this queue's events as a
     *                  part of its dispatch loop
     */
    void chain(EventQueue *target);

    /** Post an event to the queue
     *
     *  The specified callback will be executed in the context of the event
     *  queue's dispatch loop.
     *
     *  The call function is irq safe and can act as a mechanism for moving
     *  events out of irq contexts.
     *
     *  @param f        Function to execute in the context of the dispatch loop
     *  @param a0..a4   Arguments to pass to the callback
     *  @return         A unique id that represents the posted event and can
     *                  be passed to cancel, or an id of 0 if there is not
     *                  enough memory to allocate the event.
     */
    template <typename F>
    int call(F f) {
        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_dtor(e, &EventQueue::dtor<F>);
        return equeue_post(&_equeue, &EventQueue::call<F>, e);
    }

    template <typename F, typename A0>
    int call(F f, A0 a0) {
        return call(Context1<F,A0>(f,a0));
    }

    template <typename F, typename A0, typename A1>
    int call(F f, A0 a0, A1 a1) {
        return call(Context2<F,A0,A1>(f,a0,a1));
    }

    template <typename F, typename A0, typename A1, typename A2>
    int call(F f, A0 a0, A1 a1, A2 a2) {
        return call(Context3<F,A0,A1,A2>(f,a0,a1,a2));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call(F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(Context4<F,A0,A1,A2,A3>(f,a0,a1,a2,a3));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(Context5<F,A0,A1,A2,A3,A4>(f,a0,a1,a2,a3,a4));
    }

    /** Post an event to the queue after a specified delay
     *
     *  The specified callback will be executed in the context of the event
     *  queue's dispatch loop.
     *
     *  The call_in function is irq safe and can act as a mechanism for moving
     *  events out of irq contexts.
     *
     *  @param f        Function to execute in the context of the dispatch loop
     *  @param a0..a4   Arguments to pass to the callback
     *  @param ms       Time to delay in milliseconds
     *  @return         A unique id that represents the posted event and can
     *                  be passed to cancel, or an id of 0 if there is not
     *                  enough memory to allocate the event.
     */
    template <typename F>
    int call_in(int ms, F f) {
        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_delay(e, ms);
        equeue_event_dtor(e, &EventQueue::dtor<F>);
        return equeue_post(&_equeue, &EventQueue::call<F>, e);
    }

    template <typename F, typename A0>
    int call_in(int ms, F f, A0 a0) {
        return call_in(ms, Context1<F,A0>(f,a0));
    }

    template <typename F, typename A0, typename A1>
    int call_in(int ms, F f, A0 a0, A1 a1) {
        return call_in(ms, Context2<F,A0,A1>(f,a0,a1));
    }

    template <typename F, typename A0, typename A1, typename A2>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2) {
        return call_in(ms, Context3<F,A0,A1,A2>(f,a0,a1,a2));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, Context4<F,A0,A1,A2,A3>(f,a0,a1,a2,a3));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, Context5<F,A0,A1,A2,A3,A4>(f,a0,a1,a2,a3,a4));
    }

    /** Post an event to the queue periodically
     *
     *  The specified callback will be executed in the context of the event
     *  queue's dispatch loop.
     *
     *  The call_every function is irq safe and can act as a mechanism for
     *  moving events out of irq contexts.
     *
     *  @param f        Function to execute in the context of the dispatch loop
     *  @param a0..a4   Arguments to pass to the callback
     *  @param ms       Period of the event in milliseconds
     *  @return         A unique id that represents the posted event and can
     *                  be passed to cancel, or an id of 0 if there is not
     *                  enough memory to allocate the event.
     */
    template <typename F>
    int call_every(int ms, F f) {
        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_delay(e, ms);
        equeue_event_period(e, ms);
        equeue_event_dtor(e, &EventQueue::dtor<F>);
        return equeue_post(&_equeue, &EventQueue::call<F>, e);
    }

    template <typename F, typename A0>
    int call_every(int ms, F f, A0 a0) {
        return call_every(ms, Context1<F,A0>(f,a0));
    }

    template <typename F, typename A0, typename A1>
    int call_every(int ms, F f, A0 a0, A1 a1) {
        return call_every(ms, Context2<F,A0,A1>(f,a0,a1));
    }

    template <typename F, typename A0, typename A1, typename A2>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2) {
        return call_every(ms, Context3<F,A0,A1,A2>(f,a0,a1,a2));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, Context4<F,A0,A1,A2,A3>(f,a0,a1,a2,a3));
    }

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, Context5<F,A0,A1,A2,A3,A4>(f,a0,a1,a2,a3,a4));
    }

protected:
    struct equeue _equeue;
    mbed::Callback<void(int)> _update;

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct Context5 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        Context5(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()() {
            f(a0, a1, a2, a3, a4);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3>
    struct Context4 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        Context4(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()() {
            f(a0, a1, a2, a3);
        }
    };

    template <typename F, typename A0, typename A1, typename A2>
    struct Context3 {
        F f; A0 a0; A1 a1; A2 a2;

        Context3(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()() {
            f(a0, a1, a2);
        }
    };

    template <typename F, typename A0, typename A1>
    struct Context2 {
        F f; A0 a0; A1 a1;

        Context2(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()() {
            f(a0, a1);
        }
    };

    template <typename F, typename A0>
    struct Context1 {
        F f; A0 a0;

        Context1(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()() {
            f(a0);
        }
    };

    template <typename T>
    static void call(void *p) {
        (*static_cast<T*>(p))();
    }

    template <typename T>
    static void dtor(void *p) {
        static_cast<T*>(p)->~T();
    }
};


}

#endif
