/** @file task_manager.cc
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

#include <config.h>
#include "task_manager.h"

#include "omassert.h"
#include "realtime.h"
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "safeerrno.h"
#include "utils/jsonutils.h"

#define DIR_SEPARATOR "/"

using namespace std;
using namespace RestPose;

Task::~Task() {}

void
CollInfoTask::perform(RestPose::Collection * collection)
{
    Json::Value & result(resulthandle.result_target());
    result["doc_count"] = collection->doc_count();
    Json::Value & types(result["types"] = Json::objectValue);
    // FIXME - set types to a map from typenames in the collection to their schemas.
    (void) types;
    resulthandle.set_ready();
}

void
PerformSearchTask::perform(RestPose::Collection * collection)
{
    Json::Value & result(resulthandle.result_target());
    collection->perform_search(search, result);
    resulthandle.set_ready();
}

void
ServerStatusTask::perform(RestPose::Collection *)
{
    Json::Value & result(resulthandle.result_target());
    Json::Value & tasks(result["tasks"] = Json::objectValue);
    {
	Json::Value & indexing(tasks["indexing"] = Json::objectValue);
	taskman->indexing_queues.get_status(indexing["queues"]);
	taskman->indexing_threads.get_status(indexing["threads"]);
    }
    {
	Json::Value & processing(tasks["processing"] = Json::objectValue);
	taskman->processing_queues.get_status(processing["queues"]);
	taskman->processing_threads.get_status(processing["threads"]);
    }
    {
	Json::Value & search(tasks["search"] = Json::objectValue);
	taskman->search_queues.get_status(search["queues"]);
	taskman->search_threads.get_status(search["threads"]);
    }
    resulthandle.set_ready();
}

void
ProcessorPipeDocumentTask::perform(RestPose::Collection & collection)
{
    collection.send_to_pipe(taskman, pipe, doc);
}

void
ProcessorProcessDocumentTask::perform(RestPose::Collection & collection)
{
    string idterm;
    Xapian::Document xdoc = collection.process_doc(type, doc, idterm);
    taskman->queue_index_processed_doc(collection.get_name(), xdoc, idterm);
}

void
IndexerUpdateDocumentTask::perform(RestPose::Collection & collection)
{
    collection.raw_update_doc(doc, idterm);
}

void
IndexerDeleteDocumentTask::perform(RestPose::Collection & collection)
{
    collection.raw_delete_doc(idterm);
}

void
IndexerCommitTask::perform(RestPose::Collection & collection)
{
    collection.commit();
}


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

	CollUpdateTask * colltask = static_cast<CollUpdateTask *>(task);
	if (collection == NULL) {
	    collection = pool.get_readonly(coll_name);
	} else if (collection->get_name() == coll_name) {
	    Collection * tmp = collection;
	    collection = NULL;
	    pool.release(tmp);
	    collection = pool.get_readonly(coll_name);
	}
	colltask->perform(*collection);
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
	    CollUpdateTask * colltask = static_cast<CollUpdateTask *>(task);
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

	SearchTask * searchtask = static_cast<SearchTask *>(task);
	const string * coll_name_ptr = searchtask->get_coll_name();
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
	    Json::Value & result = searchtask->resulthandle.result_target();
	    result = Json::objectValue;
	    result["err"] = e.what();
	    searchtask->resulthandle.set_status(500);
	    searchtask->resulthandle.set_ready();
	    continue;
	} catch(const Xapian::DatabaseOpeningError & e) {
	    Json::Value & result = searchtask->resulthandle.result_target();
	    result["err"] = e.get_description();
	    searchtask->resulthandle.set_status(404);
	    searchtask->resulthandle.set_ready();
	    continue;
	} catch(const Xapian::Error & e) {
	    Json::Value & result = searchtask->resulthandle.result_target();
	    result["err"] = e.get_description();
	    searchtask->resulthandle.set_status(500);
	    searchtask->resulthandle.set_ready();
	    continue;
	} catch(const std::bad_alloc) {
	    Json::Value & result = searchtask->resulthandle.result_target();
	    result["err"] = "out of memory";
	    searchtask->resulthandle.set_status(503);
	    searchtask->resulthandle.set_ready();
	    continue;
	}
	searchtask->perform(collection);
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

ThreadPool::~ThreadPool()
{
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->stop();
    }
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->join();
    }
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	delete *i;
    }
}

void
ThreadPool::add_thread(Thread * thread)
{
    ContextLocker lock(mutex);
    auto_ptr<Thread> threadptr(thread);
    threads.push_back(NULL);
    threads.back() = threadptr.release();
    thread->start();
}

void
ThreadPool::stop()
{
    ContextLocker lock(mutex);
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->stop();
    }
}

void
ThreadPool::join()
{
    ContextLocker lock(mutex);
    for (std::vector<Thread *>::iterator i = threads.begin();
	 i != threads.end(); ++i) {
	(*i)->join();
    }
}

void
ThreadPool::get_status(Json::Value & status) const
{
    ContextLocker lock(mutex);
    status["size"] = uint64_t(threads.size());
}


TaskManager::TaskManager(const string & datadir_)
	: datadir(datadir_),
	  nudge_write_end(-1),
	  nudge_read_end(-1),
	  started(false),
	  stopping(false),
	  indexing_queues(100, 1000), // FIXME - pull out magic constants
	  indexing_threads(),
	  processing_queues(100, 1000), // FIXME - pull out magic constants
	  processing_threads(),
	  search_queues(100, 1000), // FIXME - pull out magic constants
	  search_threads(),
	  collections(datadir)
{
    // Create the nudge socket.
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
    if (ret == -1) {
	throw RestPose::SysError("Couldn't create internal socketpair", errno);
    }
    nudge_write_end = fds[0];
    nudge_read_end = fds[1];

    indexing_queues.set_nudge(nudge_write_end, 'I');
    processing_queues.set_nudge(nudge_write_end, 'P');
    search_queues.set_nudge(nudge_write_end, 'S');
}

TaskManager::~TaskManager()
{
    (void)io_close(nudge_write_end);
    (void)io_close(nudge_read_end);
    search_queues.close();
    processing_queues.close();
    processing_queues.wait_for_empty();
    indexing_queues.close();
    processing_threads.stop();
    indexing_queues.wait_for_empty();
    indexing_threads.stop();
    search_queues.wait_for_empty();
    search_threads.stop();
    processing_threads.join();
    indexing_threads.join();
    search_threads.join();
}

Queue::QueueState
TaskManager::queue_get_status(const RestPose::ResultHandle & resulthandle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return search_queues.push("status",
	new ServerStatusTask(resulthandle, this), false);
}

Queue::QueueState
TaskManager::queue_get_collinfo(const std::string & collection,
				const RestPose::ResultHandle & resulthandle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return search_queues.push("info",
	new CollInfoTask(resulthandle, collection), false);
}

Queue::QueueState
TaskManager::queue_search(const std::string & collection,
			  const RestPose::ResultHandle & resulthandle,
			  const Json::Value & search)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return search_queues.push("search",
	new PerformSearchTask(resulthandle, collection, search), false);
}

void
TaskManager::queue_index_processed_doc(const std::string & collection,
				       Xapian::Document xdoc,
				       const std::string & idterm)
{
    // Lock the processing queues while we try adding to the indexing queue,
    // to avoid a race condition when the indexing queue is full and we set the
    // processing queue to inactive.
    ContextLocker lock(processing_queues.cond);

    // Try adding the xapian document to the queue; block if the queue gets
    // full, but give up if the queue is closed.
    while (true) {
    Queue::QueueState state = indexing_queues.push(collection,
	    new IndexerUpdateDocumentTask(idterm, xdoc), false);
	switch (state) {
	    case Queue::HAS_SPACE:
		return;
	    case Queue::LOW_SPACE:
		// Disable this queue, to avoid processing items in it until
		// the corresponding indexer queue is no longer overloaded.
		processing_queues.set_inactive_internal(collection);
		return;
	    case Queue::FULL:
		// Continue the loop
		processing_queues.set_inactive_internal(collection);
		processing_queues.cond.wait();
		break;
	    case Queue::CLOSED:
		// This shouldn't happen, because we close the processor queues
		// and then wait for them to empty before closing the indexer
		// queues.
		// FIXME - log that data has been dropped.
		return;
	}
    }
}

Queue::QueueState
TaskManager::queue_pipe_document(const string & collection,
				 const string & pipe,
				 const Json::Value & doc,
				 bool allow_throttle,
				 double end_time)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return processing_queues.push(collection,
	new ProcessorPipeDocumentTask(this, pipe, doc), allow_throttle,
	end_time);
}

Queue::QueueState
TaskManager::queue_process_document(const string & collection,
				    const std::string & type,
				    const Json::Value & doc,
				    bool allow_throttle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return processing_queues.push(collection,
	new ProcessorProcessDocumentTask(this, type, doc), allow_throttle);
}

Queue::QueueState
TaskManager::queue_index_document(const string & collection,
				  Xapian::Document doc,
				  const string & idterm,
				  bool allow_throttle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return indexing_queues.push(collection,
				new IndexerUpdateDocumentTask(idterm, doc),
				allow_throttle);
}

Queue::QueueState
TaskManager::queue_delete_document(const string & collection,
				   const string & idterm,
				   bool allow_throttle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return indexing_queues.push(collection,
				new IndexerDeleteDocumentTask(idterm),
				allow_throttle);
}

Queue::QueueState
TaskManager::queue_commit(const std::string & collection,
			  bool allow_throttle)
{
    {
	ContextLocker lock(cond);
	if (stopping) {
	    return Queue::CLOSED;
	}
    }
    return indexing_queues.push(collection,
				new IndexerCommitTask(),
				allow_throttle);
}

void
TaskManager::start()
{
    if (started) {
	return;
    }
    started = true;
    //printf("TaskManager::start()\n");
    // Start threads for indexing and processing.
    // FIXME - these should be configurable
    int indexing_thread_count = 2;
    int processing_thread_count = 10;
    int search_thread_count = 10;

    for (int i = indexing_thread_count; i != 0; --i) {
	indexing_threads.add_thread(new IndexingThread(indexing_queues,
						       collections));
    }
    for (int i = processing_thread_count; i != 0; --i) {
	processing_threads.add_thread(new ProcessingThread(processing_queues,
							   collections));
    }
    for (int i = search_thread_count; i != 0; --i) {
	search_threads.add_thread(new SearchThread(search_queues,
						   collections));
    }
}

void
TaskManager::stop()
{
    ContextLocker lock(cond);
    stopping = true;
    processing_queues.close();
    search_queues.close();
}

void
TaskManager::join()
{
    processing_queues.wait_for_empty();
    indexing_queues.close();
    processing_threads.stop();
    search_queues.wait_for_empty();
    search_threads.stop();
    indexing_queues.wait_for_empty();
    indexing_threads.stop();
    processing_threads.join();
    indexing_threads.join();
    search_threads.join();
}

void
TaskManager::get_fdsets(fd_set * read_fd_set,
			fd_set *,
			fd_set *,
			int * max_fd,
			bool * ,
			uint64_t *)
{
    FD_SET(nudge_read_end, read_fd_set);
    if (nudge_read_end > *max_fd)
	*max_fd = nudge_read_end;
}

void
TaskManager::serve(fd_set * read_fd_set, fd_set *, fd_set *, bool timed_out)
{
    //printf("TaskManager::serve()\n");
    // If there's a message on the nudge descriptor, set the processing queue
    // to be active for any indexing queue which is no longer full.
    if (!timed_out && FD_ISSET(nudge_read_end, read_fd_set)) {
	std::string nudge_content;
	if (!io_read_append(nudge_content, nudge_read_end)) {
	    // FIXME - log the read failure.
	    fprintf(stderr, "TaskManager: failure to read from nudge pipe: %d\n", errno);
	}

	std::vector<std::string> queues;
	indexing_queues.get_queues_with_space(queues);
	for (std::vector<std::string>::const_iterator i = queues.begin();
	     i != queues.end(); ++i) {
	    //printf("TaskManager: reawakening queue %s\n", i->c_str());
	    processing_queues.set_active(*i, true);
	}
    }
}
