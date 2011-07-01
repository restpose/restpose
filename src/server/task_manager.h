/** @file task_manager.h
 * @brief Manage access and updates to collections.
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
#ifndef RESTPOSE_INCLUDED_TASK_MANAGER_H
#define RESTPOSE_INCLUDED_TASK_MANAGER_H

#include "jsonxapian/collconfigs.h"
#include "jsonxapian/collection.h"
#include "jsonxapian/collection_pool.h"
#include "server/checkpoints.h"
#include "server/result_handle.h"
#include "server/server.h"
#include "server/tasks.h"
#include "server/thread_pool.h"
#include "server/task_queue_group.h"
#include "server/task_threads.h"
#include <string>
#include "utils/queueing.h"
#include "utils/io_wrappers.h"
#include <xapian.h>

namespace Xapian {
    class Document;
};

class TaskManager;

/** Manager for all non-instantaneous tasks.
 */
class TaskManager : public SubServer {
    friend class ProcessorProcessDocumentTask;
    friend class ProcessorPipeDocumentTask;
    friend class ServerStatusTask;
    friend class DelayedCollIndexTask;

    /** Condition containing lock to be held whenever accessing mutable
     *  internal state.
     */
    Condition cond;

    /** The write end of the nudge pipe.
     *
     *  This is given to queues, so they can notify the manager when they
     *  stop being overloaded.
     */
    int nudge_write_end;

    /** The read end of the nudge pipe.
     *
     *  This is selected on in the server.
     */
    int nudge_read_end;

    /** Flag used to ensure we only get started once.
     */
    bool started;

    /** Flag set when stopping to stop any new jobs being pushed.
     */
    bool stopping;

    /** Queues, keyed by collection name, for indexing to a collection.
     */
    TaskQueueGroup indexing_queues;

    /** Threads performing indexing.
     */
    ThreadPool indexing_threads;

    /** Queues, keyed by collection name, for processing documents to be
     *  indexed to a collection.
     */
    TaskQueueGroup processing_queues;

    /** Threads processing documents.
     */
    ThreadPool processing_threads;

    /** Queues for searching, and getting info.
     */
    TaskQueueGroup search_queues;

    /** Threads for searching, and getting info.
     */
    ThreadPool search_threads;

    /** The pool of collections used by tasks.
     */
    CollectionPool & collections;

    /** The collection configs used by processing tasks.
     */
    RestPose::CollectionConfigs collconfigs;

    /** The checkpoints and recent logs.
     */
    CheckPointManager checkpoints;

    TaskManager(const TaskManager &);
    void operator=(const TaskManager &);
  public:
    TaskManager(CollectionPool & collections_);
    ~TaskManager();

    CollectionPool & get_collections() {
	return collections;
    }

    RestPose::CollectionConfigs & get_collconfigs() {
	return collconfigs;
    }

    CheckPointManager & get_checkpoints() {
	return checkpoints;
    }

    /** Get the write end of the nudge pipe.
     *
     *  This is used by resulthandlers to nudge the server when results are
     *  ready.
     */
    int get_nudge_fd() const {
	return nudge_write_end;
    }

    /** Queue a readonly task, on the named queue.
     */
    Queue::QueueState queue_readonly(const std::string & queue,
				     ReadonlyTask * task);

    /** Queue an indexing task from a processing task, on the named queue.
     *
     *  If the indexing queue is full, this will block until the queue has
     *  space.  It will also disable processing of items from the corresponding
     *  processing queue until the indexing queue size drops to the lower
     *  threshold.
     */
    void queue_indexing_from_processing(const std::string & queue,
					IndexingTask * task);

    /** Queue an indexing task.
     */
    Queue::QueueState queue_indexing(const std::string & queue,
				     IndexingTask * task,
				     bool allow_throttle);

    /** Queue a processing task.
     */
    Queue::QueueState queue_processing(const std::string & queue,
				       ProcessingTask * task,
				       bool allow_throttle,
				       double end_time=0.0);


    /** Queue a document for indexing.
     *
     *  This is intended to be called from the output of processing.
     *  It blocks and retries if the queue is full, and disables the processing
     *  queue for the collection if the queue is starting to have low space.
     */
    void queue_index_processed_doc(const std::string & collection,
				   Xapian::Document xdoc,
				   const std::string & idterm);

    Queue::QueueState queue_pipe_document(const std::string & collection,
					  const std::string & pipe,
					  const Json::Value & doc,
					  bool allow_throttle,
					  double end_time);

    /** Queue a document to be indexed.
     *
     * @param collection The name of the collection to index to.
     * @param doc The document to index.
     * @param idterm A term with a unique ID to be indexed.
     * @param allow_throttle If true, don't queue the document is the queue is
     * already busy.  Otherwise, queue the document unless the queue is
     * completely full.
     *
     * Allow throttle should be specified for documents being read from a
     * stream, if the source of documents can be paused to allow the queue to
     * catch up.  This allows other sources of documents for indexing (such as
     * pushes to the API) to have a chance at getting onto the queue.
     */
    Queue::QueueState queue_index_document(const std::string & collection,
					   Xapian::Document doc,
					   const std::string & idterm,
					   bool allow_throttle);

    /** Queue deletion of a document.
     */
    Queue::QueueState queue_delete_document(const std::string & collection,
					    const std::string & idterm,
					    bool allow_throttle);

    /** Initialise the task manager server.
     */
    void start();

    /** Stop the task manager server, and any subthreads / processes.
     */
    void stop();

    /** Wait for any subthreads / processes to finish, and do final cleanup.
     */
    void join();

    /** Set the fdsets for any fds we're interested in the main server
     *  selecting on.
     */
    void get_fdsets(fd_set * read_fd_set,
		    fd_set * write_fd_set,
		    fd_set * except_fd_set,
		    int * max_fd,
		    bool * have_timeout,
		    uint64_t * timeout);

    /**
     */
    void serve(fd_set * read_fd_set,
	       fd_set * write_fd_set,
	       fd_set * except_fd_set,
	       bool timed_out);
};

#endif /* RESTPOSE_INCLUDED_TASK_MANAGER_H */
