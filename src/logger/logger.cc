/** @file logger.cc
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

#include <config.h>
#include "logger/logger.h"

#include <cstdio>
#include "utils/io_wrappers.h"
#include "realtime.h"
#include "str.h"
#include "utils/rsperrors.h"
#include <xapian.h>

using namespace std;
using namespace RestPose;

Logger::Logger(int log_fd_)
	: Thread(), max_queue_size(100), log_fd(log_fd_)
{}

void
Logger::log(const string & message)
{
    ContextLocker lock(cond);
    double now = RealTime::now();
    if (queue.size() >= max_queue_size) {
	(queue.back().queue_full_count)++;
    } else {
	queue.push(LogMsg(now, message));
	cond.broadcast();
    }
}

void
Logger::process_queue(ContextLocker & lock)
{
    string buf;
    // Write the contents of the queue into the buffer.
    while (!queue.empty()) {
	LogMsg & item = queue.front();
	buf += str(item.timestamp) + ": " + item.message + "\n";
	if (item.queue_full_count) {
	    buf += str(item.timestamp) + ": LOG OVERLOADED - missing " +
		    str(item.queue_full_count) + " entries\n";
	}
	queue.pop();
    }

    // Drop the lock while writing out the buffer.
    if (!buf.empty()) {
	lock.unlock();
	(void)io_write(log_fd, buf);
	lock.lock();
    }
}

void
Logger::run()
{
    ContextLocker lock(cond);
    while (!stop_requested) {
	process_queue(lock);
	if (stop_requested)
	    break;
	if (queue.empty()) {
	    cond.wait();
	}
    }
}

void
Logger::join()
{
    Thread::join();
    ContextLocker lock(cond);
    process_queue(lock);
}

void
Logger::debug(const std::string & message)
{
    log("D:" + message);
}

void
Logger::info(const std::string & message)
{
    log("I:" + message);
}

void
Logger::warn(const std::string & message)
{
    log("W:" + message);
}

void
Logger::error(const std::string & message)
{
    log("E:" + message);
}

void
Logger::error(const std::string & context, const RestPose::Error & err)
{
    log("E:" + context + ": " + err.what());
}

void
Logger::error(const std::string & context, const Xapian::Error & err)
{
    log("E:" + context + ": " + err.get_description());
}

void
Logger::error(const std::string & context, const std::bad_alloc &)
{
    log("E:" + context + ": out of memory");
}

/** Global logger.
 */
Logger RestPose::g_log;
