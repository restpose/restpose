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

#include "server/task_manager.h"

void
ProcessorCheckpointTask::perform(const std::string & coll_name,
				 TaskManager * taskman)
{
    taskman->queue_indexing_from_processing(coll_name,
	new IndexingCheckpointTask(checkid, do_commit));

}

void
IndexingCheckpointTask::perform(RestPose::Collection & collection)
{
    if (do_commit) {
	LOG_INFO("Checkpoint '" + checkid + "' reached in '" + collection.get_name() + "' - committing");
	collection.commit();
    } else {
	LOG_INFO("Checkpoint '" + checkid + "' reached in '" + collection.get_name() + "'");
    }
}

IndexingTask *
IndexingCheckpointTask::clone() const
{
    return new IndexingCheckpointTask(checkid, do_commit);
}
