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

#include "jsonxapian/collection.h"
#include <map>
#include <memory>
#include "omassert.h"
#include <queue>
#include "server/collection_pool.h"
#include "server/result_handle.h"
#include "server/server.h"
#include "server/tasks.h"
#include "server/thread_pool.h"
#include <string>
#include "utils/queueing.h"
#include "utils/io_wrappers.h"
#include <vector>
#include <xapian.h>

namespace Xapian {
    class Document;
};

class TaskManager;

/** A group of queues of tasks, keyed by a name.
 */
class TaskQueueGroup {
    friend class TaskManager;

    /** A queue in the group, and associated information.
     */
    struct QueueInfo {
	/** The actual queue of tasks.
	 */
	std::queue<Task *> queue;

	/** True for queues which are active, and false for queues which are
	 *  disabled.  Disabled queues can still be pushed to, but pop
	 *  operations will behave as if the queue was empty.
	 *
	 *  Closing the group activates all queues, so that the final items in
	 *  the queues can be popped.
	 */
	bool active;

	/** True for queues which have a dedicated handler assigned.
	 *
	 *  This has no effect on pushes or pops, but is used by 
	 */
	bool assigned;

	/** Create a new, empty, active queue.
	 */
	QueueInfo() : queue(), active(true), assigned(false) {}
    };

    /** The queues of tasks.
     */
    std::map<std::string, QueueInfo> queues;

    /** The queue that the last generic pop was from.
     *
     *  This is used to implement round-robin popping from queues.
     */
    std::string last_pop_from;

    mutable Condition cond;
    bool closed;
    size_t throttle_size;
    size_t max_size;
    int nudge_fd;
    char nudge_byte;

    /** Pick a queue which is active and not assigned.
     *
     *  Block until there is such a queue, or the group is closed.  If the
     *  group is closed without finding such a queue, return queues.end().
     */
    std::map<std::string, QueueInfo>::iterator pick_queue() {
	std::map<std::string, QueueInfo>::iterator i, j;
	while (true) {
	    if (!queues.empty()) {
		// Move i and j to point to queue after last popped from.
		i = queues.lower_bound(last_pop_from + std::string(1, '\0'));
		if (i == queues.end()) {
		    i = queues.begin();
		}
		j = i;

		// Try and find an unassigned, active queue with items in it.
		while (true) {
		    if (i->second.active && !i->second.assigned &&
			!i->second.queue.empty()) {
			last_pop_from = i->first;
			return i;
		    }

		    ++i;
		    if (i == queues.end()) {
			i = queues.begin();
		    }
		    if (i == j) {
			break;
		    }
		}
	    }
	    if (closed) {
		return queues.end();
	    }
	    cond.wait();
	}
    }

    /** Disable processing of a queue, assuming the lock is already held.
     *
     *  If the queue group is closed, queues can no longer be deactivated.
     */
    void set_inactive_internal(const std::string & key)
    {
	if (closed) {
	    return;
	}
	queues[key].active = false;
    }

  public:
    /** create a new queue group.
     *
     *  @param throttle_size_ the size at which to warn that a queue is
     *  getting full.
     *
     *  @param max_size_ the size at which to prevent adding items to the
     *  queue.
     */
    TaskQueueGroup(size_t throttle_size_, size_t max_size_)
	    : closed(false),
	      throttle_size(throttle_size_),
	      max_size(max_size_),
	      nudge_fd(-1),
	      nudge_byte('Q')
    {
    }

    ~TaskQueueGroup()
    {
	// Everything _should_ have been removed from the task queue when we
	// get to this point, but just in case it hasn't, we delete any
	// remaining things.
	//
	// No need to get the condition lock, since if any other threads are
	// still running pointing at the task queue group we've got bigger
	// problems to worry about.
	for (std::map<std::string, QueueInfo>::iterator
	     i = queues.begin(); i != queues.end(); ++i) {
	    std::queue<Task *> & queue = i->second.queue;
	    while (!queue.empty()) {
		delete queue.front();
		queue.pop();
	    }
	}
    }

