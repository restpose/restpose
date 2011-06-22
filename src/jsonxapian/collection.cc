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

#include <xapian.h>
#include "jsonxapian/doctojson.h"
#include "jsonxapian/pipe.h"
#include "logger/logger.h"
#include "str.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

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

void
CollectionConfig::clear()
{
    for (map<string, Schema *>::iterator
	 i = types.begin(); i != types.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    types.clear();

    for (map<string, Pipe *>::iterator
	 i = pipes.begin(); i != pipes.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    pipes.clear();

    for (map<string, Categoriser *>::iterator
	 i = categorisers.begin(); i != categorisers.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    categorisers.clear();

    for (map<string, CategoryHierarchy *>::iterator
	 i = categories.begin(); i != categories.end(); ++i) {
	delete i->second;
	i->second = NULL;
    }
    categories.clear();
}

void
CollectionConfig::set_default_schema()
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
"    [ \"text\", { \"type\": \"text\", \"prefix\": \"t\", \"store_field\": \"text\", \"processor\": \"stem_en\" } ],"
"    [ \"*_time\", { \"type\": \"timestamp\", \"slot\": \"d*\", \"store_field\": \"*_time\" } ],"
"    [ \"time\", { \"type\": \"timestamp\", \"slot\": \"d\", \"store_field\": \"time\" } ],"
"    [ \"*_tag\", { \"type\": \"exact\", \"prefix\": \"g*\", \"store_field\": \"*_tag\" } ],"
"    [ \"tag\", { \"type\": \"exact\", \"prefix\": \"g\", \"store_field\": \"tag\" } ],"
"    [ \"*_url\", { \"type\": \"exact\", \"prefix\": \"u*\", \"store_field\": \"*_url\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"url\", { \"type\": \"exact\", \"prefix\": \"u\", \"store_field\": \"url\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"id\", { \"type\": \"id\", \"store_field\": \"id\" } ],"
"    [ \"type\", { \"type\": \"exact\", \"prefix\": \"!\", \"store_field\": \"type\" } ],"
"    [ \"*\", { \"type\": \"text\", \"prefix\": \"t\", \"store_field\": \"*\" } ]"
"  ],"
"  \"fields\": {"
"  }"
"}", tmp)); 
    defschema.to_json(default_type_config);
}

