/** @file handlers.h
 * @brief Definition of concrete handlers
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
#ifndef RESTPOSE_INCLUDED_HANDLERS_H
#define RESTPOSE_INCLUDED_HANDLERS_H

#include "rest/handler.h"

class RootHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class FileHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class FileHandler : public QueuedHandler {
    std::string path;
  public:
    FileHandler(const std::string & path_)
	    : path(path_)
    {}

    Queue::QueueState enqueue(const Json::Value & body) const;
};


class ServerStatusHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class ServerStatusHandler : public QueuedHandler {
  public:
    Queue::QueueState enqueue(const Json::Value & body) const;
};


class CollCreateHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class CollCreateHandler : public QueuedHandler {
    std::string coll_name;
  public:
    CollCreateHandler(const std::string & coll_name_)
	    : coll_name(coll_name_)
    {}

    Queue::QueueState enqueue(const Json::Value & body) const;
};


class CollListHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> &) const;
};

class CollListHandler : public QueuedHandler {
  public:
    Queue::QueueState enqueue(const Json::Value &) const;
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

    Queue::QueueState enqueue(const Json::Value & body) const;
};


class SearchHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class SearchHandler : public QueuedHandler {
    std::string coll_name;
    std::string doc_type;
  public:
    SearchHandler(const std::string & coll_name_,
		  const std::string & doc_type_)
	    : coll_name(coll_name_),
	      doc_type(doc_type_)
    {}

    Queue::QueueState enqueue(const Json::Value & body) const;
};


class NotFoundHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};

class NotFoundHandler : public Handler {
  public:
    void handle(ConnectionInfo & info);
};

#endif /* RESTPOSE_INCLUDED_HANDLERS_H */
