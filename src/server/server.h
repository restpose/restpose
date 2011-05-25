/** @file server.h
 * @brief Central server for RestPose.
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
#ifndef RESTPOSE_INCLUDED_SERVER_H
#define RESTPOSE_INCLUDED_SERVER_H

#include <sys/select.h>
#include <string>
#include <map>
#include "json/value.h"
#include "utils/threading.h"

// Forward declaration within this file.
class Server;

/** A server to be added to the central server's mainloop.
 */
class SubServer {
    public:
	virtual ~SubServer();

	/** Start the sub server.
	 */
	virtual void start() = 0;

	/** Stop the sub server.
	 *
	 *  This is called to stop the server running; it should stop any
	 *  subthreads or processes, but should not block.
	 */
	virtual void stop() = 0;

	/** Join the sub server.
	 *
	 *  This should block until all subthreads or processes have been
	 *  joined, and should then clean up any remaining resources owned by
	 *  the SubServer.
	 */
	virtual void join() = 0;

	/** Set the fdsets for any fds we're interested in the main server
	 *  selecting on.
	 */
	virtual void get_fdsets(fd_set * read_fd_set,
				fd_set * write_fd_set,
				fd_set * except_fd_set,
				int * max_fd,
				bool * have_timeout,
				uint64_t * timeout) = 0;

	/** Called for each server every time select returns.
	 *
	 *  The server should check if any of its file descriptors are now
	 *  ready to be used, and perform appropriate actions.
	 */
	virtual void serve(fd_set * read_fd_set,
			   fd_set * write_fd_set,
			   fd_set * except_fd_set,
			   bool timed_out) = 0;
};

class BackgroundTask {
    public:
	virtual ~BackgroundTask();

	/** Start the task.
	 */
	virtual void start(Server * server) = 0;

	/** Stop the task.
	 *
	 *  This is called to stop the task running; it should stop any
	 *  subthreads or processes, but should not block.
	 */
	virtual void stop() = 0;

	/** Join the task.
	 *
	 *  This should block until all subthreads or processes have been
	 *  joined, and should then clean up any remaining resources owned by
	 *  the task.
	 */
	virtual void join() = 0;
};

/** The central server.
 *
 *  This runs a mainloop, and manages a pool of worker processes performing
 *  actions.
 *
 *  Subservers can be added, which will be polled in the mainloop, to provide
 *  access via various protocols.
 */
class Server {
	/// Flag, set to true when the server has started.
	bool started;

	/// Flag, set to true when a shutdown request has been made.
	bool shutting_down;

	/** The write end of a pipe, written to when a request to wake up the
	 *  main thread is made.
	 */
	int nudge_write_end;

	/** The read end of a pipe, written to when a request to wake up the
	 *  main thread is made.
	 */
	int nudge_read_end;

	/** The sub servers added to this server.
	 */
	std::map<std::string, SubServer *> servers;

	/** Background tasks.
	 */
	std::map<std::string, BackgroundTask *> bgtasks;

	/** Run the server mainloop.
	 */
	void mainloop();

    public:
	/** Create a new server.
	 */
	Server();

	/** Destroy the server.
	 *
	 *  Stops the server if it's still running.
	 */
	~Server();

	/** Run the server.
	 *
	 *  This will block until the server is shutdown (by a signal, or by a
	 *  subserver performing a shutdown action).
	 */
	void run();

	/** Start the shutdown of the server.
	 *
	 *  This returns immediately, but the server may take some time to shut
	 *  down fully.
	 *
	 *  It is safe to call this from any thread, or from a signal handler.
	 *  It is safe to call it repeatedly - only the first call will have
	 *  any effect.
	 *
	 *  This must not be called until the server has been started (by
	 *  calling run()).
	 */
	void shutdown();

	/** Perform an emergency shutdown of the server.
	 *
	 *  This is intended to clean up the server just before a forced quit.
	 *
	 *  This should simply unlink any temporary files currently opened by
	 *  the server.  Any errors when unlinking may be ignored.
	 *
	 *  It is safe to call this from a signal handler.
	 */
	void emergency_shutdown();

	/** Stop any sub servers and tasks owned by this server.
	 */
	void stop_children(bool ignore_errors = false);

	/** Join any sub servers and tasks owned by this server.
	 */
	void join_children(bool ignore_errors = false);

	/** Add a sub server.
	 *
	 *  The Server takes ownership of the sub server, and will delete it
	 *  on shutdown.
	 *
	 *  This may only be called from the main thread.
	 */
	void add(const std::string & name, SubServer * server);

	/** Add a background task.
	 *
	 *  The Server takes ownership of the task, and will delete it on
	 *  shutdown.
	 *
	 *  This may only be called from the main thread.
	 */
	void add_bg_task(const std::string & name, BackgroundTask * task);

	/** Get the list of servers.
	 *
	 *  This may only be called from the main thread.
	 */
	const std::map<std::string, SubServer *> & get_servers() const;

	/** Get the list of background tasks.
	 *
	 *  This may only be called from the main thread.
	 */
	const std::map<std::string, BackgroundTask *> & get_bg_tasks() const;
};

#endif /* RESTPOSE_INCLUDED_SERVER_H */