void
CollectionConfig::schemas_config_to_json(Json::Value & value) const
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
CollectionConfig::schemas_config_from_json(const Json::Value & value)
{
    const Json::Value & types_obj(value["types"]);
    if (!types_obj.isNull()) {
	json_check_object(types_obj, "types definition");
	for (Json::Value::iterator i = types_obj.begin();
	     i != types_obj.end(); ++i) {
	    Schema schema(i.memberName());
	    schema.from_json(*i);
	    set_schema(i.memberName(), schema);
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
CollectionConfig::pipes_config_to_json(Json::Value & value) const
{
    Json::Value & pipes_obj(value["pipes"]);
    pipes_obj = Json::objectValue;
    for (map<string, Pipe *>::const_iterator
	 i = pipes.begin(); i != pipes.end(); ++i) {
	Json::Value & pipe_obj = pipes_obj[i->first];
	i->second->to_json(pipe_obj);
    }
}

void
CollectionConfig::pipes_config_from_json(const Json::Value & value)
{
    const Json::Value & pipes_obj(value["pipes"]);
    if (!pipes_obj.isNull()) {
	json_check_object(pipes_obj, "pipes definition");
	for (Json::Value::iterator i = pipes_obj.begin();
	     i != pipes_obj.end(); ++i) {
	    Pipe pipe;
	    pipe.from_json(*i);
	    set_pipe(i.memberName(), pipe);
	}
    }
}

void
CollectionConfig::categorisers_config_to_json(Json::Value & value) const
{
    Json::Value & categorisers_obj(value["categorisers"]);
    categorisers_obj = Json::objectValue;

    for (map<string, Categoriser *>::const_iterator
	 i = categorisers.begin(); i != categorisers.end(); ++i) {
	Json::Value & categoriser_obj = categorisers_obj[i->first];
	i->second->to_json(categoriser_obj);
    }
}

void
CollectionConfig::categorisers_config_from_json(const Json::Value & value)
{
    const Json::Value & categorisers_obj(value["categorisers"]);
    if (!categorisers_obj.isNull()) {
	json_check_object(categorisers_obj, "categorisers definition");
	for (Json::Value::iterator i = categorisers_obj.begin();
	     i != categorisers_obj.end(); ++i) {
	    Categoriser categoriser;
	    categoriser.from_json(*i);
	    set_categoriser(i.memberName(), categoriser);
	}
    }
}

void
CollectionConfig::categories_config_to_json(Json::Value & value) const
{
    Json::Value & categories_obj(value["categories"]);
    categories_obj = Json::objectValue;

    for (map<string, CategoryHierarchy *>::const_iterator
	 i = categories.begin(); i != categories.end(); ++i) {
	Json::Value & category_obj = categories_obj[i->first];
	i->second->to_json(category_obj);
    }
}

void
CollectionConfig::categories_config_from_json(const Json::Value & value)
{
    const Json::Value & categories_obj(value["categories"]);
    if (!categories_obj.isNull()) {
	json_check_object(categories_obj, "categories definition");
	for (Json::Value::iterator i = categories_obj.begin();
	     i != categories_obj.end(); ++i) {
	    CategoryHierarchy category;
	    category.from_json(*i);
	    set_category(i.memberName(), category);
	}
    }
}

CollectionConfig::CollectionConfig(const string & coll_name_)
	: coll_name(coll_name_),
	  changed(false)
{}

CollectionConfig::~CollectionConfig()
{
    clear();
}

void
CollectionConfig::set_default()
{
    clear();
    set_default_schema();
    set_pipe("default", Pipe());
}

CollectionConfig *
CollectionConfig::clone() const
{
    auto_ptr<CollectionConfig> result(new CollectionConfig(coll_name));
    // FIXME - could probably be made significantly more efficient by doing the
    // copy directly on the contents, rather than going through JSON.
    Json::Value tmp;
    result->from_json(to_json(tmp));
    return result.release();
}

Json::Value &
CollectionConfig::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    schemas_config_to_json(value);
    if (!pipes.empty()) {
	pipes_config_to_json(value);
    }
    if (!categorisers.empty()) {
	categorisers_config_to_json(value);
    }
    if (!categories.empty()) {
	categories_config_to_json(value);
    }
    value["format"] = CONFIG_FORMAT;
    return value;
}

void
CollectionConfig::from_json(const Json::Value & value)
{
    json_check_object(value, "collection configuration");

    check_format_number(json_get_uint64_member(value, "format",
					       Json::Value::maxInt));

    schemas_config_from_json(value);
    pipes_config_from_json(value);
    categorisers_config_from_json(value);
    categories_config_from_json(value);
}

Schema *
CollectionConfig::get_schema(const string & type)
{
    map<string, Schema *>::const_iterator i = types.find(type);
    if (i == types.end()) {
	return NULL;
    }
    return (i->second);
}

const Schema *
CollectionConfig::get_schema(const string & type) const
{
    map<string, Schema *>::const_iterator i = types.find(type);
    if (i == types.end()) {
	return NULL;
    }
    return (i->second);
}

Schema *
CollectionConfig::set_schema(const string & type,
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
    changed = true;

    return schemaptr;
}

const Pipe &
CollectionConfig::get_pipe(const string & pipe_name) const
{
    map<string, Pipe *>::const_iterator i = pipes.find(pipe_name);
    if (i == pipes.end()) {
	throw InvalidValueError("No pipe of requested name found");
    }
    return *(i->second);
}

void
CollectionConfig::set_pipe(const string & pipe_name,
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
    changed = true;
}

const Categoriser &
CollectionConfig::get_categoriser(const string & categoriser_name) const
{
    map<string, Categoriser *>::const_iterator i = categorisers.find(categoriser_name);
    if (i == categorisers.end()) {
	throw InvalidValueError("No categoriser of requested name found");
    }
    return *(i->second);
}

void
CollectionConfig::set_categoriser(const string & categoriser_name,
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
    changed = true;
}

const CategoryHierarchy &
CollectionConfig::get_category(const string & category_name) const
{
    map<string, CategoryHierarchy *>::const_iterator i
	    = categories.find(category_name);
    if (i == categories.end()) {
	throw InvalidValueError("No category of requested name found");
    }
    return *(i->second);
}

void
CollectionConfig::set_category(const string & category_name,
			       const CategoryHierarchy & category)
{
    CategoryHierarchy * categoryptr;
    map<string, CategoryHierarchy *>::iterator i
	    = categories.find(category_name);

    if (i == categories.end()) {
	pair<map<string, CategoryHierarchy *>::iterator, bool> ret;
	pair<string, CategoryHierarchy *> item(category_name, NULL);
	ret = categories.insert(item);
	categoryptr = new CategoryHierarchy();
	ret.first->second = categoryptr;
    } else {
	categoryptr = i->second;
    }
    *categoryptr = category;
    changed = true;
}

Json::Value &
CollectionConfig::categorise(const string & categoriser_name,
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
CollectionConfig::send_to_pipe(TaskManager * taskman,
			       const string & pipe_name,
			       Json::Value & obj)
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
	Xapian::Document xdoc = process_doc(obj, "default", "", idterm);
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

Xapian::Document
CollectionConfig::process_doc(Json::Value & doc_obj,
			      const string & doc_type,
			      const string & doc_id,
			      string & idterm)
{
    json_check_object(doc_obj, "input document");

    const Json::Value & type_obj = doc_obj[type_field];
    string doc_type_(doc_type);
    if (doc_type.empty()) {
	// No document type supplied in URL - look for it in the document.
	if (type_obj.isNull()) {
	    throw InvalidValueError(
		"No document type supplied or stored in document.");
	}
	if (type_obj.isArray()) {
	    if (type_obj.size() == 1) {
		doc_type_ = json_get_idstyle_value(type_obj[0]);
	    } else if (type_obj.size() == 0) {
		throw InvalidValueError(
		    "No document type stored in document.");
	    } else {
		throw InvalidValueError(
		    "Multiple document types stored in document.");
	    }
	} else {
	    doc_type_ = json_get_idstyle_value(type_obj);
	}
    } else {
	// Document type supplied in URL - check that it isn't different in
	// document.
	if (!type_obj.isNull()) {
	    string stored_type;
	    if (type_obj.isArray()) {
		if (type_obj.size() == 1) {
		    stored_type = json_get_idstyle_value(type_obj[0]);
		} else if (type_obj.size() > 1) {
		    throw InvalidValueError(
			"Multiple document types stored in document.");
		}
	    } else {
		stored_type = json_get_idstyle_value(type_obj);
	    }
	    if (!stored_type.empty() && doc_type != stored_type) {
	    	throw InvalidValueError("Document type supplied differs from "
					"that inside document.");
	    }
	}
    }

    if (doc_id.empty()) {
	// No document id supplied in URL - assume it's in the document,
	// or deliberately absent.
    } else {
	// Document id supplied in URL - check that it isn't different in
	// document, and set it in the document.
	Json::Value & id_obj = doc_obj[id_field];
	if (id_obj.isNull()) {
	    id_obj = Json::arrayValue;
	    id_obj[0] = doc_id;
	} else {
	    string stored_id;
	    if (id_obj.isArray()) {
		if (id_obj.size() == 1) {
		    stored_id = json_get_idstyle_value(id_obj[0]);
		} else if (id_obj.size() > 1) {
		    throw InvalidValueError(
			"Multiple document ids stored in document.");
		}
	    } else {
		stored_id = json_get_idstyle_value(id_obj);
	    }
	    if (!stored_id.empty() && doc_id != stored_id) {
	    	throw InvalidValueError("Document id supplied differs from "
					"that inside document.");
	    }
	}
    }

    Schema * schema = get_schema(doc_type_);
    if (schema == NULL) {
	Schema newschema(doc_type_);
	newschema.from_json(default_type_config);
	schema = set_schema(doc_type_, newschema);
    }
    return schema->process(doc_obj, idterm);
}


Collection::Collection(const string & coll_name_,
		       const string & coll_path_)
	: config(coll_name_),
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
Collection::read_config()
{
    try {
	string config_str(group.get_metadata("_restpose_config"));

	if (!last_config.empty() && config_str == last_config) {
	    return;
	}

	last_config = config_str;
	if (config_str.empty()) {
	    config.set_default();
	    return;
	}

	// Set the schema.
	Json::Value config_obj;
	config.from_json(json_unserialise(config_str, config_obj));
    } catch(...) {
	group.close();
	throw;
    }
}

void
Collection::write_config()
{
    Json::Value config_obj;
    group.set_metadata("_restpose_config", 
		       json_serialise(config.to_json(config_obj)));
}

Schema &
Collection::get_schema(const string & type)
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    Schema * result = config.get_schema(type);
    if (result == NULL) {
	throw InvalidValueError("Schema not found");
    }
    return *result;
}

void
Collection::set_schema(const string & type,
		       const Schema & schema)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set schema");
    }
    config.set_schema(type, schema);
    write_config();
}

