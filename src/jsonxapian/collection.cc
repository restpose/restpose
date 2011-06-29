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
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace std;
using namespace RestPose;

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

const CategoryHierarchy *
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
