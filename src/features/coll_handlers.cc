/** @file coll_handlers.cc
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

#include <config.h>
#include "features/coll_handlers.h"

#include "httpserver/httpserver.h"
#include "httpserver/response.h"
#include "server/task_manager.h"
#include "server/tasks.h"

using namespace std;
using namespace RestPose;


Handler *
CollListHandlerFactory::create(const std::vector<std::string> &) const
{
    return new CollListHandler;
}

Queue::QueueState
CollListHandler::enqueue(ConnectionInfo &,
			 const Json::Value &)
{
    return taskman->queue_readonly("info",
	new CollListTask(resulthandle, taskman->get_collections()));
}


Handler *
CollInfoHandlerFactory::create(
	const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollInfoHandler(coll_name);
}

Queue::QueueState
CollInfoHandler::enqueue(ConnectionInfo &,
			 const Json::Value &)
{
    return taskman->queue_readonly("info",
	new CollInfoTask(resulthandle, coll_name));
}


Handler *
CollCreateHandlerFactory::create(
	const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollCreateHandler(coll_name);
}

Queue::QueueState
CollCreateHandler::enqueue(ConnectionInfo &,
			   const Json::Value &)
{
    return Queue::FULL; // FIXME
}
