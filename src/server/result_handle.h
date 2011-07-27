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

#include <string>
#include "utils/threading.h"
#include "json/value.h"

class Response;

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
    class Internal;

    Internal * internal;

  public:
    ResultHandle();
    ~ResultHandle();
    ResultHandle(const ResultHandle & other);
    void operator=(const ResultHandle & other);

    void set_nudge(int nudge_fd, char nudge_byte);

    /** Get a reference to the response object.
     *
     *  This reference should only be used by the preparing thread before
     *  set_ready() has been called.
     */
    Response & response();

    /** Mark the result as ready.
     *
     *  This should be called (once) by the preparing thread.
     *  After this point, the preparing thread must not access response().
     */
    void set_ready();

    /** Check if the result is ready - return true if it is.
     *
     *  This is intended for use by the waiting thread.
     */
    bool is_ready() const;

    /** This may be called by the preparing thread to indicate a failure.
     *
     *  If set_ready() has already been called, this call will have no effect.
     *  Otherwise, it will set the response to describe the failure, and
     *  mark the result as ready.
     *
     *  After this point, the preparing thread must not access response().
     */
    void failed_json(const Json::Value & body, int status_code = 200);

    /** Report a failure in a standard format.
     *
     *  As failed_json(), but returns a standard JSON body, allowing the caller
     *  to set the message in it.
     */
    void failed(const std::string & err, int status_code = 200) {
	Json::Value result(Json::objectValue);
	result["err"] = err;
	failed_json(result, status_code);
    }
};

}

#endif /* RESTPOSE_INCLUDED_RESULT_HANDLE_H */
