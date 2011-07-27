/** @file handler.cc
 * @brief Base classes of handlers used in routing.
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
#include "rest/handler.h"

#include "httpserver/httpserver.h"
#include "logger/logger.h"
#include <microhttpd.h>
#include "server/task_manager.h"
#include "str.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

// Virtual destructor to ensure there's a vtable.
Handler::~Handler() {}

QueuedHandler::QueuedHandler()
	: Handler(),
	  queued(false)
{
}

/** Handle queue push status responses which correspond to the push having
 *  failed.
 */
bool
QueuedHandler::handle_queue_push_fail(Queue::QueueState state,
				      ConnectionInfo & conn)
{
    switch (state) {
	case Queue::CLOSED:
	    conn.respond(MHD_HTTP_INTERNAL_SERVER_ERROR, "{\"err\":\"Server is shutting down\"}", "application/json");
	    return true;
	case Queue::FULL:
	    conn.respond(MHD_HTTP_SERVICE_UNAVAILABLE, "{\"err\":\"Too many active requests\"}", "application/json");
	    return true;
	default:
	    // Do nothing
	    break;
    }
    return false;
}

void
QueuedHandler::handle(ConnectionInfo & conn)
{
    LOG_DEBUG("QueuedHandler: firstcall=" + str(conn.first_call) +
	      ", queued=" + str(queued) +
	      ", data=\"" + (conn.upload_data ? conn.upload_data : "NULL") +
	      "\", size=" + (conn.upload_data_size ? str(*(conn.upload_data_size)) : string("NULL")));
    if (conn.first_call) {
	resulthandle.set_nudge(taskman->get_nudge_fd(), 'H');
	return;
    }
    if (!queued) {
	if (*(conn.upload_data_size) != 0) {
	    // FIXME - enforce an upload size limit
	    uploaded_data += string(conn.upload_data, *(conn.upload_data_size));
	    *(conn.upload_data_size) = 0;
	    return;
	}

	// FIXME - check Content-Type
	Json::Value body(Json::nullValue);
	if (uploaded_data.size() != 0) {
	    // Handle failure to parse data
	    try {
		json_unserialise(uploaded_data, body);
	    } catch(InvalidValueError & e) {
		LOG_ERROR(string("Invalid JSON supplied in request body: ") + e.what());
		resulthandle.failed(e.what(), 400);
		conn.respond(resulthandle);
		return;
	    }
	}
	Queue::QueueState state = enqueue(conn, body);
	if (handle_queue_push_fail(state, conn)) {
	    return;
	}
	queued = true;
    } else {
	if (resulthandle.is_ready()) {
	    conn.respond(resulthandle);
	}
    }
}

NoWaitQueuedHandler::NoWaitQueuedHandler()
	: Handler()
{
}

/** Handle queue push status responses which correspond to the push having
 *  failed.
 */
bool
NoWaitQueuedHandler::handle_queue_push_fail(Queue::QueueState state,
				      ConnectionInfo & conn)
{
    switch (state) {
	case Queue::CLOSED:
	    conn.respond(MHD_HTTP_INTERNAL_SERVER_ERROR, "{\"err\":\"Server is shutting down\"}", "application/json");
	    return true;
	case Queue::FULL:
	    conn.respond(MHD_HTTP_SERVICE_UNAVAILABLE, "{\"err\":\"Too many active requests\"}", "application/json");
	    return true;
	default:
	    // Do nothing
	    break;
    }
    return false;
}

void
NoWaitQueuedHandler::handle(ConnectionInfo & conn)
{
    LOG_DEBUG("NoWaitQueuedHandler: firstcall=" + str(conn.first_call) +
	      ", data=\"" + (conn.upload_data ? conn.upload_data : "NULL") +
	      "\", size=" + (conn.upload_data_size ? str(*(conn.upload_data_size)) : string("NULL")))
    if (conn.first_call) {
	return;
    }

    if (*(conn.upload_data_size) != 0) {
	// FIXME - enforce an upload size limit
	uploaded_data += string(conn.upload_data, *(conn.upload_data_size));
	*(conn.upload_data_size) = 0;
	return;
    }

    // FIXME - check Content-Type
    Json::Value body(Json::nullValue);
    if (uploaded_data.size() != 0) {
	// FIXME - handle failure to parse data
	json_unserialise(uploaded_data, body);
    }

    Queue::QueueState state;
    try {
	state = enqueue(conn, body);
    } catch (const InvalidValueError & e) {
	LOG_ERROR(string("Error queueing task: ") + e.what());
	Json::Value result(Json::objectValue);
	result["err"] = e.what();
	conn.respond(400, json_serialise(result), "application/json");
	return;
    }
    if (handle_queue_push_fail(state, conn)) {
	return;
    }

    // Return HTTP Accepted status code
    // FIXME - this response should really include a pointer to a status
    // monitor, or something similar.
    if (state == Queue::LOW_SPACE) {
	conn.respond(202, "{\"high_load\":1}", "application/json");
    } else {
	conn.respond(202, "{}", "application/json");
    }
}
