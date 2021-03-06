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

#include "infohandlers.h"
#include "jsonxapian/doctojson.h"
#include "jsonxapian/indexing.h"
#include "jsonxapian/pipe.h"
#include "jsonxapian/query_builder.h"
#include "logger/logger.h"
#include <memory>
#include "postingsources/multivalue_keymaker.h"
#include "str.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/stringutils.h"
#include <vector>
#include <xapian.h>

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
Collection::update_modified_categories_group(const string & prefix,
					     const Taxonomy & taxonomy,
					     const Categories & modified)
{
    // Find all documents with terms of the form prefix + "C" + cat where cat
    // is any of the categories in modified.  For each of these documents,
    // read in the list of terms of form prefix + "C" + *, build the
    // appropriate list of ancestor categories using the taxonomy, and set
    // the list of terms of form prefix + "A" to the ancestors.

    LOG_DEBUG("updating " + str(modified.size()) +
	      " modified categories for group: " + prefix);

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
		    taxonomy.find((*ti).substr(cat_prefix.size()));
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

void
Collection::update_modified_categories(const string & taxonomy_name,
				       const Taxonomy & taxonomy,
				       const Categories & modified)
{
    const set<string> & groups = config.get_taxonomy_groups(taxonomy_name);

    /* Note; when there are many documents in which more than one group uses
     * the same taxonomy, it would be more efficient to do the update of all
     * the groups at the same time, rather than one-by-one; we only do it
     * one-by-one for ease of implementation.
     */
    for (set<string>::const_iterator i = groups.begin();
	 i != groups.end(); ++i) {
	update_modified_categories_group(*i + "\t", taxonomy, modified);
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
	throw InvalidStateError("Collection must be open to get pipe");
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
	throw InvalidStateError("Collection must be open to get categoriser");
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

const Taxonomy *
Collection::get_taxonomy(const string & category_name) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get taxonomy");
    }
    return config.get_taxonomy(category_name);
}

void
Collection::set_taxonomy(const string & category_name,
			 const Taxonomy & category)
{
    if (!group.is_writable()) {
	throw InvalidStateError("Collection must be open for writing to set category");
    }
    config.set_taxonomy(category_name, category);
    write_config();
}

Json::Value &
Collection::get_taxonomy_names(Json::Value & result) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to get taxonomy");
    }
    return config.get_taxonomy_names(result);
}

void
Collection::remove_taxonomy(const string & taxonomy_name)
{
    config.remove_taxonomy(taxonomy_name);

    // Update all documents which used the taxonomy.
    Xapian::Database db = group.get_db();
    const set<string> & groups = config.get_taxonomy_groups(taxonomy_name);
    for (set<string>::const_iterator i = groups.begin();
	 i != groups.end(); ++i) {
	string prefix = *i + "\t";
	string ancestor_prefix = prefix + "A";
	// Remove all terms starting with ancestor_prefix.
	for (Xapian::TermIterator allti = db.allterms_begin(ancestor_prefix);
	     allti != db.allterms_end(ancestor_prefix); ++allti) {
	    for (Xapian::PostingIterator pi = db.postlist_begin(*allti);
		 pi != db.postlist_end(*allti); ++pi) {
		Xapian::Document doc(db.get_document(*pi));
		Xapian::TermIterator ti = doc.termlist_begin();

		// Get the idterm
		ti.skip_to("\t");
		if (ti == doc.termlist_end() || (*ti)[0] != '\t') {
		    throw InvalidValueError("Document has no ID - cannot update category terms");
		}
		string idterm = *ti;

		ti.skip_to(ancestor_prefix);
		while (ti != doc.termlist_end() &&
		       string_startswith(*ti, ancestor_prefix)) {
		    doc.remove_term(*ti);
		}

		group.add_doc(doc, idterm);
	    }
	}
    }

    write_config();
}

void
Collection::category_add(const string & taxonomy_name,
			 const string & cat_name)
{
    Categories modified;
    config.category_add(taxonomy_name, cat_name, modified);
    // modified either contains the new category, or is empty if the
    // category already existed.  In either case, there are no
    // changes to other categories, so no need to update documents.
    write_config();
}

