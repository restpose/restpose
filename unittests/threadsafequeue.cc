/** @file threadsafequeue.cc
 * @brief Tests for ThreadsafeQueue
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
#include "utils/threadsafequeue.h"

#include "UnitTest++.h"
#include <string>
#include <vector>
#include "str.h"

using namespace RestPose;
using namespace Queue;

class ReaderThread : public Thread {
    ThreadsafeQueue<int> & queue;
    std::vector<int> results;

  public:
    ReaderThread(ThreadsafeQueue<int> & queue_)
	    : queue(queue_)
    {}

    std::string result() const
    {
	std::string r;
	for (std::vector<int>::const_iterator i = results.begin();
	     i != results.end(); ++i) {
	    r += str(*i);
	    r += ",";
	}
	if (!r.empty()) {
	    r = r.substr(0, r.size() - 1);
	}
	return r;
    }

    void run() {
	int * r;
	while (queue.pop(r)) {
	    results.push_back(*r);
	    delete r;
	    ContextLocker lock(cond);
	    if (stop_requested) {
		return;
	    }
	}
    }
};

/** Test making a queue, and immediately closing it. */
TEST(ThreadsafeQueueEmpty)
{
    ThreadsafeQueue<int> queue(100, 200);
    ReaderThread reader(queue);
    reader.start();
    queue.close();
    reader.join();
    CHECK_EQUAL("", reader.result());
}

/** Test making a queue, adding a single item, and reading it out. */
TEST(ThreadsafeQueueTrivial)
{
    ThreadsafeQueue<int> queue(100, 200);
    CHECK_EQUAL(HAS_SPACE, queue.push(new int(1), false));
    ReaderThread reader(queue);
    reader.start();
    queue.close();
    reader.join();
    CHECK_EQUAL("1", reader.result());
}

/** Test making a queue, adding two items, and reading them out. */
TEST(ThreadsafeQueueTwoItems)
{
    ThreadsafeQueue<int> queue(100, 200);
    CHECK_EQUAL(HAS_SPACE, queue.push(new int(1), false));
    CHECK_EQUAL(HAS_SPACE, queue.push(new int(2), false));

    ReaderThread reader(queue);
    reader.start();
    queue.close();
    reader.join();
    CHECK_EQUAL("1,2", reader.result());
}

/** Test making a queue, setting a reader to read from it, but closing the
 *  queue before the reader starts. */
TEST(ThreadsafeQueueClosed)
{
    ThreadsafeQueue<int> queue(100, 200);
    CHECK_EQUAL(HAS_SPACE, queue.push(new int(1), false));
    CHECK_EQUAL(HAS_SPACE, queue.push(new int(2), false));
    queue.close();
    CHECK_EQUAL(CLOSED, queue.push(new int(3), false));

    ReaderThread reader(queue);
    reader.start();
    reader.join();
    CHECK_EQUAL("1,2", reader.result());
}

TEST(ThreadsafeQueueFill)
{
    ThreadsafeQueue<int> queue(10, 20);
    ReaderThread reader(queue);
    for (int i = 1; i <= 9; ++i) {
	// First 9 items report HAS_SPACE
	CHECK_EQUAL(HAS_SPACE, queue.push(new int(i), false));
    }
    // The next item brings the queue up to the point at which further pushes
    // would be throttled (but this actual push isn't throttled).
    CHECK_EQUAL(LOW_SPACE, queue.push(new int(10), true));
    for (int i = 11; i <= 20; ++i) {
	// Next 10 items report LOW_SPACE, or fail to push if allowed to be
	// throttled.
	CHECK_EQUAL(FULL, queue.push(new int(i), true));
	CHECK_EQUAL(LOW_SPACE, queue.push(new int(i), false));
    }
    CHECK_EQUAL(FULL, queue.push(new int(21), false));
    CHECK_EQUAL(FULL, queue.push(new int(21), true));
    reader.start();

    queue.close();
    reader.join();
    CHECK_EQUAL("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20", reader.result());
}
