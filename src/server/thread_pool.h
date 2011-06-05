/** @file thread_pool.h
 * @brief Thread pool
 */
/* Copyright (c) 2011 Richard Boulton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef RESTPOSE_INCLUDED_THREAD_POOL_H
#define RESTPOSE_INCLUDED_THREAD_POOL_H

#include <json/value.h>
#include "utils/io_wrappers.h"
#include "utils/threading.h"
#include <map>

class TaskThread;

/** A pool of threads.
 */
class ThreadPool {
    mutable Mutex mutex;

    /** Details of the running threads.
     *
     *  The value is true if the thread is running; false if the thread has
     *  finished and needs to be joined (and then deleted).
     */
    std::map<TaskThread *, bool> threads;

    /// Number of threads currently running.
    unsigned running;

    /// Number of threads waiting to be joined.
    unsigned waiting_for_join;

    ThreadPool(const ThreadPool &);
    void operator=(const ThreadPool &);
  public:
    ThreadPool() : running(0), waiting_for_join(0) {}
    ~ThreadPool();

    /** Add a thread to the pool, and start it.
     */
    void add_thread(TaskThread * thread);

    /** Mark that a thread in the pool has finished.
     */
    void thread_finished(TaskThread * thread);

    /** Stop all threads in the pool.
     */
    void stop();

    /** Join all threads in the pool.
     */
    void join();

    /** Get the current status of the thread pool.
     */
    void get_status(Json::Value & status) const;
};

#endif /* RESTPOSE_INCLUDED_THREAD_POOL_H */
