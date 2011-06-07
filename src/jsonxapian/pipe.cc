/** @file pipe.cc
 * @brief Input pipelines
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
#include "pipe.h"

#include "utils.h"
#include "utils/jsonutils.h"

using namespace std;
using namespace RestPose;

Json::Value &
Pipe::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    if (!mappings.empty()) {
	Json::Value & mappings_obj = value["mappings"] = Json::arrayValue;
	for (vector<Mapping>::const_iterator i = mappings.begin();
	     i != mappings.end(); ++i) {
	    mappings_obj.append(Json::objectValue);
	    i->to_json(mappings_obj[mappings_obj.size() - 1]);
	}
    }
    if (apply_all) {
	value["apply_all"] = true;
    }
    if (!target.empty()) {
	value["target"] = target;
    }
    return value;
}

void
Pipe::from_json(const Json::Value & value)
{
    mappings.clear();
    apply_all = false;
    target.resize(0);

    json_check_object(value, "pipe definition");
    Json::Value tmp;

    // Read mappings
    tmp = value.get("mappings", Json::nullValue);
    if (!tmp.isNull()) {
	json_check_array(tmp, "pipe mappings");
	for (Json::Value::iterator i = tmp.begin(); i != tmp.end(); ++i) {
	    mappings.resize(mappings.size() + 1);
	    mappings.back().from_json(*i);
	}
    }

    // Read apply_all
    tmp = value.get("apply_all", Json::nullValue);
    if (!tmp.isNull()) {
	json_check_bool(tmp, "pipe apply_all property");
	apply_all = tmp.asBool();
    }

    // Read target
    tmp = value.get("target", Json::nullValue);
    if (!tmp.isNull()) {
	json_check_string(tmp, "pipe target property");
	target = tmp.asString();
    }
}