void
Collection::category_remove(const string & taxonomy_name,
			    const string & cat_name)
{
    Categories modified;
    const Taxonomy & taxonomy =
	    config.category_remove(taxonomy_name, cat_name, modified);
    update_modified_categories(taxonomy_name, taxonomy, modified);
    write_config();
}

void
Collection::category_add_parent(const string & taxonomy_name,
				const string & child_name,
				const string & parent_name)
{
    Categories modified;
    const Taxonomy & taxonomy =
	    config.category_add_parent(taxonomy_name, child_name,
				       parent_name, modified);
    update_modified_categories(taxonomy_name, taxonomy, modified);
    write_config();
}

void
Collection::category_remove_parent(const string & taxonomy_name,
				   const string & child_name,
				   const string & parent_name)
{
    Categories modified;
    const Taxonomy & taxonomy =
	    config.category_remove_parent(taxonomy_name, child_name,
					  parent_name, modified);
    update_modified_categories(taxonomy_name, taxonomy, modified);
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
			 Json::Value & obj,
			 bool & new_fields)
{
    config.send_to_pipe(taskman, pipe_name, obj, new_fields);
}

void
Collection::add_doc(Json::Value & doc_obj,
		    const string & doc_type)
{
    string idterm;
    IndexingErrors errors;
    bool new_fields;
    Xapian::Document doc(config.process_doc(doc_obj, doc_type, "FIXME", idterm, errors, new_fields));
    if (!errors.errors.empty()) {
	throw InvalidValueError(errors.errors[0].first + ": " + errors.errors[0].second);
    }
    raw_update_doc(doc, idterm);
}

