/** @file indexing.h
 * @brief Routines used for indexing.
 */
/* Copyright (c) 2010 Richard Boulton
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

#ifndef RESTPOSE_INCLUDED_INDEXING_H
#define RESTPOSE_INCLUDED_INDEXING_H

#include "jsonxapian/docdata.h"
#include "json/value.h"
#include <map>
#include <string>
#include <xapian.h>

#include "utils/rsperrors.h"
#include "schema.h"

namespace RestPose {
    class CollectionConfig;

    /** Information about the presence of a field in a document.
     */
    struct FieldPresence {
	/// True if the field has a non-empty occurrence.
	bool nonempty;

	/// True if the field has an empty occurrence.
	bool empty;

	/// True if the field has an occurrence which produced an error.
	bool errors;
    };

    struct IndexingErrors {
	/** Errors occurring while indexing.
	 *
	 *  First item in each pair is the fieldname, next item is the error
	 *  message.
	 */
	std::vector<std::pair<std::string, std::string> > errors;

	/** Flag indicating if the error is a total failure to process the
	 * document (if true), or a failure of processing part of the document
	 * (if false).
	 */
	bool total_failure;

	IndexingErrors() : total_failure(false) {}

	void append(const std::string & fieldname,
		    const std::string & error) {
	    errors.push_back(std::pair<std::string, std::string>
			     (fieldname, error));
	}
    };

    /** Container for the state when indexing a document.
     */
    struct IndexingState {
	/** The Xapian document holding the result of indexing.
	 */
	Xapian::Document doc;

	/** The document data fields, which will be stored in the Xapian
	 *  document data.
	 */
	DocumentData docdata;

	/** Fields which are present in a document.
	 */
	std::map<std::string, FieldPresence> presence;

	/** The configuration of the collection.
	 */
	const CollectionConfig & collconfig;

	/** The term representing the document ID.
	 *
	 *  This gets set when processing the document ID field.
	 */
	std::string & idterm;

	/** Errors produced when processing the document.
	 *
	 *  First item in the pair is the fieldname, second item is the
	 *  error string.
	 */
	IndexingErrors & errors;


	IndexingState(const CollectionConfig & collconfig_,
		      std::string & idterm_,
		      IndexingErrors & errors_)
		: collconfig(collconfig_),
		  idterm(idterm_),
		  errors(errors_)
	{
	    idterm.resize(0);
	}

	/** Set the idterm.
	 *
	 *  If the idterm is already set, this doesn't alter it, and instead
	 *  records an error.
	 */
	void set_idterm(const std::string & fieldname,
			const std::string & idterm_);

	/** Register a field as being present, and having an empty value.
	 */
	void field_empty(const std::string & fieldname) {
	    presence[fieldname].empty = true;
	}

	/** Register a field as being present, and having a non-empty value.
	 */
	void field_nonempty(const std::string & fieldname) {
	    presence[fieldname].nonempty = true;
	}

	/** Log an error having occurred when processing a field.
	 */
	void append_error(const std::string & fieldname,
			  const std::string & error) {
	    presence[fieldname].errors = true;
	    errors.append(fieldname, error);
	}
    };

    /** Base class for field indexers: these process a JSON value and store
     *  the result in a field.
     */
    class FieldIndexer {
      public:
	/** Process a field into a document.
	 *
	 *  This method should store terms and values in the document supplied.
	 *
	 *  If a value should be stored for the field, the method should set the docdata
	 *  appropriately.
	 *
	 *  If the field is an ID field, it should set the idterm appropriately.
	 *
	 *  @param state
	 *  @param doc The document to store terms and values in.
	 *  @param docdata The document data (may be modified by the method).
	 *  @param value The JSON value held in the field.
	 *  @param idterm A string which may be set to the document ID.
	 */
	virtual void index(IndexingState & state,
			   const std::string & fieldname,
			   const Json::Value & values) const = 0;
	virtual ~FieldIndexer();
    };

    /** A field indexer for the metadata field.
     */
    class MetaIndexer : public FieldIndexer {
	std::string prefix;
	unsigned int slot;
      public:
	MetaIndexer(const std::string & prefix_,
		    unsigned int slot_)
		: prefix(prefix_), slot(slot_)
	{}

	virtual ~MetaIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };

    /** A field indexer which expects a single string as input, and indexes it
     *  with a prefix.
     */
    class ExactStringIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
	unsigned int wdfinc;
	unsigned int max_length;
	MaxLenFieldConfig::TooLongAction too_long_action;
	bool isid;
      public:
	ExactStringIndexer(const std::string & prefix_,
			   const std::string & store_field_,
			   unsigned int wdfinc_,
			   unsigned int max_length_,
			   MaxLenFieldConfig::TooLongAction too_long_action_,
			   bool isid_)
		: prefix(prefix_), store_field(store_field_), wdfinc(wdfinc_),
		  max_length(max_length_), too_long_action(too_long_action_),
		  isid(isid_)
	{}

	virtual ~ExactStringIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };

    /** A field indexer which expects a single string as input, and stores it.
     */
    class StoredIndexer : public FieldIndexer {
	std::string store_field;
      public:
	StoredIndexer(const std::string & store_field_)
		: store_field(store_field_)
	{}

	virtual ~StoredIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };


    /** A field indexer which expects a number representing seconds since 1970.
     */
    class TimeStampIndexer : public FieldIndexer {
	unsigned int slot;
	std::string store_field;
      public:
	TimeStampIndexer(unsigned int slot_,
			 const std::string & store_field_)
		: slot(slot_), store_field(store_field_)
	{}

	virtual ~TimeStampIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };


    /** A field indexer which expects a date.
     *
     *  The date must be a string in the format YYYY-MM-DD.
     *
     *  The year part may be less than (or more than) 4 digits, and may be
     *  prefixed with a hyphen to indicate a negative year.  The calendar used
     *  is undefined, but would usually be an extrapolated Gregorian calendar
     *  in which the year 0 does exist.
     */
    class DateIndexer : public FieldIndexer {
	unsigned int slot;
	std::string store_field;
      public:
	DateIndexer(unsigned int slot_,
		    const std::string & store_field_)
		: slot(slot_), store_field(store_field_)
	{}

	virtual ~DateIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;

	/** Parse a date into the form in which it is stored in the slot.
	 */
	static std::string parse_date(const Json::Value & value,
				      std::string & error);
    };


    /** A field indexer which expects a category.
     *
     *  The category and all the parents of the category are stored in terms,
     *  allowing quick searches for all documents in a category, or in a
     *  descendent of a category.
     */
    class CategoryIndexer : public FieldIndexer {
	std::string prefix;
	std::string hierarchy_name;
	std::string store_field;
	unsigned int max_length;
	MaxLenFieldConfig::TooLongAction too_long_action;
      public:
	CategoryIndexer(const std::string & prefix_,
			const std::string & hierarchy_name_,
			const std::string & store_field_,
			unsigned int max_length_,
			MaxLenFieldConfig::TooLongAction too_long_action_)
		: prefix(prefix_),
		  hierarchy_name(hierarchy_name_),
		  store_field(store_field_),
		  max_length(max_length_), too_long_action(too_long_action_)
	{}

	virtual ~CategoryIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };


    /** A field indexer which expects an array of strings as input, and
     *  indexes them using the Xapian TermGenerator.
     */
    class TermGeneratorIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
	std::string stem_lang;
      public:
	TermGeneratorIndexer(const std::string & prefix_,
			     const std::string & store_field_,
			     const std::string & stem_lang_)
		: prefix(prefix_),
		  store_field(store_field_),
		  stem_lang(stem_lang_)
	{}

	virtual ~TermGeneratorIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };

    /** A field indexer which expects an array of strings as input, and
     *  indexes them as CJK text.
     */
    class CJKIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
      public:
	CJKIndexer(const std::string & prefix_,
		   const std::string & store_field_)
		: prefix(prefix_), store_field(store_field_)
	{}

	virtual ~CJKIndexer();

	void index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const;
    };

};

#endif /* RESTPOSE_INCLUDED_INDEXING_H */
