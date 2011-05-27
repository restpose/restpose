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

#include "server/server.h"
#include "server/result_handle.h"
#include "utils/threadsafequeue.h"

using namespace RestPose;

class TaskManager;
class ConnectionInfo;

/** Handlers for restful resources.
 *
 *  An instance handler is added to the router, associated with a given path
 *  pattern and HTTP method.  This "prototype" instance is used as a factory to
 *  create instances of the handler for handling each matching request.
 */
class Handler {
  protected:
    TaskManager * taskman;
    Server * server;
  public:
    Handler() : taskman(NULL), server(NULL) {}

    /** Set pointers to system resources, used for starting tasks, etc.
     */
    void set_context(TaskManager * taskman_, Server * server_) {
	taskman = taskman_;
	server = server_;
    }

    virtual ~Handler();

    /** Create a new handler, for handling this type of request, with the
     *  specified path parameters.
     */
    virtual Handler *
	    create(const std::vector<std::string> & path_params) const = 0;

    /** Handle a request.
     *
     *  This will be called multiple times by the server, until the request has
     *  been fully received (and called further times if the server is nudged,
     *  until the request has been handled).
     */
    virtual void handle(ConnectionInfo & info) = 0;
};

/** Handler for creating a collection.
 */
class CollCreateHandler : public Handler {
    std::string coll_name;
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
    void handle(ConnectionInfo & info);
};

/** Base class of handlers which put a task on a queue and wait for the
 *  response.
 */
class QueuedHandler : public Handler {
  protected:
    ResultHandle resulthandle;
    bool queued;

    /** Handle the request if the queue push failed.
     *
     *  Return true if the request has now been handled, false otherwise.
     */
    bool handle_queue_push_fail(Queue::QueueState state,
				ConnectionInfo & conn);
  public:
    QueuedHandler();
    void handle(ConnectionInfo & conn);
    virtual Queue::QueueState enqueue(const Json::Value & body) const = 0;
};

class ServerStatusHandler : public QueuedHandler {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
    Queue::QueueState enqueue(const Json::Value & body) const;
};

class CollInfoHandler : public QueuedHandler {
    std::string coll_name;
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
    Queue::QueueState enqueue(const Json::Value & body) const;
};

class SearchHandler : public QueuedHandler {
    std::string coll_name;
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
    Queue::QueueState enqueue(const Json::Value & body) const;
};

class NotFoundHandler : public Handler {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
    void handle(ConnectionInfo & info);
};


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
    std::map<int, const Handler *> handlers;

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
	     int methods, const Handler * handler);

    /** Get a newly created handler for a given path and method.
     *
     *  Append any wildcard path components matched to path_params.
     */
    const Handler * get(ConnectionInfo & conn,
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
    const Handler * default_handler;

    Handler * route_find(ConnectionInfo & conn) const;
  public:
    Router(TaskManager * taskman_, Server * server_);
    ~Router();

    /** Set the handler for a given path pattern and set of methods.
     */
    void add(const std::string & path_pattern, int methods, Handler * handler);

    /** Set the handler to use if no other handler matches.
     */
    void set_default(Handler *handler);

    /** Get a newly created handler for a connection.
     */
    Handler * route(ConnectionInfo & info) const;
};

#endif /* RESTPOSE_INCLUDED_ROUTER_H */