    /** Set the nudge filedescriptor and byte.
     *
     *  The nudge filedescriptor is used so that an async server waiting on
     *  select can be woken when a queue becomes no-longer overloaded.
     *
     *  @param nudge_fd_ A filedescriptor that a nudge byte will be
     *  written to when a queue has been overloaded, and stops being
     *  overloaded.  Set to -1 to disable nudges.
     *
     *  @param nudge_byte_ The nudge character to write to the nudge file
     *  descriptor.
     */
    void set_nudge(int nudge_fd_, char nudge_byte_)
    {
	ContextLocker lock(cond);
	nudge_fd = nudge_fd_;
	nudge_byte = nudge_byte_;
    }

    /** Close all queues, and prevent new queues being created.
     *
     *  Prevents further items being added to the queues, and causes pop
     *  operations on an empty queue to return immediately, rather than
     *  blocking.
     *
     *  Also enables any disabled queues, to give them a chance to be cleared.
     */
    void close()
    {
	ContextLocker lock(cond);
	closed = true;

	for (std::map<std::string, QueueInfo>::iterator
	     i = queues.begin(); i != queues.end(); ++i) {
	    i->second.active = true;
	}

	cond.broadcast();
    }

    /** Enable, or disable, processing of a queue.
     *
     *  When a queue is inactive, pop operations will not return items from the
     *  queue, though push operations will still allow items to be pushed as
     *  normal.
     *
     *  If the queue group is closed, queues can no longer be deactivated.
     */
    void set_active(const std::string & key, bool on)
    {
	ContextLocker lock(cond);
	if (closed) {
	    return;
	}
	queues[key].active = on;
	cond.broadcast();
    }

    /** Push to a queue.
     *
     *  The queue takes ownership of the item pushed immediately, and
     *  deletes it even if pushing fails.
     *
     *  If the queue is full, will not push the item to the queue, and
     *  returns a state of FULL.
     *
     *  If the queue is full enough to be throttled, and allow_throttle
     *  is true, this will not push the item to the queue, and will
     *  return a state of FULL.
     *
     *  If the queue is closed, will not push the item to the queue, and
     *  returns a state of CLOSED.
     *
     *  If any other value is returned, the item has been pushed onto the
     *  queue.
     *
     *  @param key The queue to push the item onto.
     *
     *  @param item The item to push onto the queue.
     *
     *  @param allow_throttle If true, don't push the item if the queue
     *  has throttle_size or more items in it already (and return FULL).
     *  If false, allow items to be pushed if the queue has max_size or
     *  more items.
     *
     *  @returns FULL or CLOSED if the queue is full or closed
     *  respectively, as described. In either of these cases, the item
     *  has not been pushed onto the queue - otherwise the item has been
     *  pushed onto the queue.  Returns HAS_SPACE if the queue has plenty
     *  of space available, or LOW_SPACE if the queue is running low on
     *  space (this also implies that the item would not have been pushed
     *  if allow_throttle were true).
     */
    Queue::QueueState push(const std::string & key,
			   Task * item, bool allow_throttle,
			   double end_time=0.0) {
	std::auto_ptr<Task> itemptr(item);
	ContextLocker lock(cond);

	std::map<std::string, QueueInfo>::iterator i = queues.find(key);
	if (i == queues.end()) {
	    // Insert a new element.
	    std::pair<std::map<std::string, QueueInfo>::iterator, bool> ret;
	    std::pair<std::string, QueueInfo> newitem;
	    newitem.first = key;
	    ret = queues.insert(newitem);
	    Assert(ret.second);
	    i = ret.first;
	}

	QueueInfo & queue = i->second;

	while(true) {
	    if (closed) {
		return Queue::CLOSED;
	    }

	    if ((allow_throttle && (queue.queue.size() >= throttle_size)) ||
		(!allow_throttle && (queue.queue.size() >= max_size))) {
		if (end_time == 0.0) {
		    return Queue::FULL;
		} else {
		    if (cond.timedwait(end_time)) {
			return Queue::FULL;
		    }
		    continue;
		}
	    }
	    break;
	}

	queue.queue.push(NULL);
	queue.queue.back() = itemptr.release();
	Queue::QueueState result;
	size_t size = queue.queue.size();
	if (size < throttle_size) {
	    result = Queue::HAS_SPACE;
	} else {
	    result = Queue::LOW_SPACE;
	}
	cond.broadcast();
	return result;
    }

