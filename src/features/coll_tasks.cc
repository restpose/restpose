/** @file coll_tasks.cc
 * @brief Tasks related to collections.
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
#include "features/coll_tasks.h"

#include "httpserver/response.h"
#include "jsonxapian/collection_pool.h"
#include "logger/logger.h"
#include "server/task_manager.h"
#include <string>
#include <vector>

using namespace std;

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
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
CollGetConfigTask::perform(RestPose::Collection * collection)
{
    Json::Value result;
    collection->to_json(result);
    resulthandle.response().set(result, 200);
    resulthandle.set_ready();
}

void
CollSetConfigTask::perform_task(RestPose::Collection & collection,
				TaskManager *)
{
    collection.from_json(config);
}

void
CollSetConfigTask::info(string & description, string & doc_type,
			string & doc_id) const
{
    description = "Setting collection config";
    doc_type.resize(0);
    doc_id.resize(0);
}

IndexingTask *
CollSetConfigTask::clone() const
{
    return new CollSetConfigTask(config);
}
