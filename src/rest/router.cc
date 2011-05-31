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
#include "rest/router.h"

#include <cstring>
#include "httpserver/httpserver.h"
#include "omassert.h"
#include "rest/handler.h"
#include "rest/handlers.h"
#include "server/task_manager.h"
#include "utils/rsperrors.h"

using namespace std;
using namespace RestPose;

RouteLevel::~RouteLevel()
{
    for (std::map<int, const HandlerFactory *>::const_iterator
	 i = handlers.begin(); i != handlers.end(); ++i) {
	const HandlerFactory * ptr = i->second;
	for (std::map<int, const HandlerFactory *>::iterator
	     j = handlers.begin(); j != handlers.end(); ++j) {
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
		int methods, const HandlerFactory * handler_)
{
    auto_ptr<const HandlerFactory> handler(handler_);
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

const HandlerFactory *
RouteLevel::get(ConnectionInfo & conn,
		std::vector<std::string> & path_params) const
{
    if (conn.components.size() == level) {
	if (!conn.require_method(allowed_methods)) {
	    return NULL;
	}
	map<int, const HandlerFactory *>::const_iterator 
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
    const HandlerFactory * factory = routes.get(conn, path_params);
    if (conn.responded) {
	// Happens when routing finds the method isn't valid for the
	// resource.
	Assert(factory == NULL);
	return NULL;
    }
    if (factory == NULL) {
	path_params.clear();
	factory = default_handler;
    }
    if (factory != NULL) {
	return factory->create(path_params);
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
}

Router::~Router()
{
    delete default_handler;
}

void
Router::add(const std::string & path_pattern, int methods,
	    HandlerFactory * handler)
{
    routes.add(path_pattern, 0, methods, handler);
}

void
Router::set_default(HandlerFactory * handler)
{
    delete default_handler;
    default_handler = handler;
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
