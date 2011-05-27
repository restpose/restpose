/** @file handlers.cc
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

#include <config.h>
#include "rest/handlers.h"

#include "httpserver/httpserver.h"
#include <microhttpd.h>
#include "server/task_manager.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

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
CollCreateHandler::create(const std::vector<std::string> & path_params) const
{
    std::auto_ptr<CollCreateHandler> result(new CollCreateHandler);
    result->coll_name = path_params[0];
    return result.release();
}

Queue::QueueState
CollCreateHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_create_coll(coll_name, resulthandle);
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
