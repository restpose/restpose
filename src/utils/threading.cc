/** @file threading.h
 * @brief Convenient wrapper for holding thread locks.
 */
/* Copyright 2010 Richard Boulton
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
#include "threading.h"

#include <cstdio>
#include <xapian.h>

static void *
run_thread(void * arg_ptr)
{
    Thread * thread = static_cast<Thread *>(arg_ptr);
    try {
	try {
	    thread->run();
	} catch(...) {
	    thread->cleanup();
	    throw;
	}
	thread->cleanup();
    } catch(const RestPose::Error & e) {
	fprintf(stderr, "Thread died: %s\n", e.what());
    } catch(const Xapian::Error & e) {
	fprintf(stderr, "Thread died: %s\n", e.get_description().c_str());
    } catch(const std::bad_alloc) {
	fprintf(stderr, "Thread died: out of memory\n");
    }

    return NULL;
}

void
Thread::start()
{
    if (started)
	return;
    started = true;
    int ret = pthread_create(&thread, NULL, run_thread, this);
    if (ret == -1) {
	throw RestPose::ThreadError("Can't start thread");
    }
}
