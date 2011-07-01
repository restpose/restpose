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

#include "realtime.h"
#include "safeuuid.h"

using namespace RestPose;
using namespace std;

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
IndexingErrorLog::append_error(const string & msg,
			       const string & doc_type,
			       const string & doc_id)
{
    if (errors.size() < max_errors) {
	errors.push_back(IndexingError(msg, doc_type, doc_id));
    }
    total_errors++;
}

Json::Value &
IndexingErrorLog::to_json(Json::Value & result) const
{
    result["total_errors"] = total_errors;
    Json::Value & errors_obj = result["errors"] = Json::arrayValue;
    for (vector<IndexingError>::const_iterator i = errors.begin();
	 i != errors.end(); ++i) {
	i->to_json(errors_obj.append(Json::objectValue));
    }
    return result;
}

CheckPoint::CheckPoint()
	: errors(NULL),
	  last_touched(RealTime::now()),
	  reached(false)
{}

CheckPoint::~CheckPoint()
{
    delete errors;
}

void
CheckPoint::set_reached(IndexingErrorLog * errors_)
{
    delete errors;
    errors = errors_;
    reached = true;
    last_touched = RealTime::now();
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
    last_touched = RealTime::now();
    return result;
}

double
CheckPoint::seconds_since_touched() const
{
    return RealTime::now() - last_touched;
}

void
CheckPoints::expire(double max_age)
{
    map<string, CheckPoint>::iterator i = points.begin();
    while (i != points.end()) {
	if (i->second.seconds_since_touched() > max_age) {
	    points.erase(i++);
	} else {
	    ++i;
	}
    }
}

string
CheckPoints::alloc_checkpoint()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char buf[37];
    uuid_unparse_lower(uuid, buf);
    string checkid(buf, 36);
    points[checkid];
    return checkid;
}

Json::Value &
CheckPoints::ids_to_json(Json::Value & result) const
{
    result = Json::arrayValue;
    for (map<string, CheckPoint>::const_iterator i = points.begin();
	 i != points.end(); ++i) {
	result.append(i->first);
    }
    return result;
}

void
CheckPoints::set_reached(const std::string & checkid,
			 IndexingErrorLog * errors)
{
    CheckPoint & checkpoint = points[checkid];
    checkpoint.set_reached(errors);
}

Json::Value &
CheckPoints::get_state(const std::string & checkid,
		       Json::Value & result) const
{
    map<string, CheckPoint>::const_iterator i = points.find(checkid);
    if (i == points.end()) {
	result = Json::nullValue;
    } else {
	i->second.get_state(result);
    }
    return result;
}
