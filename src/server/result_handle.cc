/** @file result_handle.cc
 * @brief Hold results of an operation, for passing between threads.
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
#include "server/result_handle.h"

#include "httpserver/response.h"
#include "utils/io_wrappers.h"

using namespace RestPose;

class ResultHandle::Internal {
    Internal(const Internal &);
    void operator=(const Internal &);
  public:
    mutable Mutex mutex;
    mutable unsigned ref_count;
    Response response;
    int nudge_fd;
    char nudge_byte;
    bool is_ready;

    Internal()
	    : ref_count(1),
	      nudge_fd(-1),
	      nudge_byte('\0'),
	      is_ready(false) {}
};


ResultHandle::ResultHandle()
	: internal(new ResultHandle::Internal())
{}

ResultHandle::~ResultHandle()
{
    ContextLocker lock(internal->mutex);
    --(internal->ref_count);
    if (internal->ref_count == 0) {
	lock.unlock();
	delete internal;
    }
}

ResultHandle::ResultHandle(const ResultHandle & other)
	: internal(NULL)
{
    ContextLocker lock(other.internal->mutex);
    internal = other.internal;
    ++(internal->ref_count);
}

void
ResultHandle::operator=(const ResultHandle & other)
{
    {
	ContextLocker lock(internal->mutex);
	--(internal->ref_count);
	if (internal->ref_count == 0) {
	    lock.unlock();
	    delete internal;
	}
	internal = NULL;
    }

    {
	ContextLocker lock(other.internal->mutex);
	internal = other.internal;
	++(internal->ref_count);
    }
}

void ResultHandle::set_nudge(int nudge_fd, char nudge_byte) {
    internal->nudge_fd = nudge_fd;
    internal->nudge_byte = nudge_byte;
}

Response & ResultHandle::response() {
    return internal->response;
}

void
ResultHandle::set_ready() {
    ContextLocker lock(internal->mutex);
    internal->is_ready = true;
    lock.unlock();
    // Unlock before writing, just in case the io_write_byte() blocks.
    (void) io_write_byte(internal->nudge_fd, internal->nudge_byte);
}

bool
ResultHandle::is_ready() const {
    ContextLocker lock(internal->mutex);
    return internal->is_ready;
}
