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
#include "jsonxapian/indexing.h"
#include "jsonxapian/pipe.h"
#include "logger/logger.h"
#include "str.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/stringutils.h"
#include <vector>

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

void
Collection::update_modified_categories(const std::string & prefix,
				       const CategoryHierarchy & hierarchy,
				       const Categories & modified)
{
    // Find all documents with terms of the form prefix + "C" + cat where cat
    // is any of the categories in modified.  For each of these documents, read
    // in the list of terms of form prefix + "C" + *, build the appropriate
    // list of ancestor categories using the hierarchy, and set the list of
    // terms of form prefix + "A" to the ancestors.

    LOG_DEBUG("updating " + str(modified.size()) +
	      " modified categories for prefix: " + prefix);

    string cat_prefix = prefix + "C";
    string ancestor_prefix = prefix + "A";

    Xapian::Database db = group.get_db();
    vector<Xapian::PostingIterator> iters;
    for (Categories::const_iterator i = modified.begin();
	 i != modified.end(); ++i) {
	iters.push_back(db.postlist_begin(cat_prefix + *i));
	LOG_DEBUG("starting iteration of documents in category " + *i);
    }

    while (!iters.empty()) {
	Xapian::docid nextid = 0;
	// Run through the iterators, setting nextid to the minimum id.
	vector<Xapian::PostingIterator>::iterator piter = iters.begin();
	while (piter != iters.end()) {
	    if (*piter == Xapian::PostingIterator()) {
		LOG_DEBUG("iter at end");
		piter = iters.erase(piter);
		continue;
	    }
	    Xapian::docid thisid = **piter;
	    if (nextid == 0 || nextid > thisid) {
		LOG_DEBUG("setting nextid to " + str(thisid));
		nextid = thisid;
	    }
	    ++piter;
	}
	if (nextid == 0) {
	    break;
	}

	// Build list of ancestor categories for this document.
	Xapian::TermIterator ti = db.termlist_begin(nextid);
	ti.skip_to(cat_prefix);
	Categories ancestors;
	while (ti != db.termlist_end(nextid)) {
	    if (!string_startswith(*ti, cat_prefix)) {
		break;
	    }
	    const Category * cat =
		    hierarchy.find((*ti).substr(cat_prefix.size()));
	    if (cat != NULL) {
		ancestors.insert(cat->ancestors.begin(),
				 cat->ancestors.end());
	    }
	    ++ti;
	}
	LOG_DEBUG("Ancestors are: " + string_join(",", ancestors.begin(), ancestors.end()));

	// Set terms to reflect ancestors.
	Xapian::Document doc(db.get_document(nextid));
	ti = doc.termlist_begin();
	ti.skip_to("\t");
	if (ti == doc.termlist_end() || (*ti)[0] != '\t') {
	    throw InvalidValueError("Document has no ID - cannot update category terms");
	}
	string idterm = *ti;

	ti = doc.termlist_begin();
	ti.skip_to(ancestor_prefix);
	while (ti != doc.termlist_end()) {
	    if (!string_startswith(*ti, ancestor_prefix)) {
		break;
	    }
	    string cat = (*ti).substr(ancestor_prefix.size());
	    // Remove the category from ancestors if its there, or remove it
	    // from the document if it's not there.
	    if (!ancestors.erase(cat)) {
		doc.remove_term(*ti);
	    }

	    ++ti;
	}
	//Add any remaining categories in ancestors.
	LOG_DEBUG("Adding new ancestors: " + string_join(",", ancestors.begin(), ancestors.end()));
	for (Categories::const_iterator ancestor = ancestors.begin();
	     ancestor != ancestors.end(); ++ancestor) {
	    doc.add_term(ancestor_prefix + *ancestor, 0);
	}

        group.add_doc(doc, idterm);

	// Advance all iterators for which nextid is the minimum id.
	piter = iters.begin();
	while (piter != iters.end()) {
	    if (**piter == nextid) {
		++(*piter);
	    }
	    ++piter;
	}
    }
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
Collection::get_category_hierarchy(const string & category_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get schema");
    }
    return config.get_category_hierarchy(category_name);
}

void
Collection::set_category_hierarchy(const string & category_name,
			 const CategoryHierarchy & category)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set category");
    }
    config.set_category_hierarchy(category_name, category);
    write_config();
}

void
Collection::category_add(const std::string & hierarchy_name,
			 const std::string & cat_name)
{
    Categories modified;
    config.category_add(hierarchy_name, cat_name, modified);
    // modified either contains the new category, or is empty if the
    // category already existed.  In either case, there are no
    // changes to other categories, so no need to update documents.
}

void
Collection::category_remove(const std::string & hierarchy_name,
			    const std::string & cat_name)
{
    Categories modified;
    const CategoryHierarchy & hierarchy =
	    config.category_remove(hierarchy_name, cat_name, modified);
    update_modified_categories(hierarchy_name + "\t", hierarchy, modified);
}

void
Collection::category_add_parent(const std::string & hierarchy_name,
				const std::string & child_name,
				const std::string & parent_name)
{
    Categories modified;
    const CategoryHierarchy & hierarchy =
	    config.category_add_parent(hierarchy_name, child_name,
				       parent_name, modified);
    update_modified_categories(hierarchy_name + "\t", hierarchy, modified);
}

void
Collection::category_remove_parent(const std::string & hierarchy_name,
				   const std::string & child_name,
				   const std::string & parent_name)
{
    Categories modified;
    const CategoryHierarchy & hierarchy =
	    config.category_remove_parent(hierarchy_name, child_name,
					  parent_name, modified);
    update_modified_categories(hierarchy_name + "\t", hierarchy, modified);
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
    IndexingErrors errors;
    Xapian::Document doc(config.process_doc(doc_obj, doc_type, "FIXME", idterm, errors));
    if (!errors.errors.empty()) {
	throw InvalidValueError(errors.errors[0].first + ": " + errors.errors[0].second);
    }
    raw_update_doc(doc, idterm);
}

Xapian::Document
Collection::process_doc(Json::Value & doc_obj,
			const string & doc_type,
			const string & doc_id,
			string & idterm)
{
    IndexingErrors errors;
    Xapian::Document doc(config.process_doc(doc_obj, doc_type, doc_id, idterm, errors));
    if (!errors.errors.empty()) {
	throw InvalidValueError(errors.errors[0].first + ": " + errors.errors[0].second);
    }
    return doc;
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
	Schema tmp(doc_type);
	tmp.perform_search(config, get_db(), search, results);
    } else {
	schema->perform_search(config, get_db(), search, results);
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
