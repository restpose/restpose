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

#include "rest/handler.h"
#include "rest/handlers.h"

#include "httpserver/httpserver.h"
#include "omassert.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

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

Handler *
CollCreateHandler::create(const std::vector<std::string> & path_params) const
{
    std::auto_ptr<CollCreateHandler> result(new CollCreateHandler);
    result->coll_name = path_params[0];
    return result.release();
}

void
CollCreateHandler::handle(ConnectionInfo & conn)
{
    if (conn.first_call) {
	return;
    }

    Json::Value result(Json::objectValue);
    conn.respond(MHD_HTTP_OK, json_serialise(result), "application/json");
}

QueuedHandler::QueuedHandler()
	: Handler(),
	  resulthandle(),
	  queued(false)
{}

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
#if 0
    printf("QueuedHandler: firstcall=%d, queued=%d, data=!%s!, size=%d\n",
	   conn.first_call, queued,
	   conn.upload_data,
	   *(conn.upload_data_size)
	  );
#endif
    if (conn.first_call) {
	resulthandle.set_nudge(taskman->get_nudge_fd(), 'H');
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
	if (handle_queue_push_fail(state, conn)) {
	    return;
	}
	queued = true;
    } else {
	const Json::Value * result = resulthandle.get_result();
	if (result == NULL) {
	    return;
	}
	conn.respond(resulthandle.get_status(), json_serialise(*result), "application/json");
    }
}

Handler *
ServerStatusHandler::create(const std::vector<std::string> &) const
{
    return new ServerStatusHandler;
}

Queue::QueueState
ServerStatusHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_get_status(resulthandle);
}

Handler *
CollInfoHandler::create(const std::vector<std::string> & path_params) const
{
    std::auto_ptr<CollInfoHandler> result(new CollInfoHandler);
    result->coll_name = path_params[0];
    return result.release();
}

Queue::QueueState
CollInfoHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_get_collinfo(coll_name, resulthandle);
}

Handler *
SearchHandler::create(const std::vector<std::string> & path_params) const
{
    std::auto_ptr<SearchHandler> result(new SearchHandler);
    result->coll_name = path_params[0];
    return result.release();
}

Queue::QueueState
SearchHandler::enqueue(const Json::Value & body) const
{
    return taskman->queue_search(coll_name, resulthandle, body);
}

Handler *
NotFoundHandler::create(const std::vector<std::string> &) const
{
    return new NotFoundHandler;
}

void
NotFoundHandler::handle(ConnectionInfo & conn)
{
    conn.respond(MHD_HTTP_NOT_FOUND, "Resource not found", "text/plain");
}

RouteLevel::~RouteLevel()
{
    for (std::map<int, const Handler *>::const_iterator i = handlers.begin();
	 i != handlers.end(); ++i) {
	const Handler * ptr = i->second;
	for (std::map<int, const Handler *>::iterator j = handlers.begin();
	     j != handlers.end(); ++j) {
	    if (j->second == ptr) {
		j->second = NULL;
	    }
	}
	delete ptr;
    }
    for (std::map<std::string, RouteLevel *>::iterator
	 i = routes.begin(); i != routes.end(); ++i) {
	delete i->second;
    }
}

