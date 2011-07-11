/** @file checkpoint_tasks.cc
 * @brief Tasks related to checkpoints.
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
#include "features/checkpoint_tasks.h"

#include "httpserver/response.h"
#include "server/task_manager.h"

using namespace std;

void
IndexingCheckpointTask::perform_task(RestPose::Collection & collection,
				     TaskManager *)
{
    if (do_commit) {
	LOG_INFO("Checkpoint '" + checkid + "' reached in '" +
		 collection.get_name() + "' - committing");
	collection.commit();
    } else {
	LOG_INFO("Checkpoint '" + checkid + "' reached in '" +
		 collection.get_name() + "'");
    }
}

void
IndexingCheckpointTask::post_perform(RestPose::Collection & collection,
				     TaskManager * taskman)
{
    taskman->get_checkpoints().set_reached(collection.get_name(), checkid);
}

void
IndexingCheckpointTask::info(string & description, string & doc_type,
			     string & doc_id) const
{
    description = "Performing checkpoint";
    doc_type.resize(0);
    doc_id.resize(0);
}


IndexingTask *
IndexingCheckpointTask::clone() const
{
    return new IndexingCheckpointTask(checkid, do_commit);
}

void
CollGetCheckpointsTask::perform(RestPose::Collection *)
{
    Json::Value result(Json::objectValue);
    taskman->get_checkpoints().ids_to_json(coll_name, result);
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
CollGetCheckpointTask::perform(RestPose::Collection *)
{
    Json::Value result(Json::objectValue);
    taskman->get_checkpoints().get_state(coll_name, checkid, result);
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}
