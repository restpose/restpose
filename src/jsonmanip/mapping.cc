/** @file mapping.cc
 * @brief Mappings applied to JSON documents.
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
#include "jsonmanip/mapping.h"
#include "jsonxapian/collconfig.h"

#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

MappingActions &
MappingActions::find(const Json::Value & path, size_t path_offset)
{
    JSONPathComponent comp;
    if (!path.isArray()) {
	comp.set(path);

	return children[comp];
    }
    comp.set(path[static_cast<Json::ArrayIndex>(path_offset)]);
    MappingActions & subaction = children[comp];
    if (path.size() == path_offset + 1) {
	return subaction;
    } else {
	return subaction.find(path, path_offset + 1);
    }
}

void
Mapping::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    if (!when.is_null()) {
	Json::Value tmp;
	when.to_json(tmp);
	value["when"] = tmp;
    }
    switch (default_action) {
	case DISCARD:
	    value["default"] = "discard";
	    break;
	case PRESERVE_TOP:
	    // Don't need to store a setting, because it's set to the default.
	    // value["default"] = "preserve_top";
	    break;
    }
    
    Json::Value path = Json::arrayValue;
    std::vector<std::pair<ActionMap::const_iterator,
			  ActionMap::const_iterator> > stack;

    stack.push_back(std::make_pair(mappings.children.begin(),
				   mappings.children.end()));
    path.append(Json::nullValue);

    value["map"] = Json::arrayValue;
    Json::Value & paths = value["map"];
    while (true) {
	while (stack.back().first == stack.back().second) {
	    stack.pop_back();
	    path.resize(path.size() - 1);
	    if (stack.empty()) {
		break;
	    }
	}
	if (stack.empty()) {
	    break;
	}
	
	ActionMap::const_iterator & pos = stack.back().first;

	path[path.size() - 1] = pos->first.get();

	// Add direct targets at this level.
	for (std::vector<MappingTarget>::const_iterator
	     i = pos->second.target_fields.begin();
	     i != pos->second.target_fields.end(); ++i) {
	    Json::Value & obj = paths.append(Json::objectValue);
	    obj["to"] = i->field;
	    obj["from"] = path;
	    if (!i->categoriser.empty()) {
		obj["categoriser"] = i->categoriser;
	    }
	}

	stack.push_back(std::make_pair(pos->second.children.begin(),
				       pos->second.children.end()));
	path.append(Json::nullValue);

	++(stack[stack.size() - 2].first);

    }
    if (paths.size() == 0) {
	value.removeMember("map");
    }
}

void
Mapping::from_json(const Json::Value & value)
{
    json_check_object(value, "mapping");

    // Read the "when" conditional.
    when.from_json(value.get("when", Json::nullValue));

    // Read the "map" actions.
    mappings.children.clear();
    mappings.target_fields.clear();
    const Json::Value & mapval = value["map"];
    if (!mapval.isNull()) {
	json_check_array(mapval, "map property in mapping");
	for (Json::Value::const_iterator i = mapval.begin();
	     i != mapval.end(); ++i) {
	    json_check_object(*i, "mapping definition");
	    const Json::Value & from = (*i)["from"];
	    const Json::Value & to = (*i)["to"];
	    json_check_string(to, "mapping target fieldname");
	    MappingActions & actions = mappings.find(from);
	    actions.target_fields.push_back(MappingTarget(to.asString()));

	    const Json::Value & categoriser = (*i)["categoriser"];
	    if (!categoriser.isNull()) {
		json_check_string(categoriser, "mapping target categoriser");
		actions.target_fields.back().categoriser = categoriser.asString();
	    }
	}
    }

    // Read the "default" action.
    Json::Value defval = value.get("default", Json::nullValue);
    if (defval == "preserve_top" || defval == Json::nullValue) {
	default_action = PRESERVE_TOP;
    } else if (defval == "discard") {
	default_action = DISCARD;
    } else {
	throw InvalidValueError("Invalid value for \"default\" parameter in "
				"mapping");
    }
}

static void
append_field(Json::Value & output, const std::string & key,
	     const Json::Value & value)
{
    //printf("append_field(%s, %s)\n", key.c_str(), json_serialise(value).c_str());
    Json::Value & oldval = output[key];
    if (!oldval.isArray()) {
	// Value should be null; we'll just override it.
	output[key] = Json::arrayValue;
	oldval = output[key];
    }

    if (value.isArray()) {
	if (value.size() == 0) {
	    oldval = Json::arrayValue;
	} else {
	    for (Json::Value::const_iterator i = value.begin();
		 i != value.end(); ++i) {
		oldval.append(*i);
	    }
	}
    } else {
	oldval.append(value);
    }
}

bool
Mapping::handle(const CollectionConfig & collconfig,
		const std::vector<const MappingActions *> & stack,
		const JSONWalker::Event & event,
		Json::Value & output) const
{
    if (stack.empty()) {
	return false;
    }

    const MappingActions * actions = stack.back();
    if (actions == NULL) {
	return false;
    }

    // Look up action for current final path component.
    ActionMap::const_iterator subaction
	    = actions->children.find(event.component);
    if (subaction == actions->children.end()) {
	return false;
    }
    actions = &(subaction->second);

    bool handled = false;
    for (std::vector<MappingTarget>::const_iterator
	 i = actions->target_fields.begin();
	 i != actions->target_fields.end(); ++i) {
	if (i->categoriser.empty()) {
	    append_field(output, i->field, *(event.value));
	} else {
	    const Json::Value & fieldval = *(event.value);
	    std::string text;
	    if (!fieldval.isArray()) {
		// FIXME - log invalid value.
		//printf("invalid value for categoriser: not an array, was %s\n", json_serialise(fieldval).c_str());
		if (fieldval.isString()) {
		    text = fieldval.asString();
		}
	    } else {
		for (Json::Value::const_iterator j = fieldval.begin();
		     j != fieldval.end(); ++j) {
		    if (!(*j).isString()) {
			//printf("invalid value in array for categoriser: not a string\n");
			// FIXME - log invalid value.
		    } else {
			text.append((*j).asString());
			text.append(" ");
		    }
		}
	    }
	    if (text.empty()) {
		append_field(output, i->field, Json::StaticString(""));
	    } else {
		Json::Value category;
		collconfig.categorise(i->categoriser, text, category);
		append_field(output, i->field, category);
	    }
	}
	handled = true;
    }
    return handled;
}

void
Mapping::handle_default(const std::vector<const MappingActions *> & stack,
			const JSONWalker::Event & event,
			Json::Value & output) const
{
    if (default_action == PRESERVE_TOP) {
	if (stack.size() == 1) {
	    append_field(output, event.component.key, *(event.value));
	}
    }
}

bool
Mapping::apply(const CollectionConfig & collconfig,
	       const Json::Value & input,
	       Json::Value & output) const
{
    json_check_object(input, "input to mapping");

    if (!when.is_null() && !when.test(input)) {
	output = Json::nullValue;
	return false;
    }
    output = Json::objectValue;

    JSONWalker walker(input);
    std::vector<const MappingActions *> stack;
    JSONWalker::Event event;

    /** Whether the current top-level item has been handled. */
    bool handled_top = false;
    while (walker.next(event)) {
	switch (event.type) {
	    case JSONWalker::Event::START:
		// Handle the event itself.
		if (stack.size() == 1) {
		    handled_top = false;
		}
		if (handle(collconfig, stack, event, output)) {
		    handled_top = true;
		}

		// Now add to the stack.
		if (stack.empty()) {
		    stack.push_back(&mappings);
		} else {
		    if (stack.back() == NULL) {
			stack.push_back(NULL);
		    } else {
			ActionMap::const_iterator
				i = stack.back()->children.find(event.component);
			if (i != stack.back()->children.end()) {
			    stack.push_back(&(i->second));
			} else {
			    stack.push_back(NULL);
			}
		    }
		}

		break;

	    case JSONWalker::Event::LEAF:
		if (stack.size() == 1) {
		    handled_top = false;
		}
		if (handle(collconfig, stack, event, output)) {
		    handled_top = true;
		}
		if (!handled_top) {
		    handle_default(stack, event, output);
		}
		break;

	    case JSONWalker::Event::END:
		stack.pop_back();
		if (!handled_top) {
		    handle_default(stack, event, output);
		}
		break;
	}
    }

    return true;
}