const Pipe &
Collection::get_pipe(const string & pipe_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    return config.get_pipe(pipe_name);
}

void
Collection::set_pipe(const string & pipe_name,
		     const Pipe & pipe)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set pipe");
    }
    config.set_pipe(pipe_name, pipe);
    write_config();
}

const Categoriser &
Collection::get_categoriser(const string & categoriser_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    return config.get_categoriser(categoriser_name);
}

void
Collection::set_categoriser(const string & categoriser_name,
			    const Categoriser & categoriser)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set categoriser");
    }
    config.set_categoriser(categoriser_name, categoriser);
    write_config();
}

const CategoryHierarchy &
Collection::get_category(const string & category_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    return config.get_category(category_name);
}

void
Collection::set_category(const string & category_name,
			 const CategoryHierarchy & category)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set category");
    }
    config.set_category(category_name, category);
    write_config();
}

void
Collection::from_json(const Json::Value & value)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set config");
    }
    config.from_json(value);
    write_config();
}

Json::Value &
Collection::categorise(const string & categoriser_name,
		       const string & text,
		       Json::Value & result) const
{
    return config.categorise(categoriser_name, text, result);
}

void
Collection::send_to_pipe(TaskManager * taskman,
			 const string & pipe_name,
			 Json::Value & obj)
{
    config.send_to_pipe(taskman, pipe_name, obj);
}

