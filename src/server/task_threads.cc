/** @file task_threads.cc
 * @brief Threads for performing tasks.
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
#include "task_threads.h"

#include "httpserver/response.h"
#include "realtime.h"
#include "utils/jsonutils.h"

#define DIR_SEPARATOR "/"

using namespace std;
using namespace RestPose;

TaskThread::~TaskThread()
{
    // Delete the collection - it should have been returned to the pool
    // already, so if it's still here we had an error, and there may be
    // something wrong with it so we don't want it to be reused.
    delete collection;
}


void
ProcessingThread::run()
{
    while (true) {
	{
	    ContextLocker lock(cond);
	    if (stop_requested) {
		return;
	    }
	}

	Task * task = queuegroup.pop_any(coll_name);
	std::auto_ptr<Task> taskptr(task);
	if (!task) {
	    // Queue has been closed, and is empty.
	    return;
	}

	ProcessingTask * colltask = static_cast<ProcessingTask *>(task);
	if (collection == NULL) {
	    collection = pool.get_readonly(coll_name);
	} else if (collection->get_name() == coll_name) {
	    Collection * tmp = collection;
	    collection = NULL;
	    pool.release(tmp);
	    collection = pool.get_readonly(coll_name);
	}
	colltask->perform(*collection, taskman);
    }
}

void
ProcessingThread::cleanup()
{
    if (collection) {
	Collection * tmp = collection;
	collection = NULL;
	pool.release(tmp);
    }
}


void
IndexingThread::run()
{
    // Number of seconds of idle time to commit after.
    // FIXME - pull this out into a config file somewhere.
    double commit_after_idle = 5;
    while (true) {
	{
	    ContextLocker lock(cond);
	    if (stop_requested) {
		return;
	    }
	}

	// This call blocks, but should finish when the queue is closed, so
	// shouldn't delay termination of the thread.
	if (!queuegroup.assign_handler(coll_name)) {
	    // Only happens when queuegroup is closed, so just finish running.
	    return;
	}
	collection = pool.get_writable(coll_name);

	while (true) {
	    bool is_finished;
	    Task * task = queuegroup.pop_from(coll_name,
		RealTime::now() + commit_after_idle, is_finished);
	    if (is_finished) {
		// Queue has been closed, and is empty.
		return;
	    }
	    if (task == NULL) {
		// Timeout
		break;
	    }
	    std::auto_ptr<Task> taskptr(task);
	    IndexingTask * colltask = static_cast<IndexingTask *>(task);
	    colltask->perform(*collection);
	}

	collection->commit();
	Collection * tmp = collection;
	collection = NULL;
	pool.release(tmp);
	queuegroup.unassign_handler(coll_name);
    }
}

void
IndexingThread::cleanup()
{
    if (collection) {
	collection->commit();
	Collection * tmp = collection;
	collection = NULL;
	pool.release(tmp);
	queuegroup.unassign_handler(coll_name);
    }
}


void
SearchThread::run()
{
    while (true) {
	{
	    ContextLocker lock(cond);
	    if (stop_requested) {
		return;
	    }
	}

	string group_name;
	Task * task = queuegroup.pop_any(group_name);
	std::auto_ptr<Task> taskptr(task);
	if (!task) {
	    // Queue has been closed, and is empty.
	    return;
	}

	ReadonlyTask * rotask = static_cast<ReadonlyTask *>(task);
	const string * coll_name_ptr = rotask->get_coll_name();
	try {
	    if (coll_name_ptr == NULL) {
		if (collection != NULL) {
		    Collection * tmp = collection;
		    collection = NULL;
		    pool.release(tmp);
		}
	    } else {
		coll_name = *coll_name_ptr;
		if (collection == NULL) {
		    collection = pool.get_readonly(coll_name);
		} else if (collection->get_name() != coll_name) {
		    Collection * tmp = collection;
		    collection = NULL;
		    pool.release(tmp);
		    collection = pool.get_readonly(coll_name);
		}
	    }
	} catch(const RestPose::Error & e) {
	    Json::Value result(Json::objectValue);
	    result["err"] = e.what();
	    rotask->resulthandle.response().set(result, 500);
	    rotask->resulthandle.set_ready();
	    continue;
	} catch(const Xapian::DatabaseOpeningError & e) {
	    Json::Value result(Json::objectValue);
	    result["err"] = e.get_description();
	    rotask->resulthandle.response().set(result, 404);
	    rotask->resulthandle.set_ready();
	    continue;
	} catch(const Xapian::Error & e) {
	    Json::Value result(Json::objectValue);
	    result["err"] = e.get_description();
	    rotask->resulthandle.response().set(result, 500);
	    rotask->resulthandle.set_ready();
	    continue;
	} catch(const std::bad_alloc) {
	    Json::Value result(Json::objectValue);
	    result["err"] = "out of memory";
	    rotask->resulthandle.response().set(result, 503);
	    rotask->resulthandle.set_ready();
	    continue;
	}
	rotask->perform(collection);
    }
}

void
SearchThread::cleanup()
{
    if (collection) {
	Collection * tmp = collection;
	collection = NULL;
	pool.release(tmp);
    }
}
