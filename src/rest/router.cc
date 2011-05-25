/** @file router.cc
 * @brief Router for REST requests
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
#include "router.h"
#include "httpserver/httpserver.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

using namespace std;
using namespace RestPose;

// Virtual destructor to ensure there's a vtable.
Handler::~Handler() {}

void
CollCreateHandler::handle(ConnectionInfo & conn)
{
    if (conn.first_call) {
	return;
    }

    Json::Value result(Json::objectValue);
    conn.respond(MHD_HTTP_OK, json_serialise(result), "application/json");
}

QueuedHandler::QueuedHandler(TaskManager * taskman_)
	: taskman(taskman_),
	  resulthandle(taskman_->get_nudge_fd(), 'H'),
	  queued(false)
{}

/** Handle queue push status responses which correspond to the push having
 *  failed.
 */
static void
handle_queue_push_fail(Queue::QueueState state, ConnectionInfo & conn)
{
    switch (state) {
	case Queue::CLOSED:
	    conn.respond(MHD_HTTP_INTERNAL_SERVER_ERROR, "{\"err\":\"Server is shutting down\"}", "application/json");
	    break;
	case Queue::FULL:
	    conn.respond(MHD_HTTP_SERVICE_UNAVAILABLE, "{\"err\":\"Too many active requests\"}", "application/json");
	    break;
	default:
	    // Do nothing
	    break;
    }
}

void
QueuedHandler::handle(ConnectionInfo & conn)
{
    if (conn.first_call) {
	return;
    }
    if (!queued) {
	// FIXME - handle partially uploaded data
	// FIXME - handle failure to parse data
	Json::Value body(Json::nullValue);
	if (*(conn.upload_data_size) != 0) {
	    json_unserialise(string(conn.upload_data, *(conn.upload_data_size)),
			     body);
	    *(conn.upload_data_size) = 0;
	}
	Queue::QueueState state = enqueue(body);
	handle_queue_push_fail(state, conn);
	queued = true;
    } else {
	const Json::Value * result = resulthandle.get_result();
	if (result == NULL) {
	    return;
	}
	conn.respond(resulthandle.get_status(), json_serialise(*result), "application/json");
    }
}

Queue::QueueState
ServerStatusHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_get_status(resulthandle);
}

Queue::QueueState
CollInfoHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_get_collinfo(collection, resulthandle);
}

Queue::QueueState
SearchHandler::enqueue(const Json::Value & body) const
{
    return taskman->queue_search(collection, resulthandle, body);
}

void
NotFoundHandler::handle(ConnectionInfo & conn)
{
    printf("NotFoundHandler()%s\n", conn.first_call ? " first call" : "");
    if (conn.first_call) {
	return;
    }
    conn.respond(MHD_HTTP_NOT_FOUND, "Resource not found", "text/plain");
}

Router::Router(TaskManager * taskman_, Server * server_)
	: taskman(taskman_),
	  server(server_)
{
}

Handler *
Router::route(ConnectionInfo & conn) const
{
    conn.parse_url_components();
    if (conn.components.size() == 0) {
	return new NotFoundHandler;
    }
    if (conn.components[0] == "status") {
	if (conn.components.size() == 1) {
	    if (!conn.require_method(HTTP_GETHEAD)) {
		return NULL;
	    }
	    if (conn.method & HTTP_GETHEAD) {
		return new ServerStatusHandler(taskman);
	    }
	}
    }
    if (conn.components[0] == "coll") {
	if (conn.components.size() == 2) {
	    if (!conn.require_method(HTTP_GETHEAD | HTTP_POST)) {
		return NULL;
	    }
	    if (conn.method & HTTP_GETHEAD) {
		return new CollInfoHandler(taskman, conn.components[1]);
	    } else if (conn.method & HTTP_POST) {
		return new CollCreateHandler(taskman, server);
	    }
	}
	if (conn.components.size() == 3) {
	    if (conn.components[2] == "search") {
		if (!conn.require_method(HTTP_GETHEAD | HTTP_POST)) {
		    // Allow POST to perform searches, since some clients don't
		    // allow bodies to be submitted for GET requests.
		    return NULL;
		}
		return new SearchHandler(taskman, conn.components[1]);
	    }
	}
    }

    return new NotFoundHandler;
}
