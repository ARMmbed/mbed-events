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

// Predeclared classes
template <typename F>
class Event;


/** EventQueue
 *
 *  Flexible event queue for dispatching events
 */
class EventQueue {
public:
    /** Create an EventQueue
     *
     *  Create an event queue. The event queue either allocates a buffer of
     *  the specified size with malloc or uses the user provided buffer.
     *
     *  @param size     Size of buffer to use for events in bytes
     *                  (default to EVENTS_QUEUE_SIZE)
     *  @param buffer   Pointer to buffer to use for events
     *                  (default to NULL)
     */
    EventQueue(unsigned size=EVENTS_QUEUE_SIZE, unsigned char *buffer=NULL);

    /** Destroy an EventQueue
     */
    ~EventQueue();

    /** Dispatch events
     *
     *  Executes events until the specified milliseconds have passed.
     *  If ms is negative, the dispatch function will dispatch events
     *  indefinitely or until break_dispatch is called on this queue.
     *
     *  When called with a finite timeout, the dispatch function is guaranteed
     *  to terminate. When called with a timeout of 0, the dispatch function
     *  does not wait and is irq safe.
     *
     *  @param ms       Time to wait for events in milliseconds, a negative
     *                  value will dispatch events indefinitely
     *                  (default to -1)
     */
    void dispatch(int ms=-1);

    /** Dispatch events without a timeout
     *
     *  This is equivalent to EventQueue::dispatch with no arguments, but 
     *  avoids overload ambiguities when passed as a callback.
     *
     *  @see EventQueue::dispatch
     */
    void dispatch_forever() { dispatch(); }

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

    /** Background an event queue onto a single-shot timer-interrupt
     *
     *  When updated, the event queue will call the provided update function
     *  with a timeout indicating when the queue should be dispatched. A
     *  negative timeout will be passed to the update function when the
     *  timer-interrupt is no longer needed.
     *
     *  Passing a null function disables the existing update function.
     *
     *  The background function allows an event queue to take advantage of
     *  hardware timers or other event loops, allowing an event queue to be
     *  ran in the background without consuming the foreground thread.
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
     *  The chain function allows multiple event queues to be composed,
     *  sharing the context of a dispatch loop while still being managed
     *  independently
     *
     *  @param target   Queue that will dispatch this queue's events as a
     *                  part of its dispatch loop
     */
    void chain(EventQueue *target);

