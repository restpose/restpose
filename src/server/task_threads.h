/** @file task_threads.h
 * @brief The threads which perform tasks supplied via the task manager.
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
#ifndef RESTPOSE_INCLUDED_TASK_THREADS_H
#define RESTPOSE_INCLUDED_TASK_THREADS_H

#include "jsonxapian/collection.h"
#include "server/collection_pool.h"
#include "server/server.h"
#include "server/task_queue_group.h"
#include <string>
#include "utils/io_wrappers.h"

class TaskManager;

/** A thread for performing a type of task.
 */
class TaskThread : public Thread {
  protected:
    /** Reference to queues of tasks.
     */
    TaskQueueGroup & queuegroup;

    /** The pool to get collections from and return collections to.
     */
    CollectionPool & pool;

    /** The current collection the indexer is working on.
     */
    RestPose::Collection * collection;

    TaskThread(const TaskThread &);
    void operator=(const TaskThread &);
  public:
    /** Create an indexer for a collection.
     */
    TaskThread(TaskQueueGroup & queuegroup_,
	       CollectionPool & pool_)
	    : Thread(),
	      queuegroup(queuegroup_),
	      pool(pool_),
	      collection(NULL)
    {}

    ~TaskThread();
};

/** An processor thread.
 */
class ProcessingThread : public TaskThread {
    /** The name of the current collection.
     */
    std::string coll_name;

    /** The task manager for this thread.
     */
    TaskManager * taskman;

  public:
    /** Create an indexer for a collection.
     */
    ProcessingThread(TaskQueueGroup & queuegroup_,
		     CollectionPool & pool_,
		     TaskManager * taskman_)
	    : TaskThread(queuegroup_, pool_),
	      taskman(taskman_)
    {}

    /* Standard thread methods. */
    void run();
    void cleanup();
};

/** An indexer thread.
 */
class IndexingThread : public TaskThread {
    /** The name of the current collection.
     */
    std::string coll_name;

  public:
    /** Create an indexer for a collection.
     */
    IndexingThread(TaskQueueGroup & queuegroup_,
		  CollectionPool & pool_)
	    : TaskThread(queuegroup_, pool_)
    {}

    /* Standard thread methods. */
    void run();
    void cleanup();
};

/** A search thread.
 */
class SearchThread : public TaskThread {
    /** The name of the current collection.
     */
    std::string coll_name;

  public:
    /** Create an indexer for a collection.
     */
    SearchThread(TaskQueueGroup & queuegroup_, CollectionPool & pool_)
	    : TaskThread(queuegroup_, pool_)
    {}

    /* Standard thread methods. */
    void run();
    void cleanup();
};

#endif /* RESTPOSE_INCLUDED_TASK_THREADS_H */