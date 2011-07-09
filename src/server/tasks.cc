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

#include "httpserver/response.h"
#include "jsonxapian/collection.h"
#include "jsonxapian/collection_pool.h"
#include "jsonxapian/indexing.h"
#include "jsonxapian/pipe.h"
#include "loadfile.h"
#include "logger/logger.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"
#include "utils/stringutils.h"

using namespace std;
using namespace RestPose;

Task::~Task() {}

void
StaticFileTask::perform(RestPose::Collection *)
{
    Response & response(resulthandle.response());
    string data;
    if (load_file(path, data)) {
	response.set_data(data);
	if (string_endswith(path, ".html")) {
	    response.set_content_type("text/html");
	} else if (string_endswith(path, ".js")) {
	    response.set_content_type("application/javascript");
	} else if (string_endswith(path, ".css")) {
	    response.set_content_type("text/css");
	} else if (string_endswith(path, ".png")) {
	    response.set_content_type("image/png");
	} else if (string_endswith(path, ".jpg")) {
	    response.set_content_type("image/jpeg");
	} else {
	    response.set_content_type("text/plain");
	}
	response.set_status(200);
    } else {
	Json::Value result(Json::objectValue);
	result["err"] = "Couldn't load file" + path; // FIXME - explain the error
	response.set(result, 404); // FIXME - only return 404 if it's a file not found error.
    }
    resulthandle.set_ready();
}

void
CollListTask::perform(RestPose::Collection *)
{
    vector<string> collnames;
    collections.get_names(collnames);
    Json::Value result(Json::objectValue);
    for (vector<string>::const_iterator
	 i = collnames.begin(); i != collnames.end(); ++i) {
	result[*i] = Json::objectValue;
    }

    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
CollInfoTask::perform(RestPose::Collection * collection)
{
    Json::Value result(Json::objectValue);
    result["doc_count"] = Json::UInt64(collection->doc_count());
    Json::Value tmp;
    collection->to_json(tmp);
    result["default_type"] = tmp["default_type"];
    result["special_fields"] = tmp["special_fields"];
    result["types"] = tmp["types"];
    result["pipes"] = tmp["pipes"];
    result["categorisers"] = tmp["categorisers"];

    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
PerformSearchTask::perform(RestPose::Collection * collection)
{
    Json::Value result(Json::objectValue);
    collection->perform_search(search, doc_type, result);
    LOG_DEBUG("searched collection '" + collection->get_name() + "'");
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
GetDocumentTask::perform(RestPose::Collection * collection)
{
    Json::Value result(Json::objectValue);
    collection->get_document(doc_type, doc_id, result);
    LOG_DEBUG("GetDocument '" + doc_id + "' from '" + collection->get_name() + "'");
    if (result.isNull()) {
	result = Json::objectValue;
	result["err"] = "No document found of type \"" + doc_type +
		"\" and id \"" + doc_id + "\"";
	resulthandle.failed(result, 404);
    } else {
	resulthandle.response().set(result, 200);
	resulthandle.set_ready();
    }
}


void
ServerStatusTask::perform(RestPose::Collection *)
{
    Json::Value result(Json::objectValue);
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
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
ProcessorPipeDocumentTask::perform(const string & coll_name,
				   TaskManager * taskman)
{
    LOG_DEBUG("PipeDocument to '" + target_pipe + "' in '" + coll_name + "'");
    auto_ptr<CollectionConfig> config(taskman->get_collconfigs()
				      .get(coll_name));
    config->send_to_pipe(taskman, target_pipe, doc);
}

void
ProcessorProcessDocumentTask::perform(const string & coll_name,
				      TaskManager * taskman)
{
    LOG_DEBUG("ProcessDocument type '" + doc_type + "' in '" + coll_name + "'");
    auto_ptr<CollectionConfig> config(taskman->get_collconfigs()
				      .get(coll_name));
    string idterm;
    config->clear_changed();
    IndexingErrors errors;
    Xapian::Document xdoc = config->process_doc(doc, doc_type, doc_id, idterm, errors);
    if (errors.total_failure) {
	// Currently, there can only be one error if a total_failure occurs, so
	// we just raise it.
	throw InvalidValueError(errors.errors[0].first + ": " +
				errors.errors[0].second);
    }
    for (vector<pair<string, string> >::const_iterator
	 i = errors.errors.begin(); i != errors.errors.end(); ++i) {
	LOG_ERROR("Indexing error in field \"" + i->first + "\": \"" +
		  i->second + "\"");
    }

    taskman->queue_indexing_from_processing(coll_name,
	new IndexerUpdateDocumentTask(idterm, xdoc));

    if (config->is_changed()) {
	// FIXME - could push just the new config for the schema for the doc_type in question, to save work.
	Json::Value tmp;
	config->to_json(tmp);
	taskman->queue_indexing_from_processing(coll_name,
	    new IndexerConfigChangedTask(tmp));
	config->clear_changed();

	// Push the new config back to the cache.
	taskman->get_collconfigs().set(coll_name, config.release());
    }
}

void
IndexerConfigChangedTask::perform(RestPose::Collection & collection,
				  TaskManager *)
{
    LOG_DEBUG("Updating configuration for collection " + collection.get_name());
    collection.from_json(new_config);
}

IndexingTask *
IndexerConfigChangedTask::clone() const
{
    return new IndexerConfigChangedTask(new_config);
}

void
IndexerUpdateDocumentTask::perform(RestPose::Collection & collection,
				   TaskManager *)
{
    LOG_DEBUG("UpdateDocument idterm '" + idterm + "' in '" +
	      collection.get_name() + "'");
    collection.raw_update_doc(doc, idterm);
}

IndexingTask *
IndexerUpdateDocumentTask::clone() const
{
    return new IndexerUpdateDocumentTask(idterm, doc);
}

void
DeleteDocumentTask::perform(RestPose::Collection & collection,
			    TaskManager *)
{
    LOG_INFO("DeleteDocument type='" + doc_type +
	     "' id='" + doc_id +
	     "' in '" + collection.get_name() + "'");
    collection.raw_delete_doc("\t" + doc_type + "\t" + doc_id);
}

IndexingTask *
DeleteDocumentTask::clone() const
{
    return new DeleteDocumentTask(doc_type, doc_id);
}
