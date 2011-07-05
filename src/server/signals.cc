/** @file signals.cc
 * @brief Signal handling code
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
#include "signals.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ignore_sigpipe.h"
#include "server.h"
#include "utils/rsperrors.h"

/// Global variable pointing to the server shutdown pipe - used for cleanup.
Server * g_server;

/// Global variable holding the server PID - used to only cleanup in right pid.
pid_t g_server_pid;

// Forward declarations.
extern "C" void handle_sig(int signum);
extern "C" void emergency_exit_handler(int signum);
static void set_up_emergency_signal_handlers();

/** Send a signal to all processes in the process group.
 *
 *  @param signum The signal to send.
 */
static void
signal_process_group(int signum)
{
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    (void) sigaction(signum, &act, NULL);
    kill(0, signum);
}

// A handler for cleaning up just before an emergency exit.
extern "C" void
emergency_exit_handler(int signum)
{
    if (g_server_pid == getpid()) {
	g_server->emergency_shutdown();
    }
    signal_process_group(signum);
    release_signal_handlers();
    exit(EXIT_SUCCESS);
}

// Generic signal handler.
extern "C" void
handle_sig(int signum)
{
    switch (signum) {
	case SIGINT:
	    {
		if (g_server_pid == getpid()) {
		    g_server->shutdown();
		}
		set_up_emergency_signal_handlers();
		break;
	    }
	case SIGCHLD:
	    {
		// Ensure that all children which have exited are waited for.
		while (true) {
		    // Use a non-blocking waitpid in case a child was cleaned
		    // up elsewhere.
		    int ret = waitpid(-1, NULL, WNOHANG);
		    if (ret <= 0) break;
		}
		break;
	    }
	default:
	    {
		// Drop the signal handlers if we get a signal we don't understand.
		release_signal_handlers();
		signal_process_group(signum);
	    }
    }
}

void
set_up_signal_handlers(Server * internal)
{
    struct sigaction act;
    g_server = internal;
    g_server_pid = getpid();
    act.sa_handler = handle_sig;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGCHLD);
    if (sigaction(SIGINT, &act, NULL) == -1) {
	throw RestPose::SysError("Unable to set SIGINT handler", errno);
    }
    if (sigaction(SIGCHLD, &act, NULL) == -1) {
	throw RestPose::SysError("Unable to set SIGCHLD handler", errno);
    }

    act.sa_handler = emergency_exit_handler;
    if (sigaction(SIGTERM, &act, NULL) == -1) {
	throw RestPose::SysError("Unable to set SIGTERM handler", errno);
    }

    ignore_sigpipe();
}

/** Signal handler set up after a SIGINT signal is received, so that further
 *  SIGINT signals will abort the normal shutdown process and make the
 *  process exit immediately.
 */
static void
set_up_emergency_signal_handlers()
{
    struct sigaction act;
    act.sa_handler = emergency_exit_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGINT, &act, NULL) != 0) {
	release_signal_handlers();
	return;
    }
}

void
release_signal_handlers()
{
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    (void) sigaction(SIGTERM, &act, NULL);
    (void) sigaction(SIGINT, &act, NULL);
    (void) sigaction(SIGCHLD, &act, NULL);
}
