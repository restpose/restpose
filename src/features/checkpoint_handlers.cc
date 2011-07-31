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
#include "httpserver/httpserver.h"
#include "httpserver/response.h"
#include "server/task_manager.h"
#include "utils/validation.h"

using namespace std;
using namespace RestPose;

Handler *
CollCreateCheckpointHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    validate_collname_throw(coll_name);
    return new CollCreateCheckpointHandler(coll_name);
}

Queue::QueueState
CollCreateCheckpointHandler::create_checkpoint(TaskManager * taskman,
					       const std::string & new_coll_name,
					       std::string & checkid,
					       bool do_commit,
					       bool allow_throttle)
{
    checkid = taskman->get_checkpoints().alloc_checkpoint(new_coll_name);
    Queue::QueueState state = taskman->queue_processing(new_coll_name,
	new DelayedIndexingTask(
	    new IndexingCheckpointTask(checkid, do_commit)),
	allow_throttle);
    if (state != Queue::CLOSED && state != Queue::FULL) {
	taskman->get_checkpoints().publish_checkpoint(new_coll_name, checkid);
    }
    return state;
}

Queue::QueueState
CollCreateCheckpointHandler::enqueue(ConnectionInfo & conn,
				     const Json::Value &)

{
    string checkid;
    bool do_commit = conn.get_uri_arg_bool("commit", true);
    Queue::QueueState state = create_checkpoint(taskman, coll_name, checkid,
						do_commit, true);
    Json::Value result(Json::objectValue);
    result["checkid"] = checkid;
    resulthandle.response().set(result, 201);
    resulthandle.response().add_header("Location", "http://" + conn.host + "/coll/" + coll_name + "/checkpoint/" + checkid);
    resulthandle.set_ready();

    return state;
}

Handler *
CollGetCheckpointsHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    validate_collname_throw(coll_name);
    return new CollGetCheckpointsHandler(coll_name);
}

Queue::QueueState
CollGetCheckpointsHandler::enqueue(ConnectionInfo &,
				   const Json::Value &)
{
    return taskman->queue_readonly("checkpoints",
	new CollGetCheckpointsTask(resulthandle, coll_name, taskman));
}


Handler *
CollGetCheckpointHandlerFactory::create(const std::vector<std::string> & path_params) const
{
    string coll_name = path_params[0];
    validate_collname_throw(coll_name);
    string checkid = path_params[1];
    return new CollGetCheckpointHandler(coll_name, checkid);
}

Queue::QueueState
CollGetCheckpointHandler::enqueue(ConnectionInfo &,
				  const Json::Value &)
{
    return taskman->queue_readonly("checkpoints",
	new CollGetCheckpointTask(resulthandle, coll_name, taskman, checkid));
}