Xapian::Document
Collection::process_doc(Json::Value & doc_obj,
			const string & doc_type,
			const string & doc_id,
			string & idterm,
			bool & new_fields)
{
    IndexingErrors errors;
    Xapian::Document doc(config.process_doc(doc_obj, doc_type, doc_id, idterm,
					    errors, new_fields));
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

static Xapian::doccount
calc_fromdoc_offset(const Xapian::Database & db,
		    const Xapian::Enquire & enq,
		    const string & fromdoc_type,
		    const string & fromdoc_id,
		    Xapian::doccount fromdoc_pagesize,
		    int fromdoc_from,
		    Xapian::doccount check_at_least)
{
    Xapian::doccount from = 0;
    // Get the Xapian ID of the document.
    string idterm = "\t" + fromdoc_type + "\t" + fromdoc_id;
    Xapian::PostingIterator idpl(db.postlist_begin(idterm));
    Xapian::docid fromdoc_xapid;
    if (idpl != db.postlist_end(idterm)) {
	// idterm is in the db.
	fromdoc_xapid = *idpl;
    } else {
	// The document isn't in the database; just return an error.
	// FIXME - identify this error somehow so it can be handled more
	// easily.
	throw InvalidValueError("fromdoc document not present in database");
    }

    while (true) {
	Xapian::MSet mset = enq.get_mset(from, fromdoc_pagesize,
					 check_at_least);
	for (Xapian::MSet::const_iterator i = mset.begin();
	     i != mset.end(); ++i) {
	    if (*i == fromdoc_xapid) {
		from = i.get_rank();
		if (fromdoc_from < 0 &&
		    Xapian::doccount(-fromdoc_from) > from) {
		    from = 0;
		} else {
		    from += fromdoc_from;
		}
		return from;
	    }
	}

	if (mset.size() != fromdoc_pagesize) {
	    break;
	}
	from += fromdoc_pagesize;
    }
    // FIXME - identify this error somehow so it can be handled more
    // easily.
    throw InvalidValueError("fromdoc document not present in result set");
}

void
Collection::perform_search(const Json::Value & search,
			   const string & doc_type,
			   Json::Value & results) const
{
    if (!group.is_open()) {
	throw InvalidStateError("Collection must be open to perform search");
    }
    bool verbose = json_get_bool(search, "verbose", false);

    // Get the list of fields to return, and check it's a list of strings.
    const Json::Value & fieldlist = search["display"];
    if (!fieldlist.isNull()) {
	json_check_array(fieldlist, "list of fields to display");
	for (Json::Value::const_iterator fiter = fieldlist.begin();
	     fiter != fieldlist.end();
	     ++fiter) {
	    if (!(*fiter).isString()) {
		throw InvalidValueError(
			"Item in display field list was not a string");
	    }
	}
    }

    auto_ptr<QueryBuilder> builder;
    if (doc_type.empty()) {
	builder = auto_ptr<QueryBuilder>(new CollectionQueryBuilder(config));
    } else {
	builder = auto_ptr<QueryBuilder>(
		new DocumentTypeQueryBuilder(config, doc_type));
    }

    results = Json::objectValue;
    Xapian::Query query(builder->build(search["query"]));

    Xapian::Database db(get_db());

    Xapian::doccount total_docs, from, size, check_at_least;
    total_docs = builder->total_docs(db);
    from = json_get_uint64_member(search, "from", Json::Value::maxUInt, 0);

    if (search["size"] == -1) {
	size = total_docs;
    } else {
	size = json_get_uint64_member(search, "size", Json::Value::maxUInt, 10);
    }

    // Check for a request to get results at an offset from a specific document.
    string fromdoc_type;
    string fromdoc_id;
    int fromdoc_from = 0;
    Xapian::doccount fromdoc_pagesize = 10000;
    Json::Value fromdoc_obj = search["fromdoc"];
    if (!fromdoc_obj.isNull()) {
	if (from != 0) {
	    throw InvalidValueError("fromdoc was supplied, but from was not 0");
	}
	if (!fromdoc_obj.isObject()) {
	    throw InvalidValueError("Invalid value supplied for fromdoc: "
				    "expected an object");
	}
	string error;
	fromdoc_type = json_get_idstyle_value(fromdoc_obj["type"], error);
	if (!error.empty()) {
	    throw InvalidValueError("Invalid value supplied for fromdoc type: " +
				    error);
	}
	if (fromdoc_type.empty()) {
	    throw InvalidValueError("Missing or empty type supplied for fromdoc");
	}
	fromdoc_id = json_get_idstyle_value(fromdoc_obj["id"], error);
	if (!error.empty()) {
	    throw InvalidValueError("Invalid value supplied for fromdoc ID: " +
				    error);
	}
	if (fromdoc_id.empty()) {
	    throw InvalidValueError("Missing or empty ID supplied for fromdoc");
	}
	Json::Value & tmp = fromdoc_obj["from"];
	if (!tmp.isNull()) {
	    if (!tmp.isConvertibleTo(Json::intValue)) {
		throw InvalidValueError("fromdoc \"from\" parameter is not convertible to an integer");
	    }
	    fromdoc_from = tmp.asInt();
	}
	fromdoc_pagesize = json_get_uint64_member(fromdoc_obj, "pagesize",
						  Json::Value::maxUInt,
						  fromdoc_pagesize);
    }

    if (search["check_at_least"] == -1) {
	check_at_least = total_docs;
    } else {
	check_at_least = json_get_uint64_member(search, "check_at_least",
						Json::Value::maxUInt, 0);
    }

    Xapian::Enquire enq(db);
    enq.set_query(query);
    enq.set_weighting_scheme(Xapian::BoolWeight());

    InfoHandlers info_handlers;
    if (search.isMember("info")) {
	const Json::Value & info = search["info"];
	json_check_array(info, "list of info items to gather");
	for (Json::Value::const_iterator i = info.begin();
	     i != info.end(); ++i) {
	    info_handlers.add_handler(*i, *(builder.get()),
				      enq, &db, check_at_least);
	}
    }

    // Internal document IDs are not under the user's control, so set this
    // option for potential (though probably slight) performance increases.
    enq.set_docid_order(enq.DONT_CARE);
    auto_ptr<MultiValueKeyMaker> sorter;

    if (search.isMember("order_by")) {
	const Json::Value & order_by = search["order_by"];
	json_check_array(order_by, "list of ordering items");
	bool score_first = false;
	bool score_last = false;
	for (unsigned i = 0; i != order_by.size(); ++i) {
	    const Json::Value & order_by_item = order_by[i];
	    json_check_object(order_by_item, "ordering item");

	    if (order_by_item.isMember("field")) {
		// Order by a field.  The field must have a slot associated with
		// it, holding the sortable values.
		string fieldname = json_get_string_member(order_by_item, "field",
							  string());

		auto_ptr<SlotDecoder> decoder(builder->get_slot_decoder(fieldname));
		// FIXME - make it obvious why the sorting didn't happen; this
		// shouldn't be an error, because it could just be that no
		// documents have yet been indexed with the given field, but it
		// should be reflected in the search results somehow (possibly only
		// in an explain view).

		if (decoder.get() != NULL) {
		    if (sorter.get() == NULL) {
			sorter = auto_ptr<MultiValueKeyMaker>(new MultiValueKeyMaker());
		    }

		    bool ascending = json_get_bool(order_by_item, "ascending", true);
		    sorter->add_decoder(decoder.release(), !ascending);
		} else {
		    LOG_WARN("Unable to apply requested sort by \"" +
			     fieldname + "\" - no field config found.");
		}
	    } else if (order_by_item.isMember("score")) {
		// Order by the weights calculated in the query tree.
		if (order_by_item["score"] != "weight") {
		    throw InvalidValueError("Invalid score specification (only "
					    "allowed value is \"weight\")");
		}
		if (json_get_bool(order_by_item, "ascending", false)) {
		    throw InvalidValueError("Ascending order is not allowed when "
					    "ordering by weight");
		}
		if (i == 0) {
		    score_first = true;
		} else if (i + 1 == order_by.size()) {
		    score_last = true;
		} else {
		    throw InvalidValueError("Sorting by score is only allowed "
					    "as the first or last sorting "
					    "condition (was " + str(i) + " of "
					    + str(order_by.size()) + ")");
		}
	    } else {
		throw InvalidValueError("Invalid order_by item - neither contains "
					"\"field\" or \"score\" member");
	    }
	}
	if (score_first && score_last) {
	    throw InvalidValueError("Sorting condition list may only contain "
				    "sorting by score once.");
	}
	if (sorter.get() == NULL) {
	    enq.set_sort_by_relevance();
	} else {
	    if (score_first) {
		enq.set_sort_by_relevance_then_key(sorter.get(), false);
	    } else if (score_last) {
		enq.set_sort_by_key_then_relevance(sorter.get(), false);
	    } else {
		enq.set_sort_by_key(sorter.get(), false);
	    }
	}
    }

    if (!fromdoc_id.empty()) {
	from = calc_fromdoc_offset(db, enq, fromdoc_type, fromdoc_id,
				   fromdoc_pagesize, fromdoc_from,
				   check_at_least);
    }
    Xapian::MSet mset(enq.get_mset(from, size, check_at_least));

    // Write the results
    info_handlers.write_results(results, mset);
    results["total_docs"] = total_docs;
    results["from"] = from;
    results["size_requested"] = size;
    results["check_at_least"] = check_at_least;
    results["matches_lower_bound"] = mset.get_matches_lower_bound();
    results["matches_estimated"] = mset.get_matches_estimated();
    results["matches_upper_bound"] = mset.get_matches_upper_bound();
    Json::Value & items = results["items"] = Json::arrayValue;
    for (Xapian::MSetIterator i = mset.begin(); i != mset.end(); ++i) {
	Xapian::Document doc(i.get_document());
	DocumentData docdata;
	docdata.unserialise(doc.get_data());
	Json::Value tmp;
	items.append(docdata.to_display(fieldlist, tmp));
    }
    if (verbose) {
	// Give debugging details about the search executed.
	// Note - we can't just include query.get_description() in the output,
	// because this isn't always a valid unicode string, so we escape it
	// with hexesc.
	results["query_description"] = hexesc(query.get_description());

	// Also include the serialised form, since this can be usefully
	// unserialised to build testcases to demonstrate problems.
	results["query_serialised"] = hexesc(query.serialise());
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
