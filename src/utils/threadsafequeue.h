/** @file threadsafequeue.h
 * @brief Simple, threadsafe, non-persistent queue.
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
#ifndef RESTPOSE_INCLUDED_THREADSAFEQUEUE_H
#define RESTPOSE_INCLUDED_THREADSAFEQUEUE_H

#include <memory>
#include <queue>
#include <string>
#include "utils/io_wrappers.h"
#include "utils/threading.h"

namespace Queue {
    /** The state of a queue after pushing.
     */
    enum QueueState {
	/// Queue has space for more items before throttling.
	HAS_SPACE,

	/// Queue is nearly full (ie, has >= throttle_size items in it)
	LOW_SPACE,

	/// Queue is full.
	FULL,

	/// Queue is closed - no more items may be inserted.
	CLOSED
    };

    /** A threadsafe queue, with throttling.
     */
    template<class T> class ThreadsafeQueue {
	std::queue<T *> queue;
	Condition cond;
	size_t throttle_size;
	size_t max_size;
	bool closed;
	int nudge_fd;
	char nudge_byte;

      public:
	/** Create a new queue.
	 *
	 *  @param throttle_size_ The size at which to warn that the queue is
	 *  getting full.
	 *
	 *  @param max_size_ The size at which to prevent on adding to the queue.
	 */
	ThreadsafeQueue(size_t throttle_size_, size_t max_size_)
		: throttle_size(throttle_size_), max_size(max_size_),
		  closed(false), nudge_fd(-1), nudge_byte('Q')
	{
	}

	/** Set the nudge filedescriptor and byte.
	 *
	 *  @param nudge_fd_ A filedescriptor that a nudge byte will be written to
	 *  when the queue has been overloaded, and stops being overloaded.  Set to
	 *  -1 to disable nudges.
	 *
	 *  @param nudge_byte_ The nudge character to write to the file descriptor.
	 */
	void set_nudge(int nudge_fd_, char nudge_byte_)
	{
	    ContextLocker lock(cond);
	    nudge_fd = nudge_fd_;
	    nudge_byte = nudge_byte_;
	}

	/** Close the queue.
	 *
	 *  Prevents further items being added to the queue, and causes pop
	 *  operations on an empty queue to return immediately, rather than
	 *  blocking.
	 */
	void close()
	{
	    ContextLocker lock(cond);
	    closed = true;
	    cond.broadcast();
	}

	/** Push to the queue.
	 *
	 *  The queue takes ownership of the item pushed immediately, and
	 *  deletes it if pushing fails.
	 *
	 *  If the queue is full, will not push the item to the queue, and
	 *  returns a state of FULL.
	 *
	 *  If the queue is closed, will not push the item to the queue, and
	 *  returns a state of CLOSED.
	 *
	 *  If any other value is returned, the item has been pushed onto the
	 *  queue.
	 *
	 *  @param item The item to push onto the queue.
	 *
	 *  @param allow_throttle If true, don't push the item if the queue has
	 *  throttle_size or more items in it already (and return FULL).  If
	 *  false, allow items to be pushed if the queue has max_size or more
	 *  items.
	 *
	 *  @returns FULL or CLOSED if the queue is full or closed respectively,
	 *  as described. In either of these cases, the item has not been
	 *  pushed onto the queue - otherwise the item has been pushed onto the
	 *  queue.  Returns HAS_SPACE if the queue has plenty of space
	 *  available, or LOW_SPACE if the queue is running low on space (this
	 *  also implies that the item would not have been pushed if
	 *  allow_throttle were true).
	 */
	QueueState push(T * item, bool allow_throttle) {
	    std::auto_ptr<T> itemptr(item);
	    ContextLocker lock(cond);
	    if (closed) {
		return CLOSED;
	    }
	    if (allow_throttle) {
		if (queue.size() >= throttle_size) {
		    return FULL;
		}
	    } else {
		if (queue.size() >= max_size) {
		    return FULL;
		}
	    }
	    queue.push(NULL);
	    queue.back() = itemptr.release();
	    QueueState result;
	    size_t size = queue.size();
	    if (size < throttle_size) {
		result = HAS_SPACE;
	    } else {
		result = LOW_SPACE;
	    }
	    cond.signal();
	    return result;
	}

	/** Pop from the queue.
	 *
	 *  If the queue is closed and empty, this will return false.
	 *  Otherwise, blocks if the queue is empty, then pops the oldest item
	 *  from the queue, returns true and sets @a result to contain the
	 *  item.
	 */
	bool pop(T * & result) {
	    ContextLocker lock(cond);
	    while (!closed && queue.empty()) {
		cond.wait();
	    }
	    if (queue.empty()) {
		return false;
	    }

	    if (nudge_fd != -1 && queue.size() == throttle_size) {
		// Nudge when size is about to drop below throttle_size
		(void) io_write_byte(nudge_fd, nudge_byte);
	    }

	    std::auto_ptr<T> resultptr(queue.front());
	    queue.pop();
	    cond.signal();
	    result = resultptr.release();
	    return true;
	}
    };
};

#endif /* RESTPOSE_INCLUDED_THREADSAFEQUEUE_H */
