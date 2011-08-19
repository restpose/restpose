/** @file jsonpath.cc
 * @brief Path handling and manipultion for JSON documents.
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
#include "jsonmanip/conditionals.h"

#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

void
JSONPathComponent::set(const Json::Value & value)
{
    if (value.isString()) {
	set_string(value.asString());
    } else if (value.isNumeric() && value >= 0) {
	set_index(value.asUInt());
    } else {
	throw InvalidValueError("Path components must only be set to strings or integers");
    }
}

void
JSONPath::to_json(Json::Value & value) const
{
    value = Json::arrayValue;
    for (std::vector<JSONPathComponent>::const_iterator i = path.begin();
	 i != path.end(); ++i) {
	switch (i->type) {
	    case JSONPathComponent::JSONPATH_KEY:
		value.append(i->key);
		break;
	    case JSONPathComponent::JSONPATH_INDEX:
		value.append(i->index);
		break;
	    default:
		throw InvalidValueError("Invalid component type in JSON path");
	}
    }
}

void
JSONPath::from_json(const Json::Value & value)
{
    json_check_array(value, "path");
    path.clear();
    for (Json::Value::const_iterator it = value.begin();
	 it != value.end(); ++it) {
	if ((*it).isIntegral()) {
	    if (*it < Json::Value::Int(0)) {
		throw InvalidValueError("Negative index found in JSON path");
	    }
	    append_index((*it).asUInt());
	} else if ((*it).isString()) {
	    append_string((*it).asString());
	} else {
	    throw InvalidValueError("Item in JSON path found which is neither a string nor an integer");
	}
    }
}


JSONWalker::Level::Level(const Json::Value & new_value)
	: parent_value(&new_value),
	  pos(new_value.begin()),
	  end(new_value.end()),
	  started(false)
{
    if (new_value.isObject()) {
	type = JSONPathComponent::JSONPATH_KEY;
    } else {
	type = JSONPathComponent::JSONPATH_INDEX;
    }
}

const Json::Value &
JSONWalker::Level::current() const
{
    return *pos;
}

void
JSONWalker::Level::set_event(JSONWalker::Event & event)
{
    switch (type) {
	case JSONPathComponent::JSONPATH_KEY:
	    event.component.set_string(pos.key().asString());
	    break;
	case JSONPathComponent::JSONPATH_INDEX:
	    event.component.set_index(pos.index());
	    break;
    }
    event.value = &(*pos);
}

void
JSONWalker::Level::set_event_parent(JSONWalker::Event & event)
{
    event.value = parent_value;
    event.component = parent_component;
}

void
JSONWalker::Level::next(JSONWalker::Event & event)
{
    if (!started) {
	event.type = JSONWalker::Event::START;
	started = true;
    } else if (pos == end) {
	event.type = JSONWalker::Event::END;
    } else {
	event.type = JSONWalker::Event::LEAF;
	set_event(event);
	++pos;
    }
}

JSONWalker::JSONWalker(const Json::Value & value_)
	: value(value_)
{
    if (!value.isObject() && !value.isArray()) {
	throw InvalidValueError(std::string("JSON value passed to walker was "
					    "neither an object nor an array"));
    }
    stack.push_back(Level(value));
    stack.back().parent_component.set_index(0);
}

bool
JSONWalker::next(Event & event)
{
    if (stack.empty()) {
	return false;
    }

    stack.back().next(event);
    switch (event.type) {
	case Event::LEAF:
	    if (!event.value->isNull() && (event.value->isObject() || event.value->isArray())) {
		/* Start iterating the child instead. */
		stack.push_back(Level(*(event.value)));
		stack.back().parent_component = event.component;
		stack.back().next(event);
	    }
	    break;
	case Event::START:
	    stack.back().set_event_parent(event);
	    break;
	case Event::END:
	    stack.back().set_event_parent(event);
	    stack.pop_back();
	    break;
    }
    return true;
}
