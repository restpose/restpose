/** @file coll_handlers.h
 * @brief Handlers related to collections.
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
#ifndef RESTPOSE_INCLUDED_COLL_HANDLERS_H
#define RESTPOSE_INCLUDED_COLL_HANDLERS_H

#include "rest/handler.h"

class CollListHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> &) const;
};
class CollListHandler : public QueuedHandler {
  public:
    Queue::QueueState enqueue(ConnectionInfo & conn,
			      const Json::Value &);
};


class CollInfoHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};
class CollInfoHandler : public QueuedHandler {
    std::string coll_name;
  public:
    CollInfoHandler(const std::string & coll_name_)
	    : coll_name(coll_name_)
    {}

    Queue::QueueState enqueue(ConnectionInfo & conn,
			      const Json::Value & body);
};


class CollCreateHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class CollCreateHandler : public NoWaitQueuedHandler {
    std::string coll_name;
  public:
    CollCreateHandler(const std::string & coll_name_)
	    : coll_name(coll_name_)
    {}

    Queue::QueueState enqueue(ConnectionInfo & conn,
			      const Json::Value & body);
};


#endif /* RESTPOSE_INCLUDED_COLL_HANDLERS_H */
