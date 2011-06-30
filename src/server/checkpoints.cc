/** @file checkpoints.cc
 * @brief Checkpoints, for monitoring indexing progress.
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
#include "server/checkpoints.h"

Json::Value &
IndexingError::to_json(Json::Value & result) const
{
    result = Json::objectValue;
    result["msg"] = msg;
    if (!doc_type.empty()) {
	result["doc_type"] = doc_type;
    }
    if (!doc_id.empty()) {
	result["doc_id"] = doc_id;
    }
    return result;
}

void
IndexingErrorLog::append_error(const std::string & msg,
			       const std::string & doc_type,
			       const std::string & doc_id)
{
    if (errors.size() < max_errors) {
	errors.push_back(IndexingError(msg, doc_type, doc_id));
    }
}

Json::Value &
IndexingErrorLog::to_json(Json::Value & result) const
{
    result["total_errors"] = total_errors;
    Json::Value & errors_obj = result["errors"] = Json::arrayValue;
    for (std::vector<IndexingError>::const_iterator i = errors.begin();
	 i != errors.end(); ++i) {
	i->to_json(errors_obj.append(Json::objectValue));
    }
    return result;
}

CheckPoint::CheckPoint()
	: errors(NULL),
	  reached(false)
{}

void
CheckPoint::set_reached(IndexingErrorLog * errors_)
{
    delete errors;
    errors = errors_;
    reached = true;
}

Json::Value &
CheckPoint::get_state(Json::Value & result) const
{
    result = Json::objectValue;
    if (reached) {
	result["reached"] = true;
	if (errors) {
	    errors->to_json(result);
	} else {
	    result["total_errors"] = 0;
	    result["errors"] = Json::arrayValue;
	}
    } else {
	result["reached"] = false;
    }
    return result;
}
