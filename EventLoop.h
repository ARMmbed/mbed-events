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
#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "EventQueue.h"
#include "Thread.h"

namespace events {


/** EventLoop
 *  
 *  An event queue wrapped in a thread
 */
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
    bool _running;
};


}

#endif
