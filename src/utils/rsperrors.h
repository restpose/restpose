/** @file rsperrors.h
 * @brief Errors raised by RestPose.
 */
/* Copyright (c) 2010, 2011 Richard Boulton
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

#ifndef RESTPOSE_INCLUDED_RSPERRORS_H
#define RESTPOSE_INCLUDED_RSPERRORS_H

#include <exception>
#include <string>

namespace RestPose {
    /** Base class of errors from RestPose.
     */
    class Error : public std::exception {
	std::string message;
      public:
	Error(const std::string & message_, const std::string & type) : message("RestPose::" + type + ": " + message_) {}
	~Error() throw() {}
	virtual const char * what() const throw() { return message.c_str(); }
    };

    /** An exception indicating that there was an invalid value in input data.
     */
    class InvalidValueError : public Error {
      public:
	InvalidValueError(const std::string & message_) : Error(message_, "InvalidValueError") {}
    };

    /** An exception indicating that there was a problem with some serialised data.
     */
    class UnserialisationError : public Error {
      public:
	UnserialisationError(const std::string & message_) : Error(message_, "UnserialisationError") {}
    };

    /** An exception indicating that there was a system error.
     */
    class SysError : public Error {
	int errno_value;
      public:
	SysError(const std::string & message_, int errno_value_);
	int get_errno() const { return errno_value; }
    };

    /** An internal error in the HTTP Server.
     */
    class HTTPServerError : public Error {
      public:
	HTTPServerError(const std::string & message_) : Error(message_, "HTTPServerError") {}
    };

    /** A error with thread synchronisation functions.
     */
    class ThreadError : public Error {
      public:
	ThreadError(const std::string & message_) : Error(message_, "ThreadError") {}
    };

    /** A error due to something being in the wrong state.
     */
    class InvalidStateError : public Error {
      public:
	InvalidStateError(const std::string & message_) : Error(message_, "InvalidStateError") {}
    };

    /** An error in an importer.
     */
    class ImporterError : public Error {
      public:
	ImporterError(const std::string & message_) : Error(message_, "ImporterError") {}
    };
};

#endif /* RESTPOSE_INCLUDED_RSPERRORS_H */
