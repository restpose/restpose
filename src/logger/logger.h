/** @file logger.h
 * @brief Thread safe logger.
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
#ifndef RESTPOSE_INCLUDED_LOGGER_H
#define RESTPOSE_INCLUDED_LOGGER_H

#include "utils/rsperrors.h"
#include "utils/threading.h"
#include <string>
#include <queue>

namespace Xapian {
    class Error;
};

namespace RestPose {

class Logger : public Thread {
    struct LogMsg {
	double timestamp;
	std::string message;
	unsigned queue_full_count; // If queue is full, this count is incremented.
	LogMsg(double timestamp_, const std::string & message_)
		: timestamp(timestamp_), message(message_), queue_full_count(0)
	{}
    };

    std::queue<LogMsg> queue;
    size_t max_queue_size;

    int log_fd;

    void log(const std::string & message);
  public:
    Logger();

    /** Main loop for the logger thread.
     */
    void run();

    /** Log a debug message.
     *
     *  All actions performed by the system should be logged at this level
     *  (or higher).
     */
    void debug(const std::string & message);

    /** Log an informative message.
     *
     *  Significant actions and changes of state of the system should be
     *  logged at this level.
     */
    void info(const std::string & message);

    /** Log an error which has been handled properly by the system.
     *
     *  For example, an error due to user input, where the bad input has been
     *  detected properly.
     *
     *  The message should contain sufficient information to identify what
     *  the bad input was.
     */
    void warn(const std::string & message);

    /** Log an error which shouldn't occur in normal operation.
     */
    void error(const std::string & message);

    /** Log an error which shouldn't occur in normal operation.
     *
     *  Convenience wrapper for logging a RestPose::Error - displays the
     *  context first, and then a description of the error.
     */
    void error(const std::string & context, const RestPose::Error & err);

    /** Log an error which shouldn't occur in normal operation.
     *
     *  Convenience wrapper for logging a Xapian::Error - displays the
     *  context first, and then a description of the error.
     */
    void error(const std::string & context, const Xapian::Error & err);

    /** Log an error which shouldn't occur in normal operation.
     *
     *  Convenience wrapper for logging a std::bad_alloc - displays the
     *  context first, and then a description of the error.
     */
    void error(const std::string & context, const std::bad_alloc & err);
};

extern Logger g_log;

}

#ifdef RESTPOSE_ENABLE_DEBUGLOG
#define LOG_DEBUG(message) RestPose::g_log.debug(message);
#else
#define LOG_DEBUG(message)
#endif
#define LOG_INFO(message) RestPose::g_log.info(message);
#define LOG_WARN(message) RestPose::g_log.warn(message);
#define LOG_ERROR(...) RestPose::g_log.error(__VA_ARGS__);

#endif /* restpose_included_logger_h */