    /** Assign a handler to a queue.
     *
     *  Blocks until a queue is ready to have a handler assigned, or until
     *  all queues are closed.
     *
     *  @param assignment Used to return the new assignment.
     *
     *  @returns true if an assignment was made, false otherwise.
     */
    bool assign_handler(std::string & assignment) {
	ContextLocker lock(cond);
	std::map<std::string, QueueInfo>::iterator i = pick_queue();
	if (i == queues.end()) {
	    assignment.resize(0);
	    return false;
	}
	assignment = i->first;
	i->second.assigned = true;
	// Assigning a handler can't make there be work for any other thread
	// to do, so no need to signal the condition.
	return true;
    }

    /** Unassign a handler from a queue.
     */
    void unassign_handler(const std::string & assignment) {
	ContextLocker lock(cond);
	queues[assignment].assigned = false;
	cond.broadcast();
    }

    /** Pop from any active, non-assigned, queue.
     *
     *  If closed and all queues are empty, this will return NULL.  Otherwise,
     *  blocks if all queues are empty or disabled, then pops the oldest item
     *  from a queue, picking a different queue each time in round-robin order
     *  if there are multiple options, then returns a pointer to the item and
     *  sets @a result_key to contain the key of the queue returned from.
     */
    Task * pop_any(std::string & result_key) {
	ContextLocker lock(cond);
	std::map<std::string, QueueInfo>::iterator i = pick_queue();
	if (i == queues.end()) {
	    return NULL;
	}

	result_key = i->first;
	QueueInfo & queue = i->second;

	if (nudge_fd != -1 && queue.queue.size() == throttle_size) {
	    // Nudge when size is about to drop below throttle_size
	    (void) io_write_byte(nudge_fd, nudge_byte);
	}

	std::auto_ptr<Task> resultptr(queue.queue.front());
	queue.queue.pop();
	//printf("pop_any: queue %s now has %d items\n\n", result_key.c_str(), queue.queue.size());
	cond.broadcast();
	return resultptr.release();
    }

    /** Pop from a specific queue.
     *
     *  If the queue is closed and empty, this will return NULL.  Otherwise,
     *  blocks if the queue is empty or disabled, then pops the oldest item
     *  from the queue, and returns a pointer to it.
     */
    Task * pop_from(const std::string & key,
		    double end_time,
		    bool & is_finished) {
	ContextLocker lock(cond);
	std::map<std::string, QueueInfo>::iterator i = queues.find(key);
	is_finished = false;
	while (!closed) {
	    if (i != queues.end() && i->second.active && !i->second.queue.empty()) {
		break;
	    }
	    if (cond.timedwait(end_time)) {
		return NULL;
	    }
	    // Redo the find even if it succeeded before, because the queue may
	    // have been removed from the list during the wait.
	    i = queues.find(key);
	}
	if (i == queues.end() || i->second.queue.empty()) {
	    // Queue is empty - should only get here if it's also closed.
	    is_finished = true;
	    return NULL;
	}

	QueueInfo & queue = i->second;
	//printf("pop_from: queue %s has %d items\n", key.c_str(), queue.queue.size());

	if (nudge_fd != -1 && queue.queue.size() == throttle_size) {
	    // Nudge when size is about to drop below throttle_size
	    (void) io_write_byte(nudge_fd, nudge_byte);
	}

	std::auto_ptr<Task> resultptr(queue.queue.front());
	queue.queue.pop();
	cond.broadcast();
	return resultptr.release();
    }

