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

#include "logger/logger.h"
#include <memory>
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


CheckPoints::~CheckPoints()
{
    for (map<string, CheckPoint *>::iterator
	 i = points.begin(); i != points.end(); ++i) {
	delete i->second;
    }
}

void
CheckPoints::expire(double max_age)
{
    map<string, CheckPoint *>::iterator i = points.begin();
    while (i != points.end()) {
	if (i->second == NULL) {
	    points.erase(i++);
	} else if (i->second->seconds_since_touched() >= max_age) {
	    LOG_INFO("expiring old checkpoint: " + i->first);
	    std::auto_ptr<CheckPoint> oldpoint(i->second);
	    points.erase(i++);
	} else {
	    ++i;
	}
    }
}

void
CheckPoints::publish_checkpoint(const string & checkid)
{
    CheckPoint * & cp = points[checkid];
    if (cp == NULL) {
	// cp being non-NULL happens if the checkpoint has already been
	// reached (so was created by set_reached()).
	//
	// It could also theoretically happen if the uuid generated wasn't
	// unique, but we'll ignore this possibility since it's very unlikely,
	// and would simply result in the checkpoint being marked as reached
	// early (for one of the two checkpoints sharing the id).
	cp = new CheckPoint;
    }
}

Json::Value &
CheckPoints::ids_to_json(Json::Value & result) const
{
    result = Json::arrayValue;
    for (map<string, CheckPoint *>::const_iterator i = points.begin();
	 i != points.end(); ++i) {
	if (i->second != NULL) {
	    result.append(i->first);
	}
    }
    return result;
}

void
CheckPoints::set_reached(const string & checkid,
			 IndexingErrorLog * errors)
{
    auto_ptr<IndexingErrorLog> errorsptr(errors);
    CheckPoint * & cp = points[checkid];
    if (cp == NULL) {
	cp = new CheckPoint;
    }
    cp->set_reached(errorsptr.release());
}

Json::Value &
CheckPoints::get_state(const string & checkid,
		       Json::Value & result) const
{
    map<string, CheckPoint *>::const_iterator i = points.find(checkid);
    if (i == points.end() || i->second == NULL) {
	result = Json::nullValue;
    } else {
	i->second->get_state(result);
    }
    return result;
}


CheckPointManager::~CheckPointManager()
{
    for (map<string, IndexingErrorLog *>::iterator
	 i = recent_errors.begin(); i != recent_errors.end(); ++i) {
	delete i->second;
    }
    for (map<string, CheckPoints *>::iterator
	 i = checkpoints.begin(); i != checkpoints.end(); ++i) {
	delete i->second;
    }
}

void
CheckPointManager::append_error(const string & coll_name,
				const string & msg,
				const string & doc_type,
				const string & doc_id)
{
    ContextLocker lock(mutex);
    IndexingErrorLog * & log = recent_errors[coll_name];
    if (log == NULL) {
	log = new IndexingErrorLog(max_recent_errors);
    }
    log->append_error(msg, doc_type, doc_id);
}

string
CheckPointManager::alloc_checkpoint(const string &)
{
    uuid_t uuid;
    uuid_generate(uuid);
    char buf[37];
    uuid_unparse_lower(uuid, buf);
    string checkid(buf, 36);
    return checkid;
}

void
CheckPointManager::publish_checkpoint(const string & coll_name,
				      const string & checkid)
{
    ContextLocker lock(mutex);
    CheckPoints * & cps = checkpoints[coll_name];
    if (cps == NULL) {
	cps = new CheckPoints;
    } else {
	cps->expire(expiry_time);
    }
    return cps->publish_checkpoint(checkid);
}

Json::Value &
CheckPointManager::ids_to_json(const string & coll_name,
			       Json::Value & result)
{
    ContextLocker lock(mutex);
    map<string, CheckPoints *>::iterator i = checkpoints.find(coll_name);
    if (i == checkpoints.end() || i->second == NULL) {
	result = Json::arrayValue;
	return result;
    }
    i->second->expire(expiry_time);
    return i->second->ids_to_json(result);
}

void
CheckPointManager::set_reached(const string & coll_name,
			       const string & checkid)
{
    ContextLocker lock(mutex);
    CheckPoints * & cps = checkpoints[coll_name];
    if (cps == NULL) {
	cps = new CheckPoints;
    } else {
	cps->expire(expiry_time);
    }

    map<string, IndexingErrorLog *>::iterator
	    i = recent_errors.find(coll_name);
    if (i == recent_errors.end()) {
	cps->set_reached(checkid, NULL);
    } else {
	auto_ptr<IndexingErrorLog> errors(i->second);
	recent_errors.erase(i);
	cps->set_reached(checkid, errors.release());
    }
}

Json::Value &
CheckPointManager::get_state(const string & coll_name,
			     const string & checkid,
			     Json::Value & result)
{
    ContextLocker lock(mutex);
    map<string, CheckPoints *>::iterator i = checkpoints.find(coll_name);
    if (i == checkpoints.end() || i->second == NULL) {
	result = Json::nullValue;
	return result;
    }
    i->second->expire(expiry_time);
    return i->second->get_state(checkid, result);
}
