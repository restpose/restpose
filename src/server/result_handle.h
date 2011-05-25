/** @file result_handle.h
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
#ifndef RESTPOSE_INCLUDED_RESULT_HANDLE_H
#define RESTPOSE_INCLUDED_RESULT_HANDLE_H

#include <json/value.h>
#include "utils/threading.h"

namespace RestPose {

/** A synchronised reference counted container pointing to a result.
 *
 *  This is intended for use when one thread is preparing the result, and
 *  another thread wants to use the result when it's prepared.
 *
 *  The waiting thread should be notified by some other mechanism (eg, a nudge
 *  on a file descriptor) that the result is ready.
 */
class ResultHandle {
    class Internal {
	Internal(const Internal &);
	void operator=(const Internal &);
      public:
	mutable Mutex mutex;
	mutable unsigned ref_count;
	Json::Value value;
	int status_code;
	int nudge_fd;
	char nudge_byte;
	bool is_ready;

	Internal(int nudge_fd_, char nudge_byte_)
		: ref_count(1),
		  status_code(200),
		  nudge_fd(nudge_fd_),
		  nudge_byte(nudge_byte_),
		  is_ready(false) {}
    };

    Internal * internal;

  public:
    ResultHandle(int nudge_fd, char nudge_byte);
    ~ResultHandle();
    ResultHandle(const ResultHandle & other);
    void operator=(const ResultHandle & other);

    /** If the result is ready, return a pointer to it.  Otherwise, return NULL.
     *
     *  This is intended for use by the waiting thread.
     */
    const Json::Value * get_result() const {
	ContextLocker lock(internal->mutex);
	if (internal->is_ready) {
	    return &internal->value;
	} else {
	    return NULL;
	}
    }

    /** Get a reference to the result.
     *
     *  This is indended for use by the preparing thread, before is_ready is
     *  set to true.
     */
    Json::Value & result_target() {
	return internal->value;
    }

    /** Set the status code.
     *
     *  This should only be called before set_ready() has been called.
     */
    void set_status(int status_code) {
	internal->status_code = status_code;
    }

    /** Get the status code.
     *
     *  This should only be called after get_result() has returned non NULL.
     */
    int get_status() const {
	return internal->status_code;
    }

    /** Mark the result as ready.
     *
     *  This should be called (once) by the preparing thread.
     *  After this point, the preparing thread must not access result_target().
     */
    void set_ready();
};

}

#endif /* RESTPOSE_INCLUDED_RESULT_HANDLE_H */
