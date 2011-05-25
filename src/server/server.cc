/** @file server.cc
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

#include <config.h>
#include "server.h"

#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <memory>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "utils/io_wrappers.h"
#include "utils/rsperrors.h"
#include "signals.h"

using namespace std;

SubServer::~SubServer()
{
}

BackgroundTask::~BackgroundTask()
{
}

Server::Server()
	: started(false),
	  shutting_down(false),
	  nudge_write_end(-1),
	  nudge_read_end(-1)
{
}

Server::~Server()
{
    for (map<string, SubServer *>::iterator i = servers.begin();
	 i != servers.end(); ++i) {
	delete i->second;
    }
    servers.clear();

    for (map<string, BackgroundTask *>::iterator i = bgtasks.begin();
	 i != bgtasks.end(); ++i) {
	delete i->second;
    }
    bgtasks.clear();
}

void
Server::run()
{
    if (started || shutting_down) {
	// Exit immediately, without an error.
	return;
    }
    started = true;

    // Create the socket used for signalling a shutdown request.
    {
	int fds[2];
	int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
	if (ret == -1) {
	    throw RestPose::SysError("Couldn't create internal socketpair",
				     errno);
	}
	nudge_write_end = fds[0];
	nudge_read_end = fds[1];
    }

    set_up_signal_handlers(this);
    try {
	// Start the servers.
	for (map<string, SubServer *>::const_iterator i = servers.begin();
	     i != servers.end(); ++i) {
	    if (i->second) {
		i->second->start();
	    }
	}

	// Start the background tasks.
	for (map<string, BackgroundTask *>::const_iterator i = bgtasks.begin();
	     i != bgtasks.end(); ++i) {
	    if (i->second) {
		i->second->start(this);
	    }
	}

	mainloop();

	stop_children();
	join_children();

	release_signal_handlers();
	if (!io_close(nudge_write_end)) {
	    throw RestPose::SysError("Couldn't close internal socketpair",
				     errno);
	}
	if (!io_close(nudge_read_end)) {
	    throw RestPose::SysError("Couldn't close internal socketpair",
				     errno);
	}

    } catch(...) {
	stop_children(true);
	join_children(true);

	release_signal_handlers();
	(void)io_close(nudge_write_end);
	(void)io_close(nudge_read_end);
	throw;
    }
}

void
Server::shutdown()
{
    (void) io_write(nudge_write_end, "S");
}

void
Server::emergency_shutdown()
{
    // Clean up any temporary files.  The process will exit shortly after this
    // method returns.
}

void
Server::stop_children(bool ignore_errors)
{
    // Stop the background tasks.
    for (map<string, BackgroundTask *>::iterator i = bgtasks.begin();
	 i != bgtasks.end(); ++i) {
	if (i->second != NULL) {
	    if (ignore_errors) {
		try {
		    i->second->stop();
		} catch(...) {
		}
	    } else {
		i->second->stop();
	    }
	}
    }

    // Stop the servers.
    for (map<string, SubServer *>::iterator i = servers.begin();
	 i != servers.end(); ++i) {
	if (i->second != NULL) {
	    if (ignore_errors) {
		try {
		    i->second->stop();
		} catch(...) {
		}
	    } else {
		i->second->stop();
	    }
	}
    }
}

void
Server::join_children(bool ignore_errors)
{
    // Join the background tasks.
    for (map<string, BackgroundTask *>::iterator i = bgtasks.begin();
	 i != bgtasks.end(); ++i) {
	if (i->second) {
	    if (ignore_errors) {
		try {
		    i->second->join();
		} catch(...) {
		}
	    } else {
		i->second->join();
	    }
	}
    }

    // Join the servers.
    for (map<string, SubServer *>::iterator i = servers.begin();
	 i != servers.end(); ++i) {
	if (i->second) {
	    if (ignore_errors) {
		try {
		    i->second->join();
		} catch(...) {
		}
	    } else {
		i->second->join();
	    }
	}
    }
}

void
Server::add(const string & name, SubServer * server)
{
    auto_ptr<SubServer> ptr(server);
    servers[name] = NULL;
    servers[name] = ptr.release();
}

void
Server::add_bg_task(const string & name, BackgroundTask * task)
{
    auto_ptr<BackgroundTask> ptr(task);
    bgtasks[name] = NULL;
    bgtasks[name] = ptr.release();
}

const std::map<std::string, SubServer *> &
Server::get_servers() const
{
    return servers;
}

const std::map<std::string, BackgroundTask *> &
Server::get_bg_tasks() const
{
    return bgtasks;
}

void
Server::mainloop()
{
    while (!shutting_down) {
	int maxfd = 0;
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	bool have_timeout = false;
	uint64_t timeout = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

	FD_SET(nudge_read_end, &rfds);
	if (nudge_read_end > maxfd)
	    maxfd = nudge_read_end;

	for (map<string, SubServer *>::const_iterator i = servers.begin();
	     i != servers.end(); ++i) {
	    if (i->second) {
		i->second->get_fdsets(&rfds, &wfds, &efds, &maxfd,
				 &have_timeout, &timeout);
	    }
	}

	struct timeval tv;
	struct timeval * tv_ptr(NULL);
	if (have_timeout) {
	    tv.tv_sec = timeout / 1000;
	    tv.tv_usec = (timeout - (tv.tv_sec * 1000)) * 1000;
	    tv_ptr = &tv;
	}

	// Wait for one of the filedescriptors to be ready.
	int ret = select(maxfd + 1, &rfds, &wfds, &efds, tv_ptr);
	if (ret == -1) {
	    if (errno == EINTR) continue;
	    throw RestPose::SysError("Select failed", errno);
	}
	bool timed_out = (ret == 0);

        // Check the nudge pipe
        if (!timed_out && FD_ISSET(nudge_read_end, &rfds)) {
            string result;
            if (!io_read_append(result, nudge_read_end)) {
                throw RestPose::SysError("Couldn't read from internal socket",
					 errno);
            }
            if (result.find('S') != result.npos) {
                // Shutdown
		shutting_down = true;

		// FIXME - tell subservers to stop accepting incoming
		// connections, but allow them to finish responding to
		// requests in progress.

                return;
            }
        }

	// Call subservers.
	for (map<string, SubServer *>::const_iterator i = servers.begin();
	     i != servers.end(); ++i) {
	    if (i->second) {
		i->second->serve(&rfds, &wfds, &efds, timed_out);
	    }
	}
    }
}
