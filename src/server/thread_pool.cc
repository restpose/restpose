/** @file thread_pool.cc
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

#include <config.h>
#include "thread_pool.h"

#include <memory>
#include "server/task_threads.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

ThreadPool::~ThreadPool()
{
    ContextLocker lock(mutex);
    // FIXME - any errors thrown by, eg, stop() or join() will cause cleanup to
    // fail on the other threads.
    for (std::map<TaskThread *, bool>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	i->first->stop();
    }
    for (std::map<TaskThread *, bool>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	TaskThread * thread = i->first;
	lock.unlock();
	// Threads will call thread_finished() to inform the pool that they've
	// stopped, so we need to unlock the mutex to allow this.  They can't
	// cause the thread to be deleted, though, so this should be safe.
	thread->join();
	lock.lock();
    }
    for (std::map<TaskThread *, bool>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	delete i->first;
    }
}

void
ThreadPool::add_thread(TaskThread * thread)
{
    auto_ptr<TaskThread> threadptr(thread);
    ContextLocker lock(mutex);
    try {
	threads[threadptr.release()] = true;
    } catch(...) {
	// Need this clause to avoid leaking thread if the map insert raises an
	// exception in the line above.
	delete thread;
	throw;
    }
    thread->set_threadpool(this);
    if (!thread->start()) {
	// FIXME - log thread failing to start.
	threads.erase(thread);
	delete thread;
    } else {
	++running;
    }
}

void
ThreadPool::thread_finished(TaskThread * thread)
{
    ContextLocker lock(mutex);
    std::map<TaskThread *, bool>::iterator i = threads.find(thread);
    if (i != threads.end()) {
	i->second = false;
	--running;
	++waiting_for_join;
    }
}

void
ThreadPool::stop()
{
    ContextLocker lock(mutex);
    for (std::map<TaskThread *, bool>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	if (i->second) {
	    i->first->stop();
	}
    }
}

void
ThreadPool::join()
{
    ContextLocker lock(mutex);
    for (std::map<TaskThread *, bool>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	TaskThread * thread = i->first;
	lock.unlock();
	// Threads will call thread_finished() to inform the pool that they've
	// stopped, so we need to unlock the mutex to allow this.  They can't
	// cause the thread to be deleted, though, so this should be safe.
	thread->join();
	lock.lock();
    }
}

void
ThreadPool::get_status(Json::Value & status) const
{
    ContextLocker lock(mutex);
    status["size"] = Json::UInt64(threads.size());
    status["running"] = Json::UInt64(running);
    status["waiting_for_join"] = Json::UInt64(waiting_for_join);
}
