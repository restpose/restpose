/** @file basetasks.h
 * @brief Base classes of tasks.
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
#ifndef RESTPOSE_INCLUDED_BASETASKS_H
#define RESTPOSE_INCLUDED_BASETASKS_H

#include <map>
#include "server/result_handle.h"
#include "server/server.h"
#include <string>
#include <vector>
#include <xapian.h>

namespace RestPose {
    class Collection;
};

class TaskManager;

/** Base class of all tasks performed.
 */
class Task {
    Task(const Task &);
    void operator=(const Task &);
  public:
    /** Flag to indicate if other tasks from the same queue may be run in
     * parallel with this task.
     */
    bool allow_parallel;

    Task(bool allow_parallel_=true)
	    : allow_parallel(allow_parallel_)
    {}
    virtual ~Task();
};

/// A task for the readonly task queue.
class ReadonlyTask : public Task {
  public:
    RestPose::ResultHandle resulthandle;

    ReadonlyTask(const RestPose::ResultHandle & resulthandle_)
	    : resulthandle(resulthandle_)
    {}

    /** Get the collection name for this task.
     *
     *  Return NULL if no collection is used.
     *
     *  If NULL is returned, perform() will be called with collection=NULL.
     *  Otherwise, perform() will not be called with collection=NULL.
     */
    virtual const std::string * get_coll_name() const
    {
	return NULL;
    }

    virtual void perform(RestPose::Collection * collection) = 0;
};

/// A task for the readonly task queue, for a specific collection.
class ReadonlyCollTask : public ReadonlyTask {
    std::string coll_name;
  public:
    ReadonlyCollTask(const RestPose::ResultHandle & resulthandle_,
		     const std::string & coll_name_)
	    : ReadonlyTask(resulthandle_),
	      coll_name(coll_name_)
    {}

    const std::string * get_coll_name() const
    {
	return &coll_name;
    }
};

/** A task for the processing queue for a collection.
 */
class ProcessingTask : public Task {
  public:
    ProcessingTask(bool allow_parallel_=true) : Task(allow_parallel_) {}
    virtual void perform(const std::string & coll_name,
			 TaskManager * taskman) = 0;
};

/** A task for the indexing queue for a collection.
 */
class IndexingTask : public Task {
  public:
    IndexingTask() : Task(false) {}

    void perform(const std::string & coll_name,
		 RestPose::Collection * & collection,
		 TaskManager * taskman);

    /** Perform the task.
     *
     *  May raise exceptions to report failure - these will be caught, logged
     *  and reported appropriately.
     */
    virtual void perform_task(const std::string & coll_name,
			      RestPose::Collection * & collection,
			      TaskManager * taskman) = 0;

    /** Get a description of the task, and the document type and id it's
     *  operating on (return blank for the type and/or id if they're not
     *  applicable for this task).
     */
    virtual void info(std::string & description,
		      std::string & doc_type,
		      std::string & doc_id) const = 0;

    /** Perform cleanup actions after the main body of performing the task.
     *
     *  Should not raise exceptions.
     *
     *  Default implementation does nothing.
     */
    virtual void post_perform(RestPose::Collection * collection,
			      TaskManager * taskman);

    /** Clone this task.  Not frequently called - used when queue is full, and
     *  the task needs to be re-queued.
     */
    virtual IndexingTask * clone() const = 0;
};

/** A wrapper around a IndexingTask.
 *
 *  This is used to place it on the processing queue, to allow processing tasks
 *  which have to happen before / after it to be sequenced correctly.
 *
 *  Note that allow_parallel is always set to false for this, to ensure
 *  ordering is preserved while on the processing queue.
 */
class DelayedIndexingTask : public ProcessingTask {
    IndexingTask * task;
  public:
    DelayedIndexingTask(IndexingTask * task_)
	    : ProcessingTask(false), task(task_)
    {}

    ~DelayedIndexingTask();

    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

#endif /* RESTPOSE_INCLUDED_BASETASKS_H */
