/** @file threading.h
 * @brief Convenient wrapper for holding thread locks.
 */
/* Copyright 2010 Richard Boulton
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
#ifndef RESTPOSE_INCLUDED_THREADING_H
#define RESTPOSE_INCLUDED_THREADING_H

#include <cmath>
#include <pthread.h>
#include "rsperrors.h"
#include "safeerrno.h"

// Forward declaration, within this file.
class Condition;

/** A simple wrapper around a mutex.
 */
class Mutex {
  protected:
    pthread_mutex_t mutex;
  public:
    Mutex() {
	pthread_mutex_init(&mutex, NULL);
    }
    ~Mutex() {
	pthread_mutex_destroy(&mutex);
    }
    void lock() {
	if (pthread_mutex_lock(&mutex) != 0) {
	    throw RestPose::ThreadError("Can't lock mutex");
	}
    }
    void unlock() {
	if (pthread_mutex_unlock(&mutex) != 0) {
	    throw RestPose::ThreadError("Can't unlock mutex");
	}
    }
};

/** Hold a lock while this object is in context.
 */
class ContextLocker {
    Mutex & mutex;
    bool locked;
  public:
    ContextLocker(Mutex & mutex_) : mutex(mutex_), locked(false) {
	lock();
    }
    ~ContextLocker() {
	unlock();
    }
    void lock() {
	if (locked)
	    return;
	mutex.lock();
	locked = true;
    }
    void unlock() {
	if (!locked)
	    return;
	mutex.unlock();
	locked = false;
    }
};

/** A condition, with an internal mutex.
 */
class Condition : public Mutex {
    pthread_cond_t cond;
  public:
    Condition() : Mutex() {
	pthread_cond_init(&cond, NULL);
    }

    ~Condition() {
	pthread_cond_destroy(&cond);
    }

    /** Wait on the condition.
     *
     *  The mutex must be locked by the thread calling this.
     */
    void wait() {
	(void) pthread_cond_wait(&cond, &mutex);
    }

    /** Wait on the condition, until a time limit expires.
     *
     *  Returns true if the wait timed out.
     */
    bool timedwait(double end_time) {
	struct timespec tv;
	while (true) {
	    tv.tv_sec = long(end_time);
	    tv.tv_nsec = long(std::fmod(end_time, 1.0) * 1e9);
	    int ret = pthread_cond_timedwait(&cond, &mutex, &tv);
	    if (ret == ETIMEDOUT) return true;
	    if (ret != EINTR) return false;
	}
    }

    /** Signal the condition to one waiting process.
     *
     *  The mutex must be locked by the thread calling this.
     */
    void signal() {
	(void) pthread_cond_signal(&cond);
    }

    /** Signal the condition to all waiting processes.
     *
     *  The mutex must be locked by the thread calling this.
     */
    void broadcast() {
	(void) pthread_cond_broadcast(&cond);
    }
};

class Thread {
    pthread_t thread;
    bool started;
    bool joined;

    /// No copying of Thread objects.
    Thread(const Thread &);
    /// No assignment to Thread objects.
    void operator=(const Thread &);
  protected:
    /** A flag, set to true when stop is requested.
     */
    bool stop_requested;

  public:
    Condition cond;

    Thread()
	    : started(false),
	      joined(false),
	      stop_requested(false)
    {}

    virtual ~Thread() {
	stop();
	join();
    }

    /** Start the thread running.
     *
     *  Returns true if the thread started correctly.  Returns false if the
     *  thread failed to start.
     */
    bool start();

    /** Stop the thread running.
     */
    void stop()
    {
	ContextLocker lock(cond);
	if (!started) return;
	if (!stop_requested) {
	    stop_requested = true;
	    cond.broadcast();
	}
    }

    /** Join the thread.
     */
    void join()
    {
	{
	    ContextLocker lock(cond);
	    if (!started || joined)
		return;
	    // Set joined before actually joining, since we're guaranteed to
	    // get to the join call from here, and we want to ensure that after
	    // dropping the lock, noone else will try and join.
	    joined = true;
	}

	if (pthread_join(thread, NULL) != 0) {
	    throw RestPose::ThreadError("Can't join thread");
	}
    }

    /** Method run in thread.
     *
     *  This method should frequently check the stop_requested flag, while
     *  holding the cond mutex, and exit if it is set to true.
     */
    virtual void run() = 0;

    /** Cleanup after thread finishes.
     *
     *  Guaranteed to be called even if the thread dies with an exception.
     *
     *  Note: won't be called if the thread start() method returned false.
     */
    virtual void cleanup() {}
};

#endif /* RESTPOSE_INCLUDED_THREADING_H */
