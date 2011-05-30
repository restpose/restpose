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
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

ThreadPool::~ThreadPool()
{
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->stop();
    }
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->join();
    }
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	delete *i;
    }
}

void
ThreadPool::add_thread(Thread * thread)
{
    ContextLocker lock(mutex);
    auto_ptr<Thread> threadptr(thread);
    threads.push_back(NULL);
    threads.back() = threadptr.release();
    thread->start();
}

void
ThreadPool::stop()
{
    ContextLocker lock(mutex);
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->stop();
    }
}

void
ThreadPool::join()
{
    ContextLocker lock(mutex);
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->join();
    }
}

void
ThreadPool::get_status(Json::Value & status) const
{
    ContextLocker lock(mutex);
    status["size"] = uint64_t(threads.size());
}
