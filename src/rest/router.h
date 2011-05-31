/** @file router.h
 * @brief Route REST requests to a handler.
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
#ifndef RESTPOSE_INCLUDED_ROUTER_H
#define RESTPOSE_INCLUDED_ROUTER_H

#include <map>
#include <string>
#include "utils/queueing.h"
#include <vector>

class ConnectionInfo;
class Handler;
class HandlerFactory;
class Server;
class TaskManager;

class RouteLevel {
    /** Level number.
     *
     *  This is the index into the components of the url that this route level
     *  is for.
     */
    unsigned level;

    /** Map from initial path component to sub levels for that component.
     */
    std::map<std::string, RouteLevel *> routes;

    /** Handlers, by HTTP method.
     */
    std::map<int, const HandlerFactory *> handlers;

    int allowed_methods;

    RouteLevel(const RouteLevel &);
    void operator=(const RouteLevel &);
  public:
    RouteLevel(unsigned level_=0) : level(level_) {}
    ~RouteLevel();

    void set_level(unsigned level_) {
	level = level_;
    }

    /** Add a handler for a given path pattern and set of methods.
     */
    void add(const std::string & path_pattern, size_t pattern_offset,
	     int methods, const HandlerFactory * handler);

    /** Get a handler factory for a given path and method.
     *
     *  Append any wildcard path components matched to path_params.
     */
    const HandlerFactory * get(ConnectionInfo & conn,
			       std::vector<std::string> & path_params) const;
};

class Router {
    TaskManager * taskman;
    Server * server;

    /** The routes to look through to find the handler to use.
     */
    RouteLevel routes;

    /** Handler used if none of the routes match.
     */
    const HandlerFactory * default_handler;

    Handler * route_find(ConnectionInfo & conn) const;
  public:
    Router(TaskManager * taskman_, Server * server_);
    ~Router();

    /** Set the handler for a given path pattern and set of methods.
     *
     *  The pattern may contain path components which are used for literal matches, or are one of the specified forms.
     *
     *   - "?": match any single path component.
     *   - "*": match all trailing path component.  This must be the last component.
     *
     *  @param path_pattern The pattern for the path to match.
     *  @param method A bitmask of HTTPMethod values to accept for this handler.
     *  @param factory The factory to use when the pattern and method match.
     */
    void add(const std::string & path_pattern, int methods, HandlerFactory * factory);

    /** Set the handler to use if no other handler matches.
     */
    void set_default(HandlerFactory *factory);

    /** Get a newly created handler for a connection.
     */
    Handler * route(ConnectionInfo & info) const;
};

#endif /* RESTPOSE_INCLUDED_ROUTER_H */
