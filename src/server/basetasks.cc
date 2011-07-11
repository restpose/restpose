/** @file basetasks.cc
 * @brief Base classes of tasks.
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
#include "server/basetasks.h"

#include "server/task_manager.h"

using namespace RestPose;
using namespace std;

void
IndexingTask::perform(RestPose::Collection & collection,
		      TaskManager * taskman)
{
    try {
	perform_task(collection, taskman);
    } catch(const RestPose::Error & e) {
	string description, doc_type, doc_id;
	info(description, doc_type, doc_id);
	LOG_ERROR(description + " on collection '" + collection.get_name() +
		  "' failed", e);
	taskman->get_checkpoints().append_error(collection.get_name(),
	    description + " failed with " + e.what(), doc_type, doc_id);
    } catch(const Xapian::Error & e) {
	string description, doc_type, doc_id;
	info(description, doc_type, doc_id);
	LOG_ERROR(description + " on collection '" + collection.get_name() +
		  "' failed", e);
	taskman->get_checkpoints().append_error(collection.get_name(),
	    description + " failed with " + e.get_description(),
	    doc_type, doc_id);
    } catch(const std::bad_alloc & e) {
	string description, doc_type, doc_id;
	info(description, doc_type, doc_id);
	LOG_ERROR(description + " on collection '" + collection.get_name() +
		  "' failed", e);
	taskman->get_checkpoints().append_error(collection.get_name(),
	    description + " failed with out of memory", doc_type, doc_id);
    }
    post_perform(collection, taskman);
}

void
IndexingTask::post_perform(RestPose::Collection &,
			   TaskManager *)
{
}


DelayedIndexingTask::~DelayedIndexingTask()
{
    delete task;
}

void
DelayedIndexingTask::perform(const std::string & coll_name,
			     TaskManager * taskman)
{
    IndexingTask * tmp = task;
    task = NULL;
    taskman->queue_indexing_from_processing(coll_name, tmp);
}
