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

class Handler {
  public:
    virtual ~Handler();
    virtual void handle(ConnectionInfo & info) = 0;
};

class CollCreateHandler : public Handler {
    TaskManager * taskman;
    Server * server;
  public:
    CollCreateHandler(TaskManager * taskman_, Server * server_)
	    : taskman(taskman_), server(server_) {}
    void handle(ConnectionInfo & info);
};

/** Base class of handlers which put a task on a queue and wait for the
 *  response.
 */
class QueuedHandler : public Handler {
  protected:
    TaskManager * taskman;
    ResultHandle resulthandle;
    bool queued;

  public:
    QueuedHandler(TaskManager * taskman_);

    void handle(ConnectionInfo & conn);

    virtual Queue::QueueState enqueue(const Json::Value & body) const = 0;
};

class ServerStatusHandler : public QueuedHandler {
  public:
    ServerStatusHandler(TaskManager * taskman_) : QueuedHandler(taskman_) {}
    Queue::QueueState enqueue(const Json::Value & body) const;
};

class CollInfoHandler : public QueuedHandler {
    std::string collection;
  public:
    CollInfoHandler(TaskManager * taskman_, const std::string & collection_)
	    : QueuedHandler(taskman_), collection(collection_)
    {}
    Queue::QueueState enqueue(const Json::Value & body) const;
};

class SearchHandler : public QueuedHandler {
    std::string collection;
  public:
    SearchHandler(TaskManager * taskman_, const std::string & collection_)
	    : QueuedHandler(taskman_), collection(collection_)
    {}
    Queue::QueueState enqueue(const Json::Value & body) const;
};



class NotFoundHandler : public Handler {
  public:
    void handle(ConnectionInfo & info);
};

class Router {
    TaskManager * taskman;
    Server * server;

  public:
    Router(TaskManager * taskman_, Server * server_);

    /// Handle a connection.
    Handler * route(ConnectionInfo & info) const;
};

#endif /* RESTPOSE_INCLUDED_ROUTER_H */
