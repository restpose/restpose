/** @file collection.h
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

#ifndef RESTPOSE_INCLUDED_COLLECTION_H
#define RESTPOSE_INCLUDED_COLLECTION_H

#include "dbgroup/dbgroup.h"
#include "jsonmanip/mapping.h"
#include "jsonxapian/collconfig.h"
#include "ngramcat/categoriser.h"
#include "schema.h"
#include <string>
#include "utils/safe_inttypes.h"
#include <xapian.h>

class TaskManager;

namespace RestPose {

class Pipe;

class Collection {
    /** The configuration used for this collection.
     */
    CollectionConfig config;

    /** A cache of the serialised form of the last config read from the
     *  database.  Used to avoid re-parsing the config on reopen when it hasn't
     *  changed.
     */
    std::string last_config;

    RestPose::DbGroup group;

    /** Get a database object.
     *
     *  Will return a reference to whichever of wrdb or rodb is open,
     *  or open the database readonly and return rodb if it's closed.
     */
    const Xapian::Database & get_db() const;

    /** Read the stored configuration (schemas, pipes, etc).
     *
     *  Wipes out any config set but not stored - this is intended to be
     *  used when the collection is first opened.
     */
    void read_config();

    /** Write the configuration to the database.
     *
     *  Requires that the database is open for writing.  Doesn't commit the
     *  changes.
     */
    void write_config();

    /// Copying not allowed.
    Collection(const Collection &);
    /// Assignment not allowed.
    void operator=(const Collection &);

  public:
    Collection(const std::string & coll_name_,
	       const std::string & coll_path_);
    ~Collection();

    /** Get the name of the collection.
     */
    const std::string & get_name() const {
	return config.get_name();
    }

    /** Get the configuration object for the collection.
     */
    const CollectionConfig & get_config() const {
	return config;
    }

    /** Open the collection for writing.
     */
    void open_writable();

    /** Open the collection for reading.
     *
     *  Will reopen the collection, to point at the latest data, if it's
     *  already open.
     */
    void open_readonly();

    /** Close the collection.
     */
    void close() {
	group.close();
    }

    /** Return true iff the collection is open for writing.
     */
    bool is_writable() {
	return group.is_writable();
    }

    /** Return true iff the collection is open.
     */
    bool is_open() {
	return group.is_open();
    }

    /** Get the schema for a given type.
     *
     *  Raises an exception if the type is not known.
     *
     *  The returned reference is invalid after modifications have been made
     *  to the collection's schema.
     */
    Schema & get_schema(const std::string & type);

    /** Set the schema for a given type.
     *
     *  Takes a copy of the supplied schema.
     */
    void set_schema(const std::string & type,
		    const Schema & schema);


    /** Get an input pipe.
     *
     *  Raises an exception if the pipe is not known.
     *
     *  The returned reference is invalid after modifications have been made
     *  to the collection's pipe configuration.
     */
    const Pipe & get_pipe(const std::string & pipe_name) const;

    /** Set an input pipe.
     *
     *  Takes a copy of the supplied pipe.
     */
    void set_pipe(const std::string & pipe_name,
		  const Pipe & pipe);


    /** Get a categoriser.
     *
     *  Raises an exception if the categoriser is not known.
     *
     *  The returned reference is invalid after modifications have been made
     *  to the collection's categoriser configuration.
     */
    const Categoriser & get_categoriser(const std::string & categoriser_name) const;

    /** Set a categoriser.
     *
     *  Takes a copy of the supplied categoriser.
     */
    void set_categoriser(const std::string & categoriser_name,
			 const Categoriser & categoriser);


    /** Get a CategoryHierarchy.
     *
     *  Returns NULL if the hierarchy is not known.
     *
     *  The returned pointer is invalid after modifications have been made
     *  to the collection's categoriser configuration.
     */
    const CategoryHierarchy *
	    get_category(const std::string & category_name) const;

    /** Set a category hierarchy.
     *
     *  Takes a copy of the supplied category hierarchy.
     */
    void set_category(const std::string & category_name,
		      const CategoryHierarchy & category);


    /** Convert the collection configuration to JSON.
     *
     *  Returns a reference to the value supplied, to allow easier use inline.
     */
    Json::Value & to_json(Json::Value & value) const {
	return config.to_json(value);
    }

    /** Set the collection configuration from JSON.
     *
     *  Wipes out any previous configuration.
     */
    void from_json(const Json::Value & value);

    /** Categorise a piece of text.
     *
     *  @param categoriser_name The categoriser to use.
     *  @param text The text to categorise.
     *  @param result A JSON object to store the result in.
     *
     *  The result is returned as a JSON array of strings, with the best
     *  matching category first.
     *
     *  Returns a reference to `result`, for easier use in subsequent
     *  operations.
     */
    Json::Value & categorise(const std::string & categoriser_name,
			     const std::string & text,
			     Json::Value & result) const;

    /** Send a document, as a JSON object, to an input pipe.
     */
    void send_to_pipe(TaskManager * taskman,
		      const std::string & pipe_name,
		      Json::Value & obj);

    /** Add (or replace) a document.
     */
    void add_doc(Json::Value & doc_obj,
		 const std::string & doc_type);

    /** Process a JSON document into a Xapian document.
     */
    Xapian::Document process_doc(Json::Value & doc_obj,
				 const std::string & doc_type,
				 const std::string & doc_id,
				 std::string & idterm);

    /** Update (or add) a Xapian document, given its unique id term.
     */
    void raw_update_doc(const Xapian::Document & doc,
			const std::string & idterm);

    /** Delete a Xapian document, given its unique id term.
     */
    void raw_delete_doc(const std::string & idterm);

    /** Commit any pending changes to the database.
     */
    void commit();

    /** Get the total number of documents.
     */
    uint64_t doc_count() const;

    /** Perform a search.
     */
    void perform_search(const Json::Value & search,
			const std::string & doc_type,
			Json::Value & results) const;

    /** Get a set of stored fields from a Xapian document.
     */
    void get_doc_fields(const Xapian::Document & doc,
			const std::string & doc_type,
			const Json::Value & fieldlist,
			Json::Value & result) const;

    /** Get a JSON representation of a document, given its ID.
     */
    void get_document(const std::string & doc_type,
		      const std::string & docid,
		      Json::Value & result) const;
};

}

#endif /* RESTPOSE_INCLUDED_COLLECTION_H */
