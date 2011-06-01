/** @file task_queue_group.h
 * @brief A group of task queues.
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
#ifndef RESTPOSE_INCLUDED_TASK_QUEUE_GROUP_H
#define RESTPOSE_INCLUDED_TASK_QUEUE_GROUP_H

#include <map>
#include <memory>
#include "omassert.h"
#include <queue>
#include "server/server.h"
#include "server/tasks.h"
#include <set>
#include <string>
#include "utils/queueing.h"
#include "utils/io_wrappers.h"
#include <vector>
#include <xapian.h>

class TaskManager;

// FIXME - no good reason why most of this class is implemented in the header file.

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

	/** Tasks in progress.
	 *
	 *  Note - these aren't owned by the Queue.
	 */
	std::set<Task *> in_progress;

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

    /** Check if a queue is empty and should be cleaned up.
     */
    void check_for_cleanup(const std::map<std::string, QueueInfo>::iterator & i)
    {
	QueueInfo & queue(i->second);
	if (queue.queue.size() == 0 &&
	    queue.in_progress.size() == 0 &&
	    queue.active &&
	    !queue.assigned) {
	    // Boring queue - equivalent to not existing.
	    queues.erase(i);
	}
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
	if (on) {
	    check_for_cleanup(queues.find(key));
	}
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
		// printf("CLOSED %s\n", key.c_str());
		return Queue::CLOSED;
	    }

	    if ((allow_throttle && (queue.queue.size() >= throttle_size)) ||
		(!allow_throttle && (queue.queue.size() >= max_size))) {
		if (end_time == 0.0) {
		    // printf("FULL %s\n", key.c_str());
		    return Queue::FULL;
		} else {
		    if (cond.timedwait(end_time)) {
			// printf("FULL + TIMEOUT %s\n", key.c_str());
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
	// printf("PUSHED: %s %d items%s\n", key.c_str(), size, result == Queue::LOW_SPACE ? " (low space)" : "");
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
	check_for_cleanup(queues.find(assignment));
	cond.broadcast();
    }

    /** Register that a task has been completed.
     *
     *  Normally, such a registration should be done by passing details of the
     *  completed task to the next pop() command, but in the case where a
     *  thread is exiting, there is no such pop, so this method can be used
     *  instead.
     */
    void completed(const std::string & key, Task * task) {
	if (task != NULL) {
	    ContextLocker lock(cond);
	    std::map<std::string, QueueInfo>::iterator
		    i = queues.find(key);
	    if (i != queues.end()) {
		(void) i->second.in_progress.erase(task);
	    }
	}
    }

    /** Pop from any active, non-assigned, queue.
     *
     *  If closed and all queues are empty, this will return NULL.  Otherwise,
     *  blocks if all queues are empty or disabled, then pops the oldest item
     *  from a queue, picking a different queue each time in round-robin order
     *  if there are multiple options, then returns a pointer to the item and
     *  sets @a key to contain the key of the queue returned from.
     *
     *  @param key Should be initialised to the key of the task which has just
     *  been completed (if completed_task is not NULL).  Will be set to the key
     *  of the task which is returned (if returned task is not NULL).
     *  @param completed_task A task which has been completed, or NULL.
     */
    Task * pop_any(std::string & key,
		   Task * completed_task) {
	ContextLocker lock(cond);
	std::map<std::string, QueueInfo>::iterator i;
	if (completed_task != NULL) {
	    //printf("pop_any: completed: %s:%p\n", key.c_str(), completed_task);
	    i = queues.find(key);
	    Assert(i != queues.end());
	    (void) i->second.in_progress.erase(completed_task);
	    check_for_cleanup(i);
	}
	i = pick_queue();
	if (i == queues.end()) {
	    return NULL;
	}

	key = i->first;
	QueueInfo & queue = i->second;

	if (nudge_fd != -1 && queue.queue.size() == throttle_size) {
	    // Nudge when size is about to drop below throttle_size
	    (void) io_write_byte(nudge_fd, nudge_byte);
	}

	std::auto_ptr<Task> resultptr(queue.queue.front());
	queue.queue.pop();
	queue.in_progress.insert(resultptr.get());
	//printf("pop_any: queue %s now has %d items\n\n", key.c_str(), queue.queue.size());
	//printf("pop_any: %s:%p\n", key.c_str(), resultptr.get());
	check_for_cleanup(i);
	cond.broadcast();
	return resultptr.release();
    }

    /** Pop from a specific queue.
     *
     *  If the queue is closed and empty, this will return NULL.  Otherwise,
     *  blocks if the queue is empty or disabled, then pops the oldest item
     *  from the queue, and returns a pointer to it.
     *
     *  @param completed_task A task which has been completed.
     */
    Task * pop_from(const std::string & key,
		    double end_time,
		    bool & is_finished,
		    Task * completed_task,
		    const std::string & completed_key) {
	ContextLocker lock(cond);
	std::map<std::string, QueueInfo>::iterator i;
	if (completed_task != NULL) {
	    i = queues.find(completed_key);
	    Assert(i != queues.end());
	    (void) i->second.in_progress.erase(completed_task);
	    check_for_cleanup(i);
	}
	i = queues.find(key);
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
	queue.in_progress.insert(resultptr.get());
	//printf("pop_from: queue %s now has %d items\n\n", key.c_str(), queue.queue.size());
	check_for_cleanup(i);
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
	    queue_val["size"] = Json::UInt64(i->second.queue.size());
	    queue_val["active"] = i->second.active;
	    queue_val["assigned"] = i->second.assigned;
	    queue_val["in_progress"] = i->second.in_progress.size();
	}
    }
};

#endif /* RESTPOSE_INCLUDED_TASK_QUEUE_GROUP_H */