void
RouteLevel::add(const std::string & path_pattern,
		size_t pattern_offset,
		int methods, const Handler * handler_)
{
    auto_ptr<const Handler> handler(handler_);
    if (pattern_offset != path_pattern.size()) {
	// Add or update the entry in the routes map

	// pattern_offset should be pointing to a '/'
	pattern_offset += 1;
	const char * next = strchr(path_pattern.c_str() + pattern_offset, '/');
	size_t next_offset;
	std::string component;
	if (next == NULL) {
	    next_offset = path_pattern.size();
	    component = path_pattern.substr(pattern_offset);
	} else {
	    next_offset = next - path_pattern.c_str();
	    component = path_pattern.substr(pattern_offset,
					    next_offset - pattern_offset);
	}

	std::map<std::string, RouteLevel *>::iterator
		i = routes.find(component);
	if (i == routes.end()) {
	    routes[component] = NULL; // Allocate space in routes
	    routes[component] = new RouteLevel(level + 1);
	}
	RouteLevel & child = *(routes[component]);
	child.add(path_pattern, next_offset, methods, handler.release());
	return;
    }

    // Add an entry at this route level.
    for (int i = 1; i <= HTTP_METHODMASK_MAX; i *= 2) {
	if (methods & i) {
	    if (handlers.find(i) != handlers.end()) {
		throw InvalidValueError("Duplicate handlers specified for path");
	    }
	    handlers[i] = NULL;
	    if (handler.get() != NULL) {
		handlers[i] = handler.release();
	    } else {
		// Multiple methods handled by this handler.
		handlers[i] = handler_;
	    }
	}
    }
    allowed_methods |= methods;
}

const Handler *
RouteLevel::get(ConnectionInfo & conn,
		std::vector<std::string> & path_params) const
{
    if (conn.components.size() == level) {
	if (!conn.require_method(allowed_methods)) {
	    return NULL;
	}
	map<int, const Handler *>::const_iterator 
		i = handlers.find(conn.method);
	if (i == handlers.end()) {
	    // Shouldn't happen; the check on allowed_methods earlier should
	    // catch this.
	    return NULL;
	}
	return i->second;
    }

    // Check for an exact match at this level.
    std::map<std::string, RouteLevel *>::const_iterator i;
    i = routes.find(conn.components[level]);
    if (i == routes.end()) {
	i = routes.find("?");
    }
    if (i == routes.end()) {
	return NULL;
    }
    if (i->first == "?") {
	path_params.push_back(conn.components[level]);
    }
    return i->second->get(conn, path_params);
}

Handler *
Router::route_find(ConnectionInfo & conn) const
{
    conn.parse_url_components();
    vector<string> path_params;
    const Handler * prototype = routes.get(conn, path_params);
    if (conn.responded) {
	// Happens when routing finds the method isn't valid for the
	// resource.
	Assert(prototype == NULL);
	return NULL;
    }
    if (prototype == NULL) {
	path_params.clear();
	prototype = default_handler;
    }
    if (prototype != NULL) {
	return prototype->create(path_params);
    } else {
	return NULL;
    } 
}

Router::Router(TaskManager * taskman_, Server * server_)
	: taskman(taskman_),
	  server(server_),
	  routes(0),
	  default_handler(NULL)
{
    // FIXME - these calls should be done externally.
    add("/status", HTTP_GETHEAD, new ServerStatusHandler);
    add("/coll/?", HTTP_GETHEAD, new CollInfoHandler);
    add("/coll/?", HTTP_PUT, new CollCreateHandler);
    //add("/coll/?/type/?", HTTP_GET, new GetDocumentHandler);
    //add("/coll/?/type/?", HTTP_POST, new IndexDocumentHandler);
    add("/coll/?/search", HTTP_GETHEAD | HTTP_POST, new SearchHandler);
    //add("/coll/?/type/?/search", HTTP_GETHEAD | HTTP_POST, new SearchHandler);
    set_default(new NotFoundHandler);
}

Router::~Router()
{
    delete default_handler;
}

void
Router::add(const std::string & path_pattern, int methods, Handler * handler)
{
    auto_ptr<Handler> handler_ptr(handler);
    handler_ptr->set_context(taskman, server);
    routes.add(path_pattern, 0, methods, handler_ptr.release());
}

void
Router::set_default(Handler * handler)
{
    delete default_handler;
    default_handler = handler;
    handler->set_context(taskman, server);
}

Handler *
Router::route(ConnectionInfo & conn) const
{
    auto_ptr<Handler> handler(route_find(conn));
    if (handler.get() != NULL) {
	handler->set_context(taskman, server);
    }
    return handler.release();
}
