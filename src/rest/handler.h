/** @file handler.h
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
#ifndef RESTPOSE_INCLUDED_HANDLER_H
#define RESTPOSE_INCLUDED_HANDLER_H

#include "json/value.h"
#include "server/result_handle.h"
#include <string>
#include "utils/queueing.h"
#include <vector>

class TaskManager;
class Server;
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

    /** Handle a request.
     *
     *  This will be called multiple times by the server, until the request has
     *  been fully received (and called further times if the server is nudged,
     *  until the request has been handled).
     */
    virtual void handle(ConnectionInfo & info) = 0;
};

/** Factory for creating a new handler for a request, with the specified path
 *  parameters.
 *
 *  @param path_params The path components which were wildcards in the route
 *  pattern.
 */
class HandlerFactory {
  public:
    virtual Handler * create(const std::vector<std::string> & path_params) const = 0;
};

/** Base class of handlers which put a task on a queue and wait for the
 *  response.
 */
class QueuedHandler : public Handler {
  private:
    /** Flag used to indicate if the request has been queued.
     */
    bool queued;

    /** Handle the request if the queue push failed.
     *
     *  Return true if the request has now been handled, false otherwise.
     */
    bool handle_queue_push_fail(Queue::QueueState state,
				ConnectionInfo & conn);

  protected:
    /** The handle used to return the response.
     */
    RestPose::ResultHandle resulthandle;

  public:
    QueuedHandler();
    void handle(ConnectionInfo & conn);
    virtual Queue::QueueState enqueue(const Json::Value & body) const = 0;
};

/** Base class of handlers which put a task on a queue, and return immediately.
 */
class NoWaitQueuedHandler : public Handler {
    bool handle_queue_push_fail(Queue::QueueState state,
				ConnectionInfo & conn);
  public:
    NoWaitQueuedHandler();
    void handle(ConnectionInfo & conn);
    virtual Queue::QueueState enqueue(const Json::Value & body) const = 0;
};

#endif /* RESTPOSE_INCLUDED_HANDLER_H */
