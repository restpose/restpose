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
#include "server/tasks.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

#define DIR_SEPARATOR "/"

Handler *
RootHandlerFactory::create(const vector<string> &) const
{
    return new FileHandler("static" DIR_SEPARATOR "index.html");
}

Handler *
FileHandlerFactory::create(const vector<string> &path_params) const
{
    string filepath("static" DIR_SEPARATOR "static");
    for (vector<string>::const_iterator i = path_params.begin();
	 i != path_params.end(); ++i) {
	filepath += DIR_SEPARATOR;
	filepath += *i;
    }
    
    return new FileHandler(filepath);
}

Queue::QueueState
FileHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_readonly("static",
	new StaticFileTask(resulthandle, path));
}


Handler *
ServerStatusHandlerFactory::create(const std::vector<std::string> &) const
{
    return new ServerStatusHandler;
}

Queue::QueueState
ServerStatusHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_readonly("status",
	new ServerStatusTask(resulthandle, taskman));
}

Handler *
CollCreateHandlerFactory::create(
	const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollCreateHandler(coll_name);
}

Queue::QueueState
CollCreateHandler::enqueue(const Json::Value &) const
{
    return Queue::FULL; // FIXME
}

Handler *
CollInfoHandlerFactory::create(
	const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollInfoHandler(coll_name);
}

Queue::QueueState
CollInfoHandler::enqueue(const Json::Value &) const
{
    return taskman->queue_get_collinfo(coll_name, resulthandle);
}

Handler *
SearchHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new SearchHandler(coll_name);
}

Queue::QueueState
SearchHandler::enqueue(const Json::Value & body) const
{
    return taskman->queue_search(coll_name, resulthandle, body);
}

Handler *
NotFoundHandlerFactory::create(const std::vector<std::string> &) const
{
    return new NotFoundHandler;
}

void
NotFoundHandler::handle(ConnectionInfo & conn)
{
    conn.respond(MHD_HTTP_NOT_FOUND, "Resource not found", "text/plain");
}
