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
#include "utils/validation.h"

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
PerformSearchTask::perform(RestPose::Collection * collection)
{
    if (!doc_type.empty()) {
	string error = validate_doc_type(doc_type);
	if (!error.empty()) {
	    resulthandle.failed(error, 400);
	    return;
	}
    }

    Json::Value result(Json::objectValue);
    collection->perform_search(search, doc_type, result);
    if (doc_type.empty()) {
	LOG_DEBUG("searched collection '" + collection->get_name() + "'");
    } else {
	LOG_DEBUG("searched collection '" + collection->get_name() +
		  "' within type '" + doc_type + "'");
    }
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
GetDocumentTask::perform(RestPose::Collection * collection)
{
    string error = validate_doc_type(doc_type);
    if (error.empty()) {
	error = validate_doc_id(doc_id);
    }
    if (!error.empty()) {
	resulthandle.failed(error, 400);
	return;
    }

    Json::Value result(Json::objectValue);
    collection->get_document(doc_type, doc_id, result);
    LOG_DEBUG("GetDocument '" + doc_id + "' from '" + collection->get_name() + "'");
    if (result.isNull()) {
	resulthandle.failed("No document found of type \"" + doc_type +
			    "\" and id \"" + doc_id + "\"", 404);
	return;
    }
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
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
    bool new_fields(false);
    config->send_to_pipe(taskman, target_pipe, doc, new_fields);
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
    // Validation happens in process_doc
    bool new_fields(false);
    Xapian::Document xdoc = config->process_doc(doc, doc_type, doc_id, idterm, errors, new_fields);
    for (vector<pair<string, string> >::const_iterator
	 i = errors.errors.begin(); i != errors.errors.end(); ++i) {
	string msg("Indexing error in field \"" + i->first + "\": \"" +
		   i->second + "\"");
	LOG_ERROR(msg);
	taskman->get_checkpoints().append_error(coll_name, msg,
						doc_type, doc_id);
    }
    if (errors.total_failure) {
	// Currently, there can only be one error if a total_failure occurs, so
	// we just raise it.
	throw InvalidValueError(errors.errors[0].first + ": " +
				errors.errors[0].second);
    }

    taskman->queue_indexing_from_processing(coll_name,
	new IndexerUpdateDocumentTask(idterm, xdoc));

    // FIXME - when it's just new fields that have been added, should send
    // out a task that updates the collection config with the new fields,
    // without overwriting any other new fields that have been added by
    // tasks running in parallel.

    if (config->is_changed() || new_fields) {
	LOG_DEBUG("Config has changed due to processing; applying new config");
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
IndexerConfigChangedTask::perform_task(const string & coll_name,
				       RestPose::Collection * & collection,
				       TaskManager * taskman)
{
    LOG_DEBUG("Updating configuration for collection " + coll_name);
    if (collection == NULL) {
	collection = taskman->get_collections().get_writable(coll_name);
    }
    collection->from_json(new_config);
}

void
IndexerConfigChangedTask::info(string & description,
			       string & doc_type,
			       string & doc_id) const
{
    description = "Updating configuration";
    doc_type.resize(0);
    doc_id.resize(0);
}

IndexingTask *
IndexerConfigChangedTask::clone() const
{
    return new IndexerConfigChangedTask(new_config);
}

void
IndexerUpdateDocumentTask::perform_task(const string & coll_name,
					RestPose::Collection * & collection,
					TaskManager * taskman)
{
    LOG_DEBUG("UpdateDocument idterm '" + idterm + "' in '" + coll_name + "'");
    if (collection == NULL) {
	collection = taskman->get_collections().get_writable(coll_name);
    }
    collection->raw_update_doc(doc, idterm);
}

void
IndexerUpdateDocumentTask::info(string & description,
				string & doc_type_ret,
				string & doc_id_ret) const
{
    description = "Updating document";
    string::size_type tab2 = idterm.find('\t', 1);
    if (tab2 == string::npos) {
	// Shouldn't happen, but we need to return something if it does.
	doc_id_ret = idterm.substr(1);
	doc_type_ret.resize(0);
	return;
    }
    doc_type_ret = idterm.substr(1, tab2 - 1);
    doc_id_ret = idterm.substr(tab2 + 1);
}

IndexingTask *
IndexerUpdateDocumentTask::clone() const
{
    return new IndexerUpdateDocumentTask(idterm, doc);
}


void
DeleteDocumentTask::perform_task(const string & coll_name,
				 RestPose::Collection * & collection,
				 TaskManager * taskman)
{
    string error = validate_doc_type(doc_type);
    if (error.empty()) {
	error = validate_doc_id(doc_id);
    }
    if (!error.empty()) {
	LOG_ERROR(error);
	taskman->get_checkpoints().append_error(coll_name, error,
						doc_type, doc_id);
	throw InvalidValueError(error);
    }

    LOG_INFO("DeleteDocument type='" + doc_type +
	     "' id='" + doc_id +
	     "' in '" + coll_name + "'");
    if (collection == NULL) {
	collection = taskman->get_collections().get_writable(coll_name);
    }
    collection->raw_delete_doc("\t" + doc_type + "\t" + doc_id);
}

void
DeleteDocumentTask::info(string & description,
			 string & doc_type_ret,
			 string & doc_id_ret) const
{
    description = "Delete document";
    doc_type_ret = doc_type;
    doc_id_ret = doc_id;
}

IndexingTask *
DeleteDocumentTask::clone() const
{
    return new DeleteDocumentTask(doc_type, doc_id);
}


void
DeleteCollectionProcessingTask::perform(const std::string & coll_name,
					TaskManager * taskman)
{
    taskman->get_collconfigs().reset(coll_name);
    taskman->queue_indexing_from_processing(coll_name,
	new DeleteCollectionTask);
}

void
DeleteCollectionTask::perform_task(const string & coll_name,
				   RestPose::Collection * & collection,
				   TaskManager * taskman)
{
    LOG_INFO("Delete collection '" + coll_name + "'");
    if (collection != NULL) {
	RestPose::Collection * tmp = collection;
	collection = NULL;
	taskman->get_collections().release(tmp);
    }
    taskman->get_collections().del(coll_name);
}

void
DeleteCollectionTask::info(string & description,
			   string & doc_type_ret,
			   string & doc_id_ret) const
{
    description = "Delete collection";
    doc_type_ret.resize(0);
    doc_id_ret.resize(0);
}

IndexingTask *
DeleteCollectionTask::clone() const
{
    return new DeleteCollectionTask;
}
