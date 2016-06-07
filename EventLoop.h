/*  EventLoop
 *
 *  EventQueue wrapped in a thread
 */
#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "EventQueue.h"
#include "Thread.h"


class EventLoop : public EventQueue {
public:
    /** Create an event loop
     *
     *  @param priority         Initial priority of the thread
     *                          (default: osPriorityNormal)
     *  @param event_count      Number of events to allow enqueue at once
     *                          (default: 32)
     *  @param event_size       Maximum size of context of event
     *                          (default: 0)
     *  @param event_pointer    Pointer to memory region to use for events
     *                          (default: NULL)
     *  @param stack_size       Stack size (in bytes) requirements for the thread
     *                          (default: DEFAULT_STACK_SIZE)
     *  @param stack_pointer    Pointer to stack area to be used by the thread
     *                          (default: NULL)
     */
    EventLoop(osPriority priority=osPriorityNormal,
              unsigned event_count=32,
              unsigned event_size=0,
              unsigned char *event_pointer=NULL,
              uint32_t stack_size=DEFAULT_STACK_SIZE,
              unsigned char *stack_pointer=NULL);

    /** Clean up event loop
     */
    ~EventLoop();

    /** Starts an event loop running in a dedicated thread
     *  @return     status code that indicates the execution status of the function.
     */
    osStatus start();

    /** Stops an event loop cleanly, waiting for any currently executing events
     *  @return     status code that indicates the execution status of the function.
     */
    osStatus stop();

private:
    rtos::Thread _thread;
};


#endif
