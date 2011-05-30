/** @file tasks.cc
 * @brief Tasks to be placed on queues for performing later.
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
#include "tasks.h"

#include "jsonxapian/collection.h"
#include "loadfile.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

Task::~Task() {}

void
StaticFileTask::perform(RestPose::Collection *)
{
    string data;
    if (load_file(path, data)) {
	resulthandle.result_target() = data;
    } else {
	resulthandle.result_target()["err"] = "Couldn't load file";
    }
    resulthandle.set_ready();
}

void
CollInfoTask::perform(RestPose::Collection * collection)
{
    Json::Value & result(resulthandle.result_target());
    result["doc_count"] = collection->doc_count();
    Json::Value & types(result["types"] = Json::objectValue);
    // FIXME - set types to a map from typenames in the collection to their schemas.
    (void) types;
    resulthandle.set_ready();
}

void
PerformSearchTask::perform(RestPose::Collection * collection)
{
    Json::Value & result(resulthandle.result_target());
    collection->perform_search(search, result);
    resulthandle.set_ready();
}

void
ServerStatusTask::perform(RestPose::Collection *)
{
    Json::Value & result(resulthandle.result_target());
    Json::Value & tasks(result["tasks"] = Json::objectValue);
    {
	Json::Value & indexing(tasks["indexing"] = Json::objectValue);
	taskman->indexing_queues.get_status(indexing["queues"]);
	taskman->indexing_threads.get_status(indexing["threads"]);
    }
    {
	Json::Value & processing(tasks["processing"] = Json::objectValue);
	taskman->processing_queues.get_status(processing["queues"]);
	taskman->processing_threads.get_status(processing["threads"]);
    }
    {
	Json::Value & search(tasks["search"] = Json::objectValue);
	taskman->search_queues.get_status(search["queues"]);
	taskman->search_threads.get_status(search["threads"]);
    }
    resulthandle.set_ready();
}

void
ProcessorPipeDocumentTask::perform(RestPose::Collection & collection,
				   TaskManager * taskman)
{
    collection.send_to_pipe(taskman, pipe, doc);
}

void
ProcessorProcessDocumentTask::perform(RestPose::Collection & collection,
				      TaskManager * taskman)
{
    string idterm;
    Xapian::Document xdoc = collection.process_doc(type, doc, idterm);
    taskman->queue_index_processed_doc(collection.get_name(), xdoc, idterm);
}

void
IndexerUpdateDocumentTask::perform(RestPose::Collection & collection)
{
    collection.raw_update_doc(doc, idterm);
}

void
IndexerDeleteDocumentTask::perform(RestPose::Collection & collection)
{
    collection.raw_delete_doc(idterm);
}

void
IndexerCommitTask::perform(RestPose::Collection & collection)
{
    collection.commit();
}
