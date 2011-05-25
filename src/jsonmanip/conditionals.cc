/** @file conditionals.cc
 * @brief Conditional expressions applied to JSON documents.
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


ConditionalClause::~ConditionalClause()
{}

ConditionalClause *
ConditionalClause::from_json(const Json::Value & value)
{
    // Must be an object.
    json_check_object(value, "conditional clause");

    Json::Value::Members members = value.getMemberNames();
    if (members.size() == 1) {
	const Json::Value & obj = value[members[0]];
	if (members[0] == "exists") {
	    return new ConditionalClauseExists(obj);
	} else if (members[0] == "get") {
	    return new ConditionalClauseGet(obj);
	} else if (members[0] == "literal") {
	    return new ConditionalClauseLiteral(obj);
	} else if (members[0] == "equals") {
	    return new ConditionalClauseEquals(obj);
	}
    }
    throw InvalidValueError("Unsupported conditional clause format");
}


ConditionalClauseExists::ConditionalClauseExists(const Json::Value & value)
	: ConditionalClause("exists")
{
    json_check_array(value, "exists member in ConditionalClauseExists");
    path.from_json(value);
}

void
ConditionalClauseExists::to_json(Json::Value & value) const
{
    path.to_json(value);
}

Json::Value
ConditionalClauseExists::apply(const Json::Value & document) const
{
    std::vector<JSONPathComponent>::const_iterator i;
    const Json::Value * current = &document;
    for (i = path.path.begin(); i != path.path.end(); ++i) {
	switch (i->type) {
	    case JSONPathComponent::JSONPATH_KEY:
		if (!current->isObject()) {
		    return false;
		}
		if (!current->isMember(i->key)) {
		    return false;
		}
		current = &(*current)[i->key];
		break;
	    case JSONPathComponent::JSONPATH_INDEX:
		if (!current->isArray()) {
		    return false;
		}
		if (!current->isValidIndex(i->index)) {
		    return false;
		}
		current = &(*current)[i->index];
		break;
	    default:
		// Shouldn't happen - bad path component if it does,
		// but this means we definitely can't find it.
		return false;
	}
    }
    return true;
}


ConditionalClauseGet::ConditionalClauseGet(const Json::Value & value)
	: ConditionalClause("get")
{
    json_check_array(value, "get member in ConditionalClauseGet");
    path.from_json(value);
}

void
ConditionalClauseGet::to_json(Json::Value & value) const
{
    path.to_json(value);
}

Json::Value
ConditionalClauseGet::apply(const Json::Value & document) const
{
    std::vector<JSONPathComponent>::const_iterator i;
    const Json::Value * current = &document;
    for (i = path.path.begin(); i != path.path.end(); ++i) {
	switch (i->type) {
	    case JSONPathComponent::JSONPATH_KEY:
		if (!current->isObject()) {
		    return Json::nullValue;
		}
		if (!current->isMember(i->key)) {
		    return Json::nullValue;
		}
		current = &(*current)[i->key];
		break;
	    case JSONPathComponent::JSONPATH_INDEX:
		if (!current->isArray()) {
		    return Json::nullValue;
		}
		if (!current->isValidIndex(i->index)) {
		    return Json::nullValue;
		}
		current = &(*current)[i->index];
		break;
	    default:
		// Shouldn't happen - bad path component if it does,
		// but this means we definitely can't find it.
		return Json::nullValue;
	}
    }
    return *current;
}


ConditionalClauseLiteral::ConditionalClauseLiteral(const Json::Value & value_)
	: ConditionalClause("literal")
{
    value = value_;
}

void
ConditionalClauseLiteral::to_json(Json::Value & value_) const
{
    value_ = value;
}

Json::Value
ConditionalClauseLiteral::apply(const Json::Value &) const
{
    return value;
}


ConditionalClauseEquals::ConditionalClauseEquals(const Json::Value & value)
	: ConditionalClause("equals")
{
    json_check_array(value, "equals member in ConditionalClauseEquals");
    for (Json::Value::const_iterator i = value.begin();
	 i != value.end(); ++i) {
	children.push_back(NULL);
	children.back() = ConditionalClause::from_json(*i);
    }
}

ConditionalClauseEquals::~ConditionalClauseEquals()
{
    for (std::vector<ConditionalClause *>::const_iterator
	 i = children.begin(); i != children.end(); ++i) {
	delete(*i);
    }
}

void
ConditionalClauseEquals::to_json(Json::Value & value) const
{
    value = Json::arrayValue;
    for (std::vector<ConditionalClause *>::const_iterator
	 i = children.begin(); i != children.end(); ++i) {
	value.append(Json::objectValue);
	(*i)->to_json(value[value.size() - 1][(*i)->name]);
    }
}

Json::Value
ConditionalClauseEquals::apply(const Json::Value & document) const
{
    if (children.size() <= 1) {
	// 0 or 1 children - trivially equal.
	return true;
    }

    std::vector<ConditionalClause *>::const_iterator i = children.begin();
    Json::Value first = (*i)->apply(document);
    ++i;

    while (i != children.end()) {
	Json::Value next = (*i)->apply(document);

	if (next != first) {
	    return false;
	}

	++i;
    }

    return true;
}


Conditional::Conditional(const Conditional & other)
	: clause(NULL)
{
    // FIXME - copy directly, rather than going through JSON.
    Json::Value value;
    other.to_json(value);
    from_json(value);
}

Conditional &
Conditional::operator=(const Conditional & other)
{
    Json::Value value;
    other.to_json(value);
    from_json(value);
    return *this;
}

void
Conditional::to_json(Json::Value & value) const
{
    if (clause == NULL) {
	value = Json::nullValue;
    } else {
	value = Json::objectValue;
	clause->to_json(value[clause->name]);
    }
}

void
Conditional::from_json(const Json::Value & value)
{
    if (clause) {
	delete clause;
	clause = NULL;
    }
    if (!value.isNull()) {
	clause = ConditionalClause::from_json(value);
    }
}

bool
Conditional::test(const Json::Value & value) const
{
    if (clause == NULL) {
	throw InvalidValueError("Attempt to test a null conditional");
    } else {
	return clause->apply(value).asBool();
    }
}
