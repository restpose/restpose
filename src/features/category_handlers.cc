/** @file category_handlers.cc
 * @brief Handlers related to categories.
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
#include "features/category_handlers.h"

#include "features/category_tasks.h"
#include "httpserver/httpserver.h"
#include "httpserver/response.h"
#include <memory>
#include "server/task_manager.h"
#include "utils/validation.h"

using namespace std;
using namespace RestPose;

Handler *
CollGetCategoryHandlerFactory::create(const vector<string> & path_params) const
{
    string coll_name = path_params[0];
    string taxonomy_name;
    string cat_id;
    string parent_id;

    validate_collname_throw(coll_name);

    if (path_params.size() >= 2) {
	taxonomy_name = path_params[1];
	validate_catid_throw(taxonomy_name);
    }
    if (path_params.size() >= 3) {
	cat_id = path_params[2];
	validate_catid_throw(cat_id);
    }
    if (path_params.size() >= 4) {
	parent_id = path_params[3];
	validate_catid_throw(parent_id);
    }

    return new CollGetCategoryHandler(coll_name, taxonomy_name, cat_id,
				      parent_id);
}

Queue::QueueState
CollGetCategoryHandler::enqueue(ConnectionInfo &,
				const Json::Value &)
{
    auto_ptr<ReadonlyTask> task;
    if (taxonomy_name.empty()) {
	task = auto_ptr<ReadonlyTask>(
	    new CollGetTaxonomiesTask(resulthandle, coll_name, taskman));
    } else if (cat_id.empty()) {
	task = auto_ptr<ReadonlyTask>(
	    new CollGetTaxonomyTask(resulthandle, coll_name,
				    taxonomy_name, taskman));
    } else if (parent_id.empty()) {
	task = auto_ptr<ReadonlyTask>(
	    new CollGetCategoryTask(resulthandle, coll_name, taxonomy_name,
				    cat_id, taskman));
    } else {
	task = auto_ptr<ReadonlyTask>(
	    new CollGetCategoryParentTask(resulthandle, coll_name,
					  taxonomy_name, cat_id, parent_id,
					  taskman));
    }

    return taskman->queue_readonly("categories", task.release());
}

Handler *
CollPutCategoryHandlerFactory::create(const vector<string> & path_params) const
{
    string coll_name = path_params[0];
    string taxonomy_name = path_params[1];
    string cat_id = path_params[2];
    string parent_id;

    validate_collname_throw(coll_name);
    validate_catid_throw(taxonomy_name);
    validate_catid_throw(cat_id);

    if (path_params.size() >= 4) {
	parent_id = path_params[3];
	validate_catid_throw(parent_id);
    }

    return new CollPutCategoryHandler(coll_name, taxonomy_name, cat_id,
				      parent_id);
}

Queue::QueueState
CollPutCategoryHandler::enqueue(ConnectionInfo &,
				const Json::Value &)
{
    auto_ptr<ProcessingTask> task;

    if (parent_id.empty()) {
	task = auto_ptr<ProcessingTask>(
	    new ProcessingCollPutCategoryTask(taxonomy_name, cat_id));
    } else {
	task = auto_ptr<ProcessingTask>(
	    new ProcessingCollPutCategoryParentTask(taxonomy_name, cat_id,
						    parent_id));
    }

    return taskman->queue_processing(coll_name, task.release(), true);
}

Handler *
CollDeleteCategoryHandlerFactory::create(const vector<string> & path_params) const
{
    string coll_name = path_params[0];
    string taxonomy_name = path_params[1];
    string cat_id;
    string parent_id;

    validate_collname_throw(coll_name);
    validate_catid_throw(taxonomy_name);

    if (path_params.size() >= 3) {
	cat_id = path_params[2];
	validate_catid_throw(cat_id);
    }
    if (path_params.size() >= 4) {
	parent_id = path_params[3];
	validate_catid_throw(parent_id);
    }

    return new CollDeleteCategoryHandler(coll_name, taxonomy_name, cat_id,
					 parent_id);
}

Queue::QueueState
CollDeleteCategoryHandler::enqueue(ConnectionInfo &,
				   const Json::Value &)
{
    auto_ptr<ProcessingTask> task;

    if (cat_id.empty()) {
	task = auto_ptr<ProcessingTask>(
	    new ProcessingCollDeleteTaxonomyTask(taxonomy_name));
    } else if (parent_id.empty()) {
	task = auto_ptr<ProcessingTask>(
	    new ProcessingCollDeleteCategoryTask(taxonomy_name, cat_id));
    } else {
	task = auto_ptr<ProcessingTask>(
	    new ProcessingCollDeleteCategoryParentTask(taxonomy_name, cat_id,
						       parent_id));
    }

    return taskman->queue_processing(coll_name, task.release(), true);
}