    /** Calls an event on the queue
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
        struct local {
            static void call(void *p) { (*static_cast<F*>(p))(); }
            static void dtor(void *p) { static_cast<F*>(p)->~F(); }
        };

        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_dtor(e, &local::dtor);
        return equeue_post(&_equeue, &local::call, e);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename F, typename A0>
    int call(F f, A0 a0) {
        return call(context10<F, A0>(f, a0));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename F, typename A0, typename A1>
    int call(F f, A0 a0, A1 a1) {
        return call(context20<F, A0, A1>(f, a0, a1));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename F, typename A0, typename A1, typename A2>
    int call(F f, A0 a0, A1 a1, A2 a2) {
        return call(context30<F, A0, A1, A2>(f, a0, a1, a2));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call(F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(context40<F, A0, A1, A2, A3>(f, a0, a1, a2, a3));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(context50<F, A0, A1, A2, A3, A4>(f, a0, a1, a2, a3, a4));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R>
    int call(T *obj, R (T::*method)()) {
        return call(mbed::callback(obj, method));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R>
    int call(const T *obj, R (T::*method)() const) {
        return call(mbed::callback(obj, method));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R>
    int call(volatile T *obj, R (T::*method)() volatile) {
        return call(mbed::callback(obj, method));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R>
    int call(const volatile T *obj, R (T::*method)() const volatile) {
        return call(mbed::callback(obj, method));
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0>
    int call(T *obj, R (T::*method)(A0), A0 a0) {
        return call(mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0>
    int call(const T *obj, R (T::*method)(A0) const, A0 a0) {
        return call(mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0>
    int call(volatile T *obj, R (T::*method)(A0) volatile, A0 a0) {
        return call(mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0>
    int call(const volatile T *obj, R (T::*method)(A0) const volatile, A0 a0) {
        return call(mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1>
    int call(T *obj, R (T::*method)(A0, A1), A0 a0, A1 a1) {
        return call(mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1>
    int call(const T *obj, R (T::*method)(A0, A1) const, A0 a0, A1 a1) {
        return call(mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1>
    int call(volatile T *obj, R (T::*method)(A0, A1) volatile, A0 a0, A1 a1) {
        return call(mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1>
    int call(const volatile T *obj, R (T::*method)(A0, A1) const volatile, A0 a0, A1 a1) {
        return call(mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call(T *obj, R (T::*method)(A0, A1, A2), A0 a0, A1 a1, A2 a2) {
        return call(mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call(const T *obj, R (T::*method)(A0, A1, A2) const, A0 a0, A1 a1, A2 a2) {
        return call(mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call(volatile T *obj, R (T::*method)(A0, A1, A2) volatile, A0 a0, A1 a1, A2 a2) {
        return call(mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call(const volatile T *obj, R (T::*method)(A0, A1, A2) const volatile, A0 a0, A1 a1, A2 a2) {
        return call(mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call(T *obj, R (T::*method)(A0, A1, A2, A3), A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call(const T *obj, R (T::*method)(A0, A1, A2, A3) const, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call(volatile T *obj, R (T::*method)(A0, A1, A2, A3) volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call(const volatile T *obj, R (T::*method)(A0, A1, A2, A3) const volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(T *obj, R (T::*method)(A0, A1, A2, A3, A4), A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(const T *obj, R (T::*method)(A0, A1, A2, A3, A4) const, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue
     *  @see EventQueue::call
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call(const volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) const volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call(mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue after a specified delay
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
        struct local {
            static void call(void *p) { (*static_cast<F*>(p))(); }
            static void dtor(void *p) { static_cast<F*>(p)->~F(); }
        };

        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_delay(e, ms);
        equeue_event_dtor(e, &local::dtor);
        return equeue_post(&_equeue, &local::call, e);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename F, typename A0>
    int call_in(int ms, F f, A0 a0) {
        return call_in(ms, context10<F, A0>(f, a0));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename F, typename A0, typename A1>
    int call_in(int ms, F f, A0 a0, A1 a1) {
        return call_in(ms, context20<F, A0, A1>(f, a0, a1));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename F, typename A0, typename A1, typename A2>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2) {
        return call_in(ms, context30<F, A0, A1, A2>(f, a0, a1, a2));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, context40<F, A0, A1, A2, A3>(f, a0, a1, a2, a3));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, context50<F, A0, A1, A2, A3, A4>(f, a0, a1, a2, a3, a4));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R>
    int call_in(int ms, T *obj, R (T::*method)()) {
        return call_in(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R>
    int call_in(int ms, const T *obj, R (T::*method)() const) {
        return call_in(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R>
    int call_in(int ms, volatile T *obj, R (T::*method)() volatile) {
        return call_in(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R>
    int call_in(int ms, const volatile T *obj, R (T::*method)() const volatile) {
        return call_in(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0>
    int call_in(int ms, T *obj, R (T::*method)(A0), A0 a0) {
        return call_in(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0>
    int call_in(int ms, const T *obj, R (T::*method)(A0) const, A0 a0) {
        return call_in(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0>
    int call_in(int ms, volatile T *obj, R (T::*method)(A0) volatile, A0 a0) {
        return call_in(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0>
    int call_in(int ms, const volatile T *obj, R (T::*method)(A0) const volatile, A0 a0) {
        return call_in(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_in(int ms, T *obj, R (T::*method)(A0, A1), A0 a0, A1 a1) {
        return call_in(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_in(int ms, const T *obj, R (T::*method)(A0, A1) const, A0 a0, A1 a1) {
        return call_in(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_in(int ms, volatile T *obj, R (T::*method)(A0, A1) volatile, A0 a0, A1 a1) {
        return call_in(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_in(int ms, const volatile T *obj, R (T::*method)(A0, A1) const volatile, A0 a0, A1 a1) {
        return call_in(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_in(int ms, T *obj, R (T::*method)(A0, A1, A2), A0 a0, A1 a1, A2 a2) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_in(int ms, const T *obj, R (T::*method)(A0, A1, A2) const, A0 a0, A1 a1, A2 a2) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_in(int ms, volatile T *obj, R (T::*method)(A0, A1, A2) volatile, A0 a0, A1 a1, A2 a2) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_in(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2) const volatile, A0 a0, A1 a1, A2 a2) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, T *obj, R (T::*method)(A0, A1, A2, A3), A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, const T *obj, R (T::*method)(A0, A1, A2, A3) const, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, volatile T *obj, R (T::*method)(A0, A1, A2, A3) volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_in(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2, A3) const volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, T *obj, R (T::*method)(A0, A1, A2, A3, A4), A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, const T *obj, R (T::*method)(A0, A1, A2, A3, A4) const, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue after a specified delay
     *  @see EventQueue::call_in
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_in(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) const volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_in(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue periodically
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
        struct local {
            static void call(void *p) { (*static_cast<F*>(p))(); }
            static void dtor(void *p) { static_cast<F*>(p)->~F(); }
        };

        void *p = equeue_alloc(&_equeue, sizeof(F));
        if (!p) {
            return 0;
        }

        F *e = new (p) F(f);
        equeue_event_delay(e, ms);
        equeue_event_period(e, ms);
        equeue_event_dtor(e, &local::dtor);
        return equeue_post(&_equeue, &local::call, e);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename F, typename A0>
    int call_every(int ms, F f, A0 a0) {
        return call_every(ms, context10<F, A0>(f, a0));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename F, typename A0, typename A1>
    int call_every(int ms, F f, A0 a0, A1 a1) {
        return call_every(ms, context20<F, A0, A1>(f, a0, a1));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename F, typename A0, typename A1, typename A2>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2) {
        return call_every(ms, context30<F, A0, A1, A2>(f, a0, a1, a2));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, context40<F, A0, A1, A2, A3>(f, a0, a1, a2, a3));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, context50<F, A0, A1, A2, A3, A4>(f, a0, a1, a2, a3, a4));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R>
    int call_every(int ms, T *obj, R (T::*method)()) {
        return call_every(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R>
    int call_every(int ms, const T *obj, R (T::*method)() const) {
        return call_every(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R>
    int call_every(int ms, volatile T *obj, R (T::*method)() volatile) {
        return call_every(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R>
    int call_every(int ms, const volatile T *obj, R (T::*method)() const volatile) {
        return call_every(ms, mbed::callback(obj, method));
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0>
    int call_every(int ms, T *obj, R (T::*method)(A0), A0 a0) {
        return call_every(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0>
    int call_every(int ms, const T *obj, R (T::*method)(A0) const, A0 a0) {
        return call_every(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0>
    int call_every(int ms, volatile T *obj, R (T::*method)(A0) volatile, A0 a0) {
        return call_every(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0>
    int call_every(int ms, const volatile T *obj, R (T::*method)(A0) const volatile, A0 a0) {
        return call_every(ms, mbed::callback(obj, method), a0);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_every(int ms, T *obj, R (T::*method)(A0, A1), A0 a0, A1 a1) {
        return call_every(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_every(int ms, const T *obj, R (T::*method)(A0, A1) const, A0 a0, A1 a1) {
        return call_every(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_every(int ms, volatile T *obj, R (T::*method)(A0, A1) volatile, A0 a0, A1 a1) {
        return call_every(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1>
    int call_every(int ms, const volatile T *obj, R (T::*method)(A0, A1) const volatile, A0 a0, A1 a1) {
        return call_every(ms, mbed::callback(obj, method), a0, a1);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_every(int ms, T *obj, R (T::*method)(A0, A1, A2), A0 a0, A1 a1, A2 a2) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_every(int ms, const T *obj, R (T::*method)(A0, A1, A2) const, A0 a0, A1 a1, A2 a2) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_every(int ms, volatile T *obj, R (T::*method)(A0, A1, A2) volatile, A0 a0, A1 a1, A2 a2) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    int call_every(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2) const volatile, A0 a0, A1 a1, A2 a2) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, T *obj, R (T::*method)(A0, A1, A2, A3), A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, const T *obj, R (T::*method)(A0, A1, A2, A3) const, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, volatile T *obj, R (T::*method)(A0, A1, A2, A3) volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    int call_every(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2, A3) const volatile, A0 a0, A1 a1, A2 a2, A3 a3) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, T *obj, R (T::*method)(A0, A1, A2, A3, A4), A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, const T *obj, R (T::*method)(A0, A1, A2, A3, A4) const, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Calls an event on the queue periodically
     *  @see EventQueue::call_every
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    int call_every(int ms, const volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) const volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4) {
        return call_every(ms, mbed::callback(obj, method), a0, a1, a2, a3, a4);
    }

    /** Creates an event bound to the event queue
     *
     *  Constructs an event bound to the specified event queue. The specified
     *  callback acts as the target for the event and is executed in the
     *  context of the event queue's dispatch loop once posted.
     *
     *  @param f        Function to execute when the event is dispatched
     *  @param a0..a4   Arguments to pass to the callback
     *  @return         Event that will dispatch on the specific queue
     */
    template <typename F>
    Event<void()> event(F f);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename F, typename A0>
    Event<void()> event(F f, A0 a0);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename F, typename A0, typename A1>
    Event<void()> event(F f, A0 a0, A1 a1);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename F, typename A0, typename A1, typename A2>
    Event<void()> event(F f, A0 a0, A1 a1, A2 a2);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3>
    Event<void()> event(F f, A0 a0, A1 a1, A2 a2, A3 a3);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    Event<void()> event(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R>
    Event<void()> event(T *obj, R (T::*method)());

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R>
    Event<void()> event(const T *obj, R (T::*method)() const);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R>
    Event<void()> event(volatile T *obj, R (T::*method)() volatile);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R>
    Event<void()> event(const volatile T *obj, R (T::*method)() const volatile);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0>
    Event<void()> event(T *obj, R (T::*method)(A0), A0 a0);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0>
    Event<void()> event(const T *obj, R (T::*method)(A0) const, A0 a0);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0>
    Event<void()> event(volatile T *obj, R (T::*method)(A0) volatile, A0 a0);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0>
    Event<void()> event(const volatile T *obj, R (T::*method)(A0) const volatile, A0 a0);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1>
    Event<void()> event(T *obj, R (T::*method)(A0, A1), A0 a0, A1 a1);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1>
    Event<void()> event(const T *obj, R (T::*method)(A0, A1) const, A0 a0, A1 a1);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1>
    Event<void()> event(volatile T *obj, R (T::*method)(A0, A1) volatile, A0 a0, A1 a1);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1>
    Event<void()> event(const volatile T *obj, R (T::*method)(A0, A1) const volatile, A0 a0, A1 a1);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    Event<void()> event(T *obj, R (T::*method)(A0, A1, A2), A0 a0, A1 a1, A2 a2);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    Event<void()> event(const T *obj, R (T::*method)(A0, A1, A2) const, A0 a0, A1 a1, A2 a2);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    Event<void()> event(volatile T *obj, R (T::*method)(A0, A1, A2) volatile, A0 a0, A1 a1, A2 a2);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2>
    Event<void()> event(const volatile T *obj, R (T::*method)(A0, A1, A2) const volatile, A0 a0, A1 a1, A2 a2);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    Event<void()> event(T *obj, R (T::*method)(A0, A1, A2, A3), A0 a0, A1 a1, A2 a2, A3 a3);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    Event<void()> event(const T *obj, R (T::*method)(A0, A1, A2, A3) const, A0 a0, A1 a1, A2 a2, A3 a3);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    Event<void()> event(volatile T *obj, R (T::*method)(A0, A1, A2, A3) volatile, A0 a0, A1 a1, A2 a2, A3 a3);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3>
    Event<void()> event(const volatile T *obj, R (T::*method)(A0, A1, A2, A3) const volatile, A0 a0, A1 a1, A2 a2, A3 a3);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    Event<void()> event(T *obj, R (T::*method)(A0, A1, A2, A3, A4), A0 a0, A1 a1, A2 a2, A3 a3, A4 a4);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    Event<void()> event(const T *obj, R (T::*method)(A0, A1, A2, A3, A4) const, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    Event<void()> event(volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4);

    /** Creates an event bound to the event queue
     *  @see EventQueue::event
     */
    template <typename T, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    Event<void()> event(const volatile T *obj, R (T::*method)(A0, A1, A2, A3, A4) const volatile, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4);

protected:
    template <typename F>
    friend class Event;
    struct equeue _equeue;
    mbed::Callback<void(int)> _update;

    template <typename F>
    struct context00 {
        F f;

        context00(F f)
            : f(f) {}

        void operator()() {
            f();
        }
    };

    template <typename F, typename A0>
    struct context10 {
        F f; A0 a0;

        context10(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()() {
            f(a0);
        }
    };

    template <typename F, typename A0, typename A1>
    struct context20 {
        F f; A0 a0; A1 a1;

        context20(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()() {
            f(a0, a1);
        }
    };

    template <typename F, typename A0, typename A1, typename A2>
    struct context30 {
        F f; A0 a0; A1 a1; A2 a2;

        context30(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()() {
            f(a0, a1, a2);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3>
    struct context40 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context40(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()() {
            f(a0, a1, a2, a3);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct context50 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context50(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()() {
            f(a0, a1, a2, a3, a4);
        }
    };

    template <typename F, typename B0>
    struct context01 {
        F f;

        context01(F f)
            : f(f) {}

        void operator()(B0 b0) {
            f(b0);
        }
    };

    template <typename F, typename A0, typename B0>
    struct context11 {
        F f; A0 a0;

        context11(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()(B0 b0) {
            f(a0, b0);
        }
    };

    template <typename F, typename A0, typename A1, typename B0>
    struct context21 {
        F f; A0 a0; A1 a1;

        context21(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()(B0 b0) {
            f(a0, a1, b0);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename B0>
    struct context31 {
        F f; A0 a0; A1 a1; A2 a2;

        context31(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()(B0 b0) {
            f(a0, a1, a2, b0);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename B0>
    struct context41 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context41(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()(B0 b0) {
            f(a0, a1, a2, a3, b0);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4, typename B0>
    struct context51 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context51(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()(B0 b0) {
            f(a0, a1, a2, a3, a4, b0);
        }
    };

    template <typename F, typename B0, typename B1>
    struct context02 {
        F f;

        context02(F f)
            : f(f) {}

        void operator()(B0 b0, B1 b1) {
            f(b0, b1);
        }
    };

    template <typename F, typename A0, typename B0, typename B1>
    struct context12 {
        F f; A0 a0;

        context12(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()(B0 b0, B1 b1) {
            f(a0, b0, b1);
        }
    };

    template <typename F, typename A0, typename A1, typename B0, typename B1>
    struct context22 {
        F f; A0 a0; A1 a1;

        context22(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()(B0 b0, B1 b1) {
            f(a0, a1, b0, b1);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename B0, typename B1>
    struct context32 {
        F f; A0 a0; A1 a1; A2 a2;

        context32(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()(B0 b0, B1 b1) {
            f(a0, a1, a2, b0, b1);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename B0, typename B1>
    struct context42 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context42(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()(B0 b0, B1 b1) {
            f(a0, a1, a2, a3, b0, b1);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4, typename B0, typename B1>
    struct context52 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context52(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()(B0 b0, B1 b1) {
            f(a0, a1, a2, a3, a4, b0, b1);
        }
    };

    template <typename F, typename B0, typename B1, typename B2>
    struct context03 {
        F f;

        context03(F f)
            : f(f) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(b0, b1, b2);
        }
    };

    template <typename F, typename A0, typename B0, typename B1, typename B2>
    struct context13 {
        F f; A0 a0;

        context13(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(a0, b0, b1, b2);
        }
    };

    template <typename F, typename A0, typename A1, typename B0, typename B1, typename B2>
    struct context23 {
        F f; A0 a0; A1 a1;

        context23(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(a0, a1, b0, b1, b2);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename B0, typename B1, typename B2>
    struct context33 {
        F f; A0 a0; A1 a1; A2 a2;

        context33(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(a0, a1, a2, b0, b1, b2);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename B0, typename B1, typename B2>
    struct context43 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context43(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(a0, a1, a2, a3, b0, b1, b2);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4, typename B0, typename B1, typename B2>
    struct context53 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context53(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()(B0 b0, B1 b1, B2 b2) {
            f(a0, a1, a2, a3, a4, b0, b1, b2);
        }
    };

    template <typename F, typename B0, typename B1, typename B2, typename B3>
    struct context04 {
        F f;

        context04(F f)
            : f(f) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(b0, b1, b2, b3);
        }
    };

    template <typename F, typename A0, typename B0, typename B1, typename B2, typename B3>
    struct context14 {
        F f; A0 a0;

        context14(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(a0, b0, b1, b2, b3);
        }
    };

    template <typename F, typename A0, typename A1, typename B0, typename B1, typename B2, typename B3>
    struct context24 {
        F f; A0 a0; A1 a1;

        context24(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(a0, a1, b0, b1, b2, b3);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename B0, typename B1, typename B2, typename B3>
    struct context34 {
        F f; A0 a0; A1 a1; A2 a2;

        context34(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(a0, a1, a2, b0, b1, b2, b3);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename B0, typename B1, typename B2, typename B3>
    struct context44 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context44(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(a0, a1, a2, a3, b0, b1, b2, b3);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4, typename B0, typename B1, typename B2, typename B3>
    struct context54 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context54(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3) {
            f(a0, a1, a2, a3, a4, b0, b1, b2, b3);
        }
    };

    template <typename F, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context05 {
        F f;

        context05(F f)
            : f(f) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(b0, b1, b2, b3, b4);
        }
    };

    template <typename F, typename A0, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context15 {
        F f; A0 a0;

        context15(F f, A0 a0)
            : f(f), a0(a0) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(a0, b0, b1, b2, b3, b4);
        }
    };

    template <typename F, typename A0, typename A1, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context25 {
        F f; A0 a0; A1 a1;

        context25(F f, A0 a0, A1 a1)
            : f(f), a0(a0), a1(a1) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(a0, a1, b0, b1, b2, b3, b4);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context35 {
        F f; A0 a0; A1 a1; A2 a2;

        context35(F f, A0 a0, A1 a1, A2 a2)
            : f(f), a0(a0), a1(a1), a2(a2) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(a0, a1, a2, b0, b1, b2, b3, b4);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context45 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3;

        context45(F f, A0 a0, A1 a1, A2 a2, A3 a3)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(a0, a1, a2, a3, b0, b1, b2, b3, b4);
        }
    };

    template <typename F, typename A0, typename A1, typename A2, typename A3, typename A4, typename B0, typename B1, typename B2, typename B3, typename B4>
    struct context55 {
        F f; A0 a0; A1 a1; A2 a2; A3 a3; A4 a4;

        context55(F f, A0 a0, A1 a1, A2 a2, A3 a3, A4 a4)
            : f(f), a0(a0), a1(a1), a2(a2), a3(a3), a4(a4) {}

        void operator()(B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) {
            f(a0, a1, a2, a3, a4, b0, b1, b2, b3, b4);
        }
    };
};

}

#endif