void
Collection::add_doc(Json::Value & doc_obj,
		    const string & doc_type)
{
    string idterm;
    Xapian::Document doc(config.process_doc(doc_obj, doc_type, "FIXME", idterm));
    raw_update_doc(doc, idterm);
}

Xapian::Document
Collection::process_doc(Json::Value & doc_obj,
			const string & doc_type,
			const string & doc_id,
			string & idterm)
{
    return config.process_doc(doc_obj, doc_type, doc_id, idterm);
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
    LOG_INFO("Committing changes to collection \"" + config.get_name() + "\"");
    group.sync();
}

uint64_t
Collection::doc_count() const
{
    return get_db().get_doccount();
}

void
Collection::perform_search(const Json::Value & search,
			   const string & doc_type,
			   Json::Value & results) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to perform search");
    }
    const Schema * schema = config.get_schema(doc_type);
    if (schema == NULL) {
	results = Json::objectValue;
    } else {
	schema->perform_search(get_db(), search, results);
    }
}

void
Collection::get_doc_fields(const Xapian::Document & doc,
			   const string & doc_type,
			   const Json::Value & fieldlist,
			   Json::Value & result) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get document");
    }
    const Schema * schema = config.get_schema(doc_type);
    if (schema == NULL) {
	result = Json::objectValue;
    } else {
	schema->display_doc(doc, fieldlist, result);
    }
}

void
Collection::get_document(const string & doc_type,
			 const string & docid,
			 Json::Value & result) const
{
    bool found;
    string idterm = "\t" + doc_type + "\t" + docid;
    Xapian::Document doc = group.get_document(idterm, found);
    if (found) {
	doc_to_json(doc, result);
    } else {
	result = Json::nullValue;
    }
}
