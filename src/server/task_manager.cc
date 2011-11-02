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

#include "logger/logger.h"
#include "safeerrno.h"
#include "str.h"
#include <sys/select.h>
#include <sys/socket.h>
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

TaskManager::TaskManager(CollectionPool & collections_)
	: nudge_write_end(-1),
	  nudge_read_end(-1),
	  started(false),
	  stopping(false),
	  indexing_queues(100000, 101000), // FIXME - pull out magic constants
	  indexing_threads(),
	  processing_queues(100000, 101000), // FIXME - pull out magic constants
	  processing_threads(),
	  search_queues(1000, 2000), // FIXME - pull out magic constants
	  search_threads(),
	  collections(collections_),
	  collconfigs(collections),
	  checkpoints(100, 24 * 60 * 60) // Keep up to 100 log messages per checkpoint, and keep checkpoints for a day.  FIXME - pull out magic constants
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
TaskManager::queue_readonly(const std::string & queue, ReadonlyTask * task)
{
    std::auto_ptr<ReadonlyTask> taskptr(task);
    {
	ContextLocker lock(cond);
	if (stopping) {
	    LOG_DEBUG("TaskManager queuing readonly task on '" + queue +
		      "' failed - queue closed");
	    return Queue::CLOSED;
	}
    }
    Queue::QueueState result = search_queues.push(queue, taskptr.release(),
						  false);
    LOG_DEBUG("TaskManager queuing readonly task on '" + queue +
	      "': state " + str(result));
    return result;
}

void
TaskManager::queue_indexing_from_processing(const std::string & queue,
					    IndexingTask * task)
{
    auto_ptr<IndexingTask> taskptr(task);

    // Lock the processing queues while we try adding to the indexing queue,
    // to avoid a race condition when the indexing queue is full and we set the
    // processing queue to inactive.
    ContextLocker lock(processing_queues.cond);

    // Try adding the xapian document to the queue; block if the queue gets
    // full, but give up if the queue is closed.
    while (true) {
	Queue::QueueState state =
		indexing_queues.push(queue, taskptr->clone(), false);
	switch (state) {
	    case Queue::HAS_SPACE:
		LOG_DEBUG("TaskManager queued indexing task on '" + queue +
			  "' from processing");
		return;
	    case Queue::LOW_SPACE:
		LOG_DEBUG("TaskManager queued indexing task on '" + queue +
			  "' from processing: low space");
		// Disable this queue, to avoid processing items in it until
		// the corresponding indexer queue is no longer overloaded.
		processing_queues.set_inactive_internal(queue);
		return;
	    case Queue::FULL:
		LOG_DEBUG("TaskManager waiting to queue indexing task on '" +
			  queue + "' from processing: full.");
		// Continue the loop
		processing_queues.set_inactive_internal(queue);
		processing_queues.cond.wait();
		break;
	    case Queue::CLOSED:
		// This shouldn't happen, because we close the processor queues
		// and then wait for them to empty before closing the indexer
		// queues.
		LOG_ERROR("TaskManager unable to queue indexing task on '"
			  + queue + "' from processing: closed.  "
			  "Dropped task.");
		return;
	}
    }
}

Queue::QueueState
TaskManager::queue_indexing(const string & queue,
			    IndexingTask * task,
			    bool allow_throttle)
{
    auto_ptr<IndexingTask> taskptr(task);
    {
	ContextLocker lock(cond);
	if (stopping) {
	    LOG_DEBUG("TaskManager queuing indexing task failed - "
		      "queue closed");
	    return Queue::CLOSED;
	}
    }
    Queue::QueueState result = indexing_queues.push(queue, taskptr.release(),
						    allow_throttle);
    LOG_DEBUG("TaskManager queuing indexing task: state " + str(result));
    return result;
}

