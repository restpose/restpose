/** @file checkpoint_handlers.cc
 * @brief Handlers related to checkpoints.
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
#include "features/checkpoint_handlers.h"

#include "features/checkpoint_tasks.h"
#include "httpserver/response.h"
#include "server/task_manager.h"

using namespace std;
using namespace RestPose;

Handler *
CollCreateCheckpointHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollCreateCheckpointHandler(coll_name);
}

Queue::QueueState
CollCreateCheckpointHandler::enqueue(const Json::Value &)
{
    string checkid;
    checkid = taskman->get_checkpoints().alloc_checkpoint(coll_name);
    bool do_commit = true; // FIXME - get this from a parameter.
    Queue::QueueState state = taskman->queue_processing(coll_name,
	new ProcessorCheckpointTask(checkid, do_commit),
	false);
    if (state != Queue::CLOSED && state != Queue::FULL) {
	taskman->get_checkpoints().publish_checkpoint(coll_name, checkid);
    }

    Json::Value result(Json::objectValue);
    result["id"] = checkid;
    result["ok"] = 1;
    resulthandle.response().set(result, 201);
    resulthandle.response().add_header("Location", "/coll/" + coll_name + "/checkpoint/" + checkid);
    resulthandle.set_ready();

    return state;
}


Handler *
CollGetCheckpointsHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    return new CollGetCheckpointsHandler(coll_name);
}

Queue::QueueState
CollGetCheckpointsHandler::enqueue(const Json::Value &)
{
    return taskman->queue_readonly("checkpoints",
	new CollGetCheckpointsTask(resulthandle, coll_name, taskman));
}


Handler *
CollGetCheckpointHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    string checkid = path_params[1];
    return new CollGetCheckpointHandler(coll_name, checkid);
}

Queue::QueueState
CollGetCheckpointHandler::enqueue(const Json::Value &)
{
    return taskman->queue_readonly("checkpoints",
	new CollGetCheckpointTask(resulthandle, coll_name, taskman, checkid));
}