    /** Fill a vector with the names of all queues which have space for pushing
     *  to them.
     */
    void get_queues_with_space(std::vector<std::string> & result) const {
	result.clear();
	ContextLocker lock(cond);
	for (std::map<std::string, QueueInfo>::const_iterator i = queues.begin();
	     i != queues.end(); ++i) {
	    if (i->second.queue.size() < throttle_size) {
		result.push_back(i->first);
	    }
	}
    }

    /** Wait until the group is closed and empty.
     */
    void wait_for_empty() const {
	ContextLocker lock(cond);
	while (!closed) {
	    cond.wait();
	}
	while (true) {
	    std::map<std::string, QueueInfo>::const_iterator i = queues.begin();
	    bool found = false;
	    while (i != queues.end()) {
		if (!i->second.queue.empty()) {
		    found = true;
		    break;
		}
		++i;
	    }
	    if (!found) return;
	    cond.wait();
	}
    }

    /** Get the current status of the task queue group.
     */
    void get_status(Json::Value & result) const {
	ContextLocker lock(cond);
	result = Json::objectValue;
	for (std::map<std::string, QueueInfo>::const_iterator
	     i = queues.begin(); i != queues.end(); ++i) {
	    Json::Value & queue_val(result[i->first] = Json::objectValue);
	    queue_val["size"] = uint64_t(i->second.queue.size());
	    queue_val["active"] = i->second.active;
	    queue_val["assigned"] = i->second.assigned;
	}
    }
};

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

    /** The directory that data is kept in.
     */
    const std::string datadir;

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
    CollectionPool collections;

    TaskManager(const TaskManager &);
    void operator=(const TaskManager &);
  public:
    TaskManager(const std::string & datadir_);
    ~TaskManager();

    /** Get the write end of the nudge pipe.
     *
     *  This is used by resulthandlers to nudge the server when results are
     *  ready.
     */
    int get_nudge_fd() const {
	return nudge_write_end;
    }

    /** Queue getting information about the server status.
     */
    Queue::QueueState queue_get_status(const RestPose::ResultHandle & resulthandle);

    /** Queue getting information about a collection.
     */
    Queue::QueueState queue_get_collinfo(const std::string & collection,
					 const RestPose::ResultHandle & resulthandle);

    /** Queue for searching a collection.
     */
    Queue::QueueState queue_search(const std::string & collection,
				   const RestPose::ResultHandle & resulthandle,
				   const Json::Value & search);

    /** Queue a document for indexing.
     *
     *  This is intended to be called from the output of processing.
     *  It blocks and retries if the queue is full, and disables the processing
     *  queue for the collection if the queue is starting to have low space.
     */
    void queue_index_processed_doc(const std::string & collection,
				   Xapian::Document xdoc,
				   const std::string & idterm);

    /** Queue setting the schema for a collection.
     */
    Queue::QueueState queue_set_schema(const std::string & collection,
				       const Json::Value & schema);

    Queue::QueueState queue_pipe_document(const std::string & collection,
					  const std::string & pipe,
					  const Json::Value & doc,
					  bool allow_throttle,
					  double end_time);

    Queue::QueueState queue_process_document(const std::string & collection,
					     const std::string & type,
					     const Json::Value & doc,
					     bool allow_throttle);

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

    /** Queue a commit to the index.
     * @param collection The name of the collection to index to.
     * @param allow_throttle If true, don't queue the document is the queue is
     * already busy.  Otherwise, queue the document unless the queue is
     * completely full.  See documentation for @a queue_index_document().
     */
    Queue::QueueState queue_commit(const std::string & collection,
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
