/** @file collconfig.cc
 * @brief Collection configuration.
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
#include "collconfig.h"

#include <xapian.h>
#include "jsonxapian/doctojson.h"
#include "jsonxapian/indexing.h"
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
    meta_field = "_meta";

    Schema defschema("");
    Json::Value tmp;
    defschema.from_json(json_unserialise(
"{"
"  \"patterns\": ["
"    [ \"*_text\", { \"type\": \"text\", \"prefix\": \"t*\", \"store_field\": \"*_text\", \"processor\": \"stem_en\" } ],"
"    [ \"text\", { \"type\": \"text\", \"prefix\": \"t\", \"store_field\": \"text\", \"processor\": \"stem_en\" } ],"
"    [ \"*_time\", { \"type\": \"timestamp\", \"slot\": \"d*\", \"store_field\": \"*_time\" } ],"
"    [ \"time\", { \"type\": \"timestamp\", \"slot\": \"d\", \"store_field\": \"time\" } ],"
"    [ \"*_tag\", { \"type\": \"exact\", \"prefix\": \"g*\", \"store_field\": \"*_tag\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"tag\", { \"type\": \"exact\", \"prefix\": \"g\", \"store_field\": \"tag\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"*_url\", { \"type\": \"exact\", \"prefix\": \"u*\", \"store_field\": \"*_url\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"url\", { \"type\": \"exact\", \"prefix\": \"u\", \"store_field\": \"url\", \"max_length\": 100, \"too_long_action\": \"hash\" } ],"
"    [ \"*_cat\", { \"type\": \"cat\", \"prefix\": \"c*\", \"store_field\": \"*_cat\", \"max_length\": 32, \"too_long_action\": \"hash\" } ],"
"    [ \"cat\", { \"type\": \"cat\", \"prefix\": \"c\", \"store_field\": \"cat\", \"max_length\": 32, \"too_long_action\": \"hash\" } ],"
"    [ \"id\", { \"type\": \"id\", \"store_field\": \"id\" } ],"
"    [ \"type\", { \"type\": \"exact\", \"prefix\": \"!\", \"store_field\": \"type\" } ],"
"    [ \"_meta\", { \"type\": \"meta\", \"prefix\": \"#\", \"slot\": 0 } ],"
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
    special_obj["meta_field"] = meta_field;
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
	meta_field = json_get_string_member(special_obj, "meta_field",
					    meta_field);
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
	    CategoryHierarchy hierarchy;
	    hierarchy.from_json(*i);
	    set_category_hierarchy(i.memberName(), hierarchy);
	}
    }
}

CategoryHierarchy &
CollectionConfig::get_or_add_cat_hierarchy(const std::string & hierarchy_name)
{
    map<string, CategoryHierarchy *>::const_iterator i
	    = categories.find(hierarchy_name);
    if (i == categories.end()) {
	pair<map<string, CategoryHierarchy *>::iterator, bool> ret;
	pair<string, CategoryHierarchy *> item(hierarchy_name, NULL);
	ret = categories.insert(item);
	CategoryHierarchy * categoryptr = new CategoryHierarchy();
	ret.first->second = categoryptr;
	changed = true;
	return *categoryptr;
    } else {
	return *(i->second);
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

const CategoryHierarchy *
CollectionConfig::get_category_hierarchy(const string & hierarchy_name) const
{
    map<string, CategoryHierarchy *>::const_iterator i
	    = categories.find(hierarchy_name);
    if (i == categories.end()) {
	return NULL;
    }
    return i->second;
}

void
CollectionConfig::set_category_hierarchy(const string & hierarchy_name,
					 const CategoryHierarchy & category)
{
    CategoryHierarchy * categoryptr;
    map<string, CategoryHierarchy *>::iterator i
	    = categories.find(hierarchy_name);

    if (i == categories.end()) {
	pair<map<string, CategoryHierarchy *>::iterator, bool> ret;
	pair<string, CategoryHierarchy *> item(hierarchy_name, NULL);
	ret = categories.insert(item);
	categoryptr = new CategoryHierarchy();
	ret.first->second = categoryptr;
    } else {
	categoryptr = i->second;
    }

    // Copy the category hierarchy.
    *categoryptr = category;
    changed = true;
}

const CategoryHierarchy &
CollectionConfig::category_add(const std::string & hierarchy_name,
			       const std::string & cat_name,
			       Categories & modified)
{
    CategoryHierarchy & hierarchy = get_or_add_cat_hierarchy(hierarchy_name);
    hierarchy.add(cat_name, modified);
    changed = true;
    return hierarchy;
}

const CategoryHierarchy &
CollectionConfig::category_remove(const std::string & hierarchy_name,
				  const std::string & cat_name,
				  Categories & modified)
{
    CategoryHierarchy & hierarchy = get_or_add_cat_hierarchy(hierarchy_name);
    hierarchy.remove(cat_name, modified);
    changed = true;
    return hierarchy;
}

const CategoryHierarchy &
CollectionConfig::category_add_parent(const std::string & hierarchy_name,
				      const std::string & child_name,
				      const std::string & parent_name,
				      Categories & modified)
{
    CategoryHierarchy & hierarchy = get_or_add_cat_hierarchy(hierarchy_name);
    hierarchy.add_parent(child_name, parent_name, modified);
    changed = true;
    return hierarchy;
}

const CategoryHierarchy &
CollectionConfig::category_remove_parent(const std::string & hierarchy_name,
					 const std::string & child_name,
					 const std::string & parent_name,
					 Categories & modified)
{
    CategoryHierarchy & hierarchy = get_or_add_cat_hierarchy(hierarchy_name);
    hierarchy.remove_parent(child_name, parent_name, modified);
    changed = true;
    return hierarchy;
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
    LOG_DEBUG("Sending to pipe \"" + pipe_name + "\"");
    // FIXME - this method just uses a recursive implementation, and doesn't
    // check that a document isn't being passed to a pipe it's already come
    // from (which would almost certainly be a mistake), so it's easy for it
    // to overflow the stack, and kill the process.  Would be better to do a
    // non-recursive implementation, and keep track of pipes which have
    // already been used to ensure that no loops happen.

    if (pipe_name.empty()) {
	string idterm;
	IndexingErrors errors;
	// FIXME - remove hardcoded "default" here - pipes should have a way to
	// say what type the output document is.
	Xapian::Document xdoc = process_doc(obj, "default", "", idterm, errors);

	if (!errors.errors.empty()) {
	    throw InvalidValueError(errors.errors[0].first + ": " + errors.errors[0].second);
	}

	taskman->queue_indexing_from_processing(get_name(),
	    new IndexerUpdateDocumentTask(idterm, xdoc));

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
			      string & idterm,
			      IndexingErrors & errors)
{
    json_check_object(doc_obj, "input document");
    Xapian::Document doc;

    string doc_type_(doc_type);
    if (doc_type.empty()) {
	// No document type supplied in URL - look for it in the document.
	const Json::Value & type_obj = doc_obj[type_field];
	if (type_obj.isNull()) {
	    errors.append(type_field,
			  "No document type supplied or stored in document.");
	    errors.total_failure = true;
	    return doc;
	}
	if (type_obj.isArray()) {
	    if (type_obj.size() == 1) {
		string error;
		doc_type_ = json_get_idstyle_value(type_obj[0], error);
		if (!error.empty()) {
		    errors.append(type_field, error);
		    errors.total_failure = true;
		    return doc;
		}
	    } else if (type_obj.size() == 0) {
		errors.append(type_field,
			      "No document type stored in document.");
		errors.total_failure = true;
		return doc;
	    } else {
		errors.append(type_field,
			      "Multiple document types stored in document.");
		errors.total_failure = true;
		return doc;
	    }
	} else {
	    string error;
	    doc_type_ = json_get_idstyle_value(type_obj, error);
	    if (!error.empty()) {
		errors.append(type_field, error);
		errors.total_failure = true;
		return doc;
	    }
	}
    } else {
	// Document type supplied in URL - check that it isn't different in
	// document.
	Json::Value & type_obj = doc_obj[type_field];
	if (type_obj.isNull()) {
	    type_obj = Json::arrayValue;
	    type_obj[0] = doc_type;
	} else {
	    string stored_type;
	    if (type_obj.isArray()) {
		if (type_obj.size() == 1) {
		    string error;
		    stored_type = json_get_idstyle_value(type_obj[0], error);
		    if (!error.empty()) {
			errors.append(type_field, error);
			errors.total_failure = true;
			return doc;
		    }
		} else if (type_obj.size() > 1) {
		    errors.append(type_field,
				  "Multiple document types stored in document.");
		    errors.total_failure = true;
		    return doc;
		}
	    } else {
		string error;
		stored_type = json_get_idstyle_value(type_obj, error);
		if (!error.empty()) {
		    errors.append(type_field, error);
		    errors.total_failure = true;
		    return doc;
		}
	    }
	    if (!stored_type.empty() && doc_type != stored_type) {
		errors.append(type_field,
			      "Document type supplied differs from "
			      "that inside document.");
		errors.total_failure = true;
		return doc;
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
		    string error;
		    stored_id = json_get_idstyle_value(id_obj[0], error);
		    if (!error.empty()) {
			errors.append(id_field, error);
			errors.total_failure = true;
			return doc;
		    }
		} else if (id_obj.size() > 1) {
		    errors.append(id_field,
				  "Multiple document ids stored in document.");
		    errors.total_failure = true;
		    return doc;
		}
	    } else {
		string error;
		stored_id = json_get_idstyle_value(id_obj, error);
		if (!error.empty()) {
		    errors.append(id_field, error);
		    errors.total_failure = true;
		    return doc;
		}
	    }
	    if (!stored_id.empty() && doc_id != stored_id) {
		errors.append(id_field,
			      "Document id supplied ('" + doc_id +
			      "') differs from that inside document ('" +
			      stored_id + "').");
		errors.total_failure = true;
		return doc;
	    }
	}
    }

    Schema * schema = get_schema(doc_type_);
    if (schema == NULL) {
	Schema newschema(doc_type_);
	newschema.from_json(default_type_config);
	schema = set_schema(doc_type_, newschema);
    }
    doc = schema->process(doc_obj, *this, idterm, errors);
    return doc;
}