Queue::QueueState
TaskManager::queue_processing(const string & queue,
			      ProcessingTask * task,
			      bool allow_throttle,
			      double end_time)
{
    auto_ptr<ProcessingTask> taskptr(task);
    {
	ContextLocker lock(cond);
	if (stopping) {
	    LOG_DEBUG("TaskManager queuing processing task failed - "
		      "queue closed");
	    return Queue::CLOSED;
	}
    }
    Queue::QueueState result = processing_queues.push(queue, taskptr.release(),
						      allow_throttle, end_time);
    LOG_DEBUG("TaskManager queuing processing task: state " + str(result));
    return result;
}

Queue::QueueState
TaskManager::queue_pipe_document(const string & collection,
				 const string & pipe,
				 const Json::Value & doc,
				 bool allow_throttle,
				 double end_time)
{
    return queue_processing(collection,
			    new ProcessorPipeDocumentTask(pipe, doc),
			    allow_throttle,
			    end_time);
}

Queue::QueueState
TaskManager::queue_index_document(const string & collection,
				  Xapian::Document doc,
				  const string & idterm,
				  bool allow_throttle)
{
    return queue_indexing(collection,
			  new IndexerUpdateDocumentTask(idterm, doc),
			  allow_throttle);
}

void
TaskManager::start()
{
    if (started) {
	return;
    }
    started = true;
    LOG_DEBUG("TaskManager starting");

    // Start threads for indexing, processing and searching.
    // FIXME - these should be configurable
    int indexing_thread_count = 2;
    int processing_thread_count = 10;
    int search_thread_count = 10;

    for (int i = indexing_thread_count; i != 0; --i) {
	indexing_threads.add_thread(new IndexingThread(indexing_queues,
						       collections,
						       this));
    }
    for (int i = processing_thread_count; i != 0; --i) {
	processing_threads.add_thread(new ProcessingThread(processing_queues,
							   collections,
							   this));
    }
    for (int i = search_thread_count; i != 0; --i) {
	search_threads.add_thread(new SearchThread(search_queues,
						   collections));
    }
}

void
TaskManager::stop()
{
    LOG_DEBUG("TaskManager stopping");
    ContextLocker lock(cond);
    stopping = true;
    processing_queues.close();
    search_queues.close();
}

void
TaskManager::join()
{
    LOG_DEBUG("TaskManager waiting for processing queue to empty");
    processing_queues.wait_for_empty();
    indexing_queues.close();
    processing_threads.stop();
    LOG_DEBUG("TaskManager waiting for search queue to empty");
    search_queues.wait_for_empty();
    search_threads.stop();
    LOG_DEBUG("TaskManager waiting for indexing queue to empty");
    indexing_queues.wait_for_empty();
    indexing_threads.stop();
    LOG_DEBUG("TaskManager waiting for processing threads to finish");
    processing_threads.join();
    LOG_DEBUG("TaskManager waiting for indexing threads to finish");
    indexing_threads.join();
    LOG_DEBUG("TaskManager waiting for search threads to finish");
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
    // If there's a message on the nudge descriptor, set the processing queue
    // to be active for any indexing queue which is no longer full.
    if (!timed_out && FD_ISSET(nudge_read_end, read_fd_set)) {
	std::string nudge_content;
	if (!io_read_append(nudge_content, nudge_read_end)) {
	    LOG_ERROR("TaskManager: failure to read from nudge pipe: " + str(errno));
	}

	std::vector<std::string> busy_list;
	std::vector<std::string> inactive;
	indexing_queues.get_busy_queues(busy_list);
	processing_queues.get_inactive_queues(inactive);
	std::set<std::string> busy(busy_list.begin(), busy_list.end());
	for (std::vector<std::string>::const_iterator i = inactive.begin();
	     i != inactive.end(); ++i) {
	    if (busy.find(*i) == busy.end()) {
		//printf("TaskManager: reawakening queue %s\n", i->c_str());
		processing_queues.set_active(*i, true);
	    }
	}
    }
}
