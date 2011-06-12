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
#include "logger/logger.h"
#include "realtime.h"
#include "server/thread_pool.h"
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
TaskThread::set_threadpool(ThreadPool * threadpool_)
{
    threadpool = threadpool_;
}

void
TaskThread::release_from_threadpool()
{
    if (threadpool != NULL) {
	threadpool->thread_finished(this);
    }
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

	try {

	    Task * newtask = queuegroup.pop_any(coll_name, task);
	    delete task;
	    task = newtask;

	    if (!task) {
		// Queue has been closed, and is empty.
		return;
	    }

	    ProcessingTask * colltask = static_cast<ProcessingTask *>(task);
	    colltask->perform(coll_name, taskman);

	} catch(const RestPose::Error & e) {
	    LOG_ERROR("Processing failed with", e);
	} catch(const Xapian::DatabaseOpeningError & e) {
	    LOG_ERROR("Processing failed with", e);
	} catch(const Xapian::Error & e) {
	    LOG_ERROR("Processing failed with", e);
	} catch(const std::bad_alloc & e) {
	    LOG_ERROR("Processing failed with", e);
	}
    }
}

void
ProcessingThread::cleanup()
{
    release_from_threadpool();
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

	try {
	    // This call blocks, but should finish when the queue is closed, so
	    // shouldn't delay termination of the thread.
	    if (!queuegroup.assign_handler(coll_name)) {
		// Only happens when queuegroup is closed, so just finish running.
		return;
	    }
	    collection = pool.get_writable(coll_name);

	    while (true) {
		bool is_finished;

		Task * newtask = queuegroup.pop_from(coll_name,
		    RealTime::now() + commit_after_idle, is_finished,
		    task, last_coll_name);
		delete task;
		task = newtask;
		last_coll_name = coll_name;

		if (is_finished) {
		    // Queue has been closed, and is empty.
		    return;
		}
		if (task == NULL) {
		    // Timeout
		    break;
		}
		IndexingTask * colltask = static_cast<IndexingTask *>(task);
		colltask->perform(*collection);
	    }

	    collection->commit();
	    Collection * tmp = collection;
	    collection = NULL;
	    pool.release(tmp);
	    queuegroup.unassign_handler(coll_name);

	} catch(const RestPose::Error & e) {
	    LOG_ERROR("Indexing failed with", e);
	} catch(const Xapian::DatabaseOpeningError & e) {
	    LOG_ERROR("Indexing failed with", e);
	} catch(const Xapian::Error & e) {
	    LOG_ERROR("Indexing failed with", e);
	} catch(const std::bad_alloc & e) {
	    LOG_ERROR("Indexing failed with", e);
	}
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
    release_from_threadpool();
}


void
SearchThread::run()
{
    string group_name;
    while (true) {
	{
	    ContextLocker lock(cond);
	    if (stop_requested) {
		return;
	    }
	}

	Task * newtask = queuegroup.pop_any(group_name, task);
	delete task;
	task = newtask;

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
		if (collection == NULL) {
		    collection = pool.get_readonly(coll_name);
		} else if (collection->get_name() != coll_name) {
		    Collection * tmp = collection;
		    collection = NULL;
		    pool.release(tmp);
		    collection = pool.get_readonly(coll_name);
		}
		coll_name = *coll_name_ptr;
	    }

	    rotask->perform(collection);
	} catch(const RestPose::Error & e) {
	    LOG_ERROR("Readonly task failed with", e);
	    Json::Value result(Json::objectValue);
	    result["err"] = e.what();
	    rotask->resulthandle.failed(result, 500);
	} catch(const Xapian::DatabaseOpeningError & e) {
	    LOG_ERROR("Readonly task failed with", e);
	    Json::Value result(Json::objectValue);
	    result["err"] = e.get_description();
	    rotask->resulthandle.failed(result, 404);
	} catch(const Xapian::Error & e) {
	    LOG_ERROR("Readonly task failed with", e);
	    Json::Value result(Json::objectValue);
	    result["err"] = e.get_description();
	    rotask->resulthandle.failed(result, 500);
	} catch(const std::bad_alloc & e) {
	    LOG_ERROR("Readonly task failed with", e);
	    Json::Value result(Json::objectValue);
	    result["err"] = "out of memory";
	    rotask->resulthandle.failed(result, 503);
	}
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
    release_from_threadpool();
}
