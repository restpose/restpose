/** @file collection.cc
 * @brief Collections, a set of documents of varying types.
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
#include "collection.h"

#include "utils.h"
#include <xapian.h>
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "jsonxapian/doctojson.h"
#include "str.h"

#include "server/task_manager.h"

#include <set>

using namespace std;
using namespace RestPose;

// The current configuration format number.
// This should be incremented whenever a change is made to the configuration
// format.
static const unsigned int CONFIG_FORMAT = 3u;

// The oldest supported configuration format number.
static const unsigned int CONFIG_FORMAT_OLDEST = 3u;

static void
check_format_number(unsigned int format)
{
    if (format < CONFIG_FORMAT_OLDEST)
	throw InvalidValueError("Configuration supplied is in too old a "
				"format: in format " + str(format) +
				" but the oldest supported is " +
				str(CONFIG_FORMAT_OLDEST));
    if (format > CONFIG_FORMAT)
	throw InvalidValueError("Configuration supplied is in too new a "
				"format: in format " + str(format) +
				" but the newest supported is " +
				str(CONFIG_FORMAT));
}

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


Collection::Collection(const string & coll_name_,
		       const string & coll_path_)
	: coll_name(coll_name_),
	  group(coll_path_)
{
}

Collection::~Collection()
{
    try {
	close();
    } catch (const Xapian::Error & e) {
	// can't safely throw in destructor.
    }

    for (map<string, Schema *>::const_iterator
	 i = types.begin(); i != types.end(); ++i) {
	delete i->second;
    }
    for (map<string, Pipe *>::const_iterator
	 i = pipes.begin(); i != pipes.end(); ++i) {
	delete i->second;
    }
    for (map<string, Categoriser *>::const_iterator
	 i = categorisers.begin(); i != categorisers.end(); ++i) {
	delete i->second;
    }
}

void
Collection::open_writable()
{
    if (!group.is_writable()) {
	group.open_writable();
	read_config();
    }
}

void
Collection::open_readonly()
{
    group.open_readonly();
    read_config();
}

const Xapian::Database &
Collection::get_db() const
{
    return group.get_db();
}

void
Collection::set_default_schema()
{
    for (map<string, Schema *>::iterator
	 i = types.begin(); i != types.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    types.clear();

    id_field = "id";
    type_field = "type";

    Schema defschema("");
    Json::Value tmp;
    defschema.from_json(json_unserialise(
"{"
"  \"patterns\": ["
"    [ \"*_text\", { \"type\": \"text\", \"prefix\": \"t*\", \"store_field\": \"*_text\", \"processor\": \"stem_en\" } ],"
"    [ \"*_date\", { \"type\": \"date\", \"slot\": \"d*\", \"store_field\": \"*_date\" } ],"
"    [ \"*_tag\", { \"type\": \"exact\", \"prefix\": \"g*\", \"store_field\": \"*_tag\" } ],"
"    [ \"*url\", { \"type\": \"exact\", \"prefix\": \"u*\", \"store_field\": \"*url\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"*\", { \"type\": \"text\", \"prefix\": \"t\", \"store_field\": \"*\", \"processor\": \"stem_en\" } ]"
"  ],"
"  \"fields\": {"
"    \"id\": { \"type\": \"id\", \"store_field\": \"id\" },"
"    \"type\": { \"type\": \"exact\", \"prefix\": \"!\", \"store_field\": \"type\" }"
"  }"
"}", tmp)); 
    defschema.to_json(default_type_config);
}

void
Collection::read_schemas_config()
{
    string schemas_config_str(group.get_metadata("_types"));
    if (!last_schemas_config.empty()) {
	if (schemas_config_str == last_schemas_config) {
	    return;
	}
    }
    last_schemas_config = schemas_config_str;

    if (schemas_config_str.empty()) {
	set_default_schema();
	set_pipe_internal("default", Pipe());
	return;
    }

    // Set the schema.
    Json::Value config_obj;
    json_unserialise(schemas_config_str, config_obj);
    schemas_config_from_json(config_obj);
}

void
Collection::schemas_config_from_json(const Json::Value & value)
{
    const Json::Value & types_obj(value["types"]);
    if (!types_obj.isNull()) {
	json_check_object(types_obj, "types definition");
	for (Json::Value::iterator i = types_obj.begin();
	     i != types_obj.end(); ++i) {
	    Schema schema(i.memberName());
	    schema.from_json(*i);
	    set_schema_internal(i.memberName(), schema);
	}
    }

    const Json::Value & deftype_obj(value["default_type"]);
    if (!deftype_obj.isNull()) {
	// Load the config into a Schema object to verify it.
	Schema schema("");
	schema.from_json(deftype_obj);
	default_type_config = deftype_obj;
    }

    const Json::Value & special_obj(value["special_fields"]);
    if (!special_obj.isNull()) {
	json_check_object(special_obj, "special_fields definition");
	id_field = json_get_string_member(special_obj, "id_field", id_field);
	type_field = json_get_string_member(special_obj, "type_field",
					    type_field);
    }
}

void
Collection::write_schemas_config()
{
    Json::Value config_obj(Json::objectValue);
    schemas_config_to_json(config_obj);
    group.set_metadata("_types", json_serialise(config_obj));
}

void
Collection::schemas_config_to_json(Json::Value & value) const
{
    Json::Value & types_obj(value["types"]);
    types_obj = Json::objectValue;
    for (map<string, Schema *>::const_iterator
	 i = types.begin(); i != types.end(); ++i) {
	Json::Value & type_obj = types_obj[i->first];
	i->second->to_json(type_obj);
    }

    Json::Value & deftype_obj(value["default_type"]);
    deftype_obj = default_type_config;

    Json::Value & special_obj(value["special_fields"]);
    special_obj["id_field"] = id_field;
    special_obj["type_field"] = type_field;
}

void
Collection::read_pipes_config()
{
    string pipes_config_str(group.get_metadata("_pipes"));
    if (!last_pipes_config.empty()) {
	if (pipes_config_str == last_pipes_config) {
	    return;
	}
    }
    last_pipes_config = pipes_config_str;

    // Clear the pipes and set built-in defaults.
    for (map<string, Pipe *>::iterator
	 i = pipes.begin(); i != pipes.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    pipes.clear();
    set_pipe_internal("default", Pipe());

    if (pipes_config_str.empty()) {
	return;
    }

    Json::Value config_obj;
    json_unserialise(pipes_config_str, config_obj);

    pipes_config_from_json(config_obj.get("pipes", Json::nullValue));
}

void
Collection::pipes_config_from_json(const Json::Value & value)
{
    if (!value.isNull()) {
	json_check_object(value, "pipes");
	for (Json::Value::iterator i = value.begin();
	     i != value.end(); ++i) {
	    Pipe pipe;
	    pipe.from_json(*i);
	    set_pipe_internal(i.memberName(), pipe);
	}
    }

}

void
Collection::write_pipes_config()
{
    Json::Value config_obj(Json::objectValue);
    pipes_config_to_json(config_obj["pipes"]);
    group.set_metadata("_pipes", json_serialise(config_obj));
}

void
Collection::pipes_config_to_json(Json::Value & value) const
{
    value = Json::objectValue;

    for (map<string, Pipe *>::const_iterator
	 i = pipes.begin(); i != pipes.end(); ++i) {
	Json::Value & pipe_obj = value[i->first];
	i->second->to_json(pipe_obj);
    }
}

void
Collection::read_categorisers_config()
{
    string categorisers_config_str(group.get_metadata("_categorisers"));
    if (!last_categorisers_config.empty()) {
	if (categorisers_config_str == last_categorisers_config) {
	    return;
	}
    }
    last_categorisers_config = categorisers_config_str;

    // Clear the categorisers and set built-in defaults.
    for (map<string, Categoriser *>::iterator
	 i = categorisers.begin(); i != categorisers.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    categorisers.clear();

    if (categorisers_config_str.empty()) {
	return;
    }

    Json::Value config_obj;
    json_unserialise(categorisers_config_str, config_obj);

    categorisers_config_from_json(config_obj.get("categorisers", Json::nullValue));
}

void
Collection::categorisers_config_from_json(const Json::Value & value)
{
    if (!value.isNull()) {
	json_check_object(value, "categorisers");
	for (Json::Value::iterator i = value.begin();
	     i != value.end(); ++i) {
	    Categoriser categoriser;
	    categoriser.from_json(*i);
	    set_categoriser_internal(i.memberName(), categoriser);
	}
    }

}

void
Collection::write_categorisers_config()
{
    Json::Value config_obj(Json::objectValue);
    categorisers_config_to_json(config_obj["categorisers"]);
    group.set_metadata("_categorisers", json_serialise(config_obj));
}

void
Collection::categorisers_config_to_json(Json::Value & value) const
{
    value = Json::objectValue;

    for (map<string, Categoriser *>::const_iterator
	 i = categorisers.begin(); i != categorisers.end(); ++i) {
	Json::Value & categoriser_obj = value[i->first];
	i->second->to_json(categoriser_obj);
    }
}

void
Collection::read_config()
{
    try {
	read_schemas_config();
	read_pipes_config();
	read_categorisers_config();
    } catch(...) {
	group.close();
	throw;
    }
}

const Schema &
Collection::get_schema(const string & type) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    map<string, Schema *>::const_iterator i = types.find(type);
    if (i == types.end()) {
	throw InvalidValueError("No schema of requested type found");
    }
    return *(i->second);
}

void
Collection::set_schema(const string & type,
		       const Schema & schema)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set schema");
    }
    set_schema_internal(type, schema);
    write_schemas_config();
}

void
Collection::set_schema_internal(const string & type,
				const Schema & schema)
{
    Schema * schemaptr;
    map<string, Schema *>::iterator i = types.find(type);

    if (i == types.end()) {
	pair<map<string, Schema *>::iterator, bool> ret;
	pair<string, Schema *> item(type, NULL);
	ret = types.insert(item);
	schemaptr = new Schema(type);
	ret.first->second = schemaptr;
    } else {
	schemaptr = i->second;
    }

    schemaptr->merge_from(schema);
}


const Pipe &
Collection::get_pipe(const string & pipe_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    map<string, Pipe *>::const_iterator i = pipes.find(pipe_name);
    if (i == pipes.end()) {
	throw InvalidValueError("No pipe of requested name found");
    }
    return *(i->second);
}

void
Collection::set_pipe(const string & pipe_name,
		     const Pipe & pipe)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set pipe");
    }
    set_pipe_internal(pipe_name, pipe);
    write_pipes_config();
}

void
Collection::set_pipe_internal(const string & pipe_name,
			      const Pipe & pipe)
{
    Pipe * pipeptr;
    map<string, Pipe *>::iterator i = pipes.find(pipe_name);

    if (i == pipes.end()) {
	pair<map<string, Pipe *>::iterator, bool> ret;
	pair<string, Pipe *> item(pipe_name, NULL);
	ret = pipes.insert(item);
	pipeptr = new Pipe();
	ret.first->second = pipeptr;
    } else {
	pipeptr = i->second;
    }
    *pipeptr = pipe;
}


const Categoriser &
Collection::get_categoriser(const string & categoriser_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    map<string, Categoriser *>::const_iterator i = categorisers.find(categoriser_name);
    if (i == categorisers.end()) {
	throw InvalidValueError("No categoriser of requested name found");
    }
    return *(i->second);
}

void
Collection::set_categoriser(const string & categoriser_name,
			    const Categoriser & categoriser)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set categoriser");
    }
    set_categoriser_internal(categoriser_name, categoriser);
    write_categorisers_config();
}

void
Collection::set_categoriser_internal(const string & categoriser_name,
				     const Categoriser & categoriser)
{
    Categoriser * categoriserptr;
    map<string, Categoriser *>::iterator i = categorisers.find(categoriser_name);

    if (i == categorisers.end()) {
	pair<map<string, Categoriser *>::iterator, bool> ret;
	pair<string, Categoriser *> item(categoriser_name, NULL);
	ret = categorisers.insert(item);
	categoriserptr = new Categoriser();
	ret.first->second = categoriserptr;
    } else {
	categoriserptr = i->second;
    }
    *categoriserptr = categoriser;
}



Json::Value &
Collection::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    schemas_config_to_json(value);
    if (!pipes.empty()) {
	pipes_config_to_json(value["pipes"]);
    }
    if (!categorisers.empty()) {
	categorisers_config_to_json(value["categorisers"]);
    }
    value["format"] = CONFIG_FORMAT;
    return value;
}

void
Collection::from_json(const Json::Value & value)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set config");
    }

    json_check_object(value, "collection configuration");

    check_format_number(json_get_uint64_member(value, "format",
					       Json::Value::maxInt));

    schemas_config_from_json(value);
    pipes_config_from_json(value.get("pipes", Json::nullValue));
    categorisers_config_from_json(value.get("categorisers", Json::nullValue));

    write_schemas_config();
    write_pipes_config();
    write_categorisers_config();
}

Json::Value &
Collection::categorise(const string & categoriser_name,
		       const string & text,
		       Json::Value & result) const
{
    result = Json::arrayValue;
    const Categoriser & cat = get_categoriser(categoriser_name);
    vector<string> result_vec;
    cat.categorise(text, result_vec);
    for (vector<string>::const_iterator i = result_vec.begin();
	 i != result_vec.end(); ++i) {
	result.append(*i);
    }
    return result;
}

void
Collection::send_to_pipe(TaskManager * taskman,
			 const string & pipe_name,
			 const Json::Value & obj)
{
    //printf("pipe %s: %s\n", pipe_name.c_str(), json_serialise(obj).c_str());
    // FIXME - this method just uses a recursive implementation, and doesn't
    // check that a document isn't being passed to a pipe it's already come
    // from (which would almost certainly be a mistake), so it's easy for it
    // to overflow the stack, and kill the process.  Would be better to do a
    // non-recursive implementation, and keep track of pipes which have
    // already been used to ensure that no loops happen.

    if (pipe_name.empty()) {
	string idterm;
	// FIXME - remove hardcoded "default" here - pipes should have a way to
	// say what type the output document is.
	Xapian::Document xdoc = process_doc("default", obj, idterm);
	taskman->queue_index_processed_doc(get_name(), xdoc, idterm);
	return;
    }
    const Pipe & pipe = get_pipe(pipe_name);
    vector<Mapping>::const_iterator i;
    //printf("mappings: %d\n", pipe.mappings.size());
    for (i = pipe.mappings.begin(); i != pipe.mappings.end(); ++i) {
	Json::Value output;
	bool applied = i->apply(*this, obj, output);
	//printf("applied: %s\n", applied ? "true" : "false");
	if (applied) {
	    send_to_pipe(taskman, pipe.target, output);
	    if (!pipe.apply_all) {
		break;
	    }
	}
    }
}

void
Collection::add_doc(const Json::Value & doc_obj)
{
    string type("default"); // FIXME - allow this to be something other than default.

    string idterm;
    const Schema & schema = get_schema(type);
    Xapian::Document doc(schema.process(doc_obj, idterm));
    raw_update_doc(doc, idterm);
}

Xapian::Document
Collection::process_doc(const string & type,
			const Json::Value & doc_obj,
			string & idterm)
{
    const Schema & schema = get_schema(type);
    return schema.process(doc_obj, idterm);
}

void
Collection::raw_update_doc(const Xapian::Document & doc,
			   const string & idterm)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to add document");
    }
    group.add_doc(doc, idterm);
}

void
Collection::raw_delete_doc(const string & idterm)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to delete document");
    }
    group.delete_doc(idterm);
}

void
Collection::commit()
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to commit");
    }
    group.sync();
}

uint64_t
Collection::doc_count() const
{
    return get_db().get_doccount();
}

void
Collection::perform_search(const Json::Value & search,
			   Json::Value & results) const
{
    string type("default"); // FIXME - allow this to be something other than default.
    const Schema & schema = get_schema(type);
    schema.perform_search(get_db(), search, results);
}

void
Collection::get_doc_fields(const Xapian::Document & doc,
			   const Json::Value & fieldlist,
			   Json::Value & result) const
{
    string type("default"); // FIXME - allow this to be something other than default.
    const Schema & schema = get_schema(type);
    schema.display_doc(doc, fieldlist, result);
}

void
Collection::get_document(const string & type,
			 const string & docid,
			 Json::Value & result) const
{
    bool found;
    string idterm = "\t" + type + "\t" + docid;
    Xapian::Document doc = group.get_document(idterm, found);
    if (found) {
	doc_to_json(doc, result);
    } else {
	result = Json::nullValue;
    }
}
