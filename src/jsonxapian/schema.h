/** @file schema.h
 * @brief A search schema.
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

#ifndef RESTPOSE_INCLUDED_SCHEMA_H
#define RESTPOSE_INCLUDED_SCHEMA_H

#include <cstdio>
#include "json/value.h"
#include <map>
#include "jsonxapian/slotname.h"
#include <string>
#include <xapian.h>

namespace RestPose {
    // Forward declaration
    class CollectionConfig;
    class FieldIndexer;
    class IndexingErrors;

    /** The configuration for an individual field in the schema.
     */
    struct FieldConfig {
	/// Create an indexer for the field.
	virtual FieldIndexer * indexer() const = 0;

	/// Create a query to search this field.
	virtual Xapian::Query query(const std::string & qtype,
				    const Json::Value & value) const = 0;

	/// Get the field that values are being stored under. ("" if none).
	virtual std::string stored_field() const = 0;

	/// Add the configuration for a field to a JSON object.
	virtual void to_json(Json::Value & value) const = 0;

	/// Create a new FieldConfig from a JSON object.
	static FieldConfig * from_json(const Json::Value & value,
				       const std::string & doc_type);

	/// Virtual destructor, to clean up subclasses correctly.
	virtual ~FieldConfig();

      protected:
	/// Default constructor.
	FieldConfig() {}

      private:
	/// Copying not allowed.
	FieldConfig(const FieldConfig &);

	/// Assignment not allowed.
	void operator=(const FieldConfig &);
    };

    /** Configuration for the Meta field.
     */
    struct MetaFieldConfig : public FieldConfig {
	/// The prefix to use for the field.
	std::string prefix;

	/// The slot to use for the field.
	SlotName slot;

	/// Create from a JSON object.
	MetaFieldConfig(const Json::Value & value);

	virtual ~MetaFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return std::string();
	}

	/// Add the configuration for the field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    /** Configuration for fields which have a maximum length.
     */
    struct MaxLenFieldConfig : public FieldConfig {
	/// An action to take if the term is too long.
	enum TooLongAction {
	    TOOLONG_HASH,
	    TOOLONG_TRUNCATE,
	    TOOLONG_ERROR
	};

	/// The maximum length allowed for field values.
	unsigned int max_length;

	/// The action to take if the term is too long.
	TooLongAction too_long_action;

	/// Create from a JSON object.
	MaxLenFieldConfig(const Json::Value & value);

	/// Create from parameters.
	MaxLenFieldConfig(unsigned int max_length_,
			  MaxLenFieldConfig::TooLongAction too_long_action_)
		: max_length(max_length_),
		  too_long_action(too_long_action_)
	{}

	/// Virtual destructor, to clean up subclasses correctly.
	virtual ~MaxLenFieldConfig();

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    /** The configuration for an ID field.
     */
    struct IDFieldConfig : public MaxLenFieldConfig {
	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// The prefix to use for this field.
	std::string prefix;

	/// Create from a JSON object.
	IDFieldConfig(const Json::Value & value, const std::string & doc_type);

	/// Create from parameters.
	IDFieldConfig(const std::string & doc_type,
		      unsigned int max_length_ = 64,
		      MaxLenFieldConfig::TooLongAction too_long_action_ = TOOLONG_ERROR,
		      const std::string & store_field_ = std::string());

	virtual ~IDFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct ExactFieldConfig : public MaxLenFieldConfig {
	/// The prefix to use for the field.
	std::string prefix;

	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// The wdf increment to perform for each field value.
	unsigned int wdfinc;

	/// Create from a JSON object.
	ExactFieldConfig(const Json::Value & value);

	/// Create from parameters.
	ExactFieldConfig(const std::string & prefix_,
			 unsigned int max_length_,
			 MaxLenFieldConfig::TooLongAction action_,
			 const std::string & store_field_,
			 unsigned int wdfinc_)
		: MaxLenFieldConfig(max_length_, action_),
		  prefix(prefix_ + "\t"),
		  store_field(store_field_),
		  wdfinc(wdfinc_)
	{}

	virtual ~ExactFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct TextFieldConfig : public FieldConfig {
	/// The prefix to use for the field.
	std::string prefix;

	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// The processor to use for the contents of the field.
	std::string processor;

	/// Create from a JSON object.
	TextFieldConfig(const Json::Value & value);

	/// Create from parameters.
	TextFieldConfig(const std::string & prefix_,
			const std::string & store_field_,
			const std::string & processor_)
		: prefix(prefix_ + "\t"),
		  store_field(store_field_),
		  processor(processor_)
	{}

	virtual ~TextFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;

      private:
	Xapian::Query query_phrase(const Json::Value & qparams) const;
	Xapian::Query query_parse(const Json::Value & qparams) const;
    };

    struct TimestampFieldConfig : public FieldConfig {
	/// The slot to use for the field.
	SlotName slot;

	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// Create from a JSON object.
	TimestampFieldConfig(const Json::Value & value);

	/// Create from parameters.
	TimestampFieldConfig(unsigned int slot_,
			const std::string & store_field_)
		: slot(slot_),
		  store_field(store_field_)
	{}

	virtual ~TimestampFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct DateFieldConfig : public FieldConfig {
	/// The slot to use for the field.
	SlotName slot;

	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// Create from a JSON object.
	DateFieldConfig(const Json::Value & value);

	/// Create from parameters.
	DateFieldConfig(unsigned int slot_,
			const std::string & store_field_)
		: slot(slot_),
		  store_field(store_field_)
	{}

	virtual ~DateFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct CategoryFieldConfig : public MaxLenFieldConfig {
	/// The prefix that the category terms are stored under.
	std::string prefix;

	/** The name of the hierarchy that this category uses.
	 *
	 *  Currently, this is just the prefix (without the tab separator),
	 *  since the hierarchy needs to know the prefix used in order to
	 *  update the appropriate documents when the hierarchy is modified,
	 *  and just using the prefix as the hierarchy name is the easiest way
	 *  to do this.
	 */
	std::string hierarchy_name;

	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// Create from a JSON object.
	CategoryFieldConfig(const Json::Value & value);

	/// Create from parameters.
	CategoryFieldConfig(std::string prefix_,
			    unsigned int max_length_ = 64,
			    MaxLenFieldConfig::TooLongAction too_long_action_ = TOOLONG_ERROR,
			    const std::string & store_field_ = std::string())
		: MaxLenFieldConfig(max_length_, too_long_action_),
		  prefix(prefix_ + "\t"),
		  store_field(store_field_)
	{}

	virtual ~CategoryFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct StoredFieldConfig : public FieldConfig {
	/// The fieldname to store field values under (empty to not store).
	std::string store_field;

	/// Create from a JSON object.
	StoredFieldConfig(const Json::Value & value);

	/// Create from parameters.
	StoredFieldConfig(const std::string & store_field_)
		: store_field(store_field_)
	{}

	virtual ~StoredFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return store_field;
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    struct IgnoredFieldConfig : public FieldConfig {
	virtual ~IgnoredFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

	/// Get the field that values are being stored under.
	std::string stored_field() const {
	    return std::string();
	}

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };


    /** An individual pattern to apply.
     */
    class FieldConfigPattern {
	bool leading_wildcard;
	std::string ending;
	Json::Value config;
      public:
	FieldConfigPattern();

	/** Set the pattern from a JSON object */
	void from_json(const Json::Value & value);

	/** Convert the pattern to a JSON object.
	 *
	 *  Returns the reference passed in.
	 */
	Json::Value & to_json(Json::Value & value) const;

	/** Test if this pattern matches.
	 *
	 *  If the pattern matches, make and return a new FieldConfig using it.
	 *  Otherwise, return NULL.
	 */
	FieldConfig * test(const std::string & fieldname,
			   const std::string & doc_type) const;
    };

    /** An ordered list of patterns to apply to configure unknown fields.
     */
    class FieldConfigPatterns {
	/** The list of patterns.
	 */
	std::vector<FieldConfigPattern> patterns;

      public:
	FieldConfigPatterns();

	/** Set the patterns from a JSON object.
	 */
	void from_json(const Json::Value & value);

	/** Convert the pattern to a JSON object.
	 *
	 *  Returns the reference passed in.
	 */
	Json::Value & to_json(Json::Value & value) const;

	/** Merge this list of patterns with another list.
	 *
	 *  For now, a very simple algorithm for merging is used - it "other"
	 *  contains any patterns, its list is used, otherwise the patterns in
	 *  this object are left unchanged.
	 */
	void merge_from(const FieldConfigPatterns & other);

	/** Get a new FieldConfig for the fieldname.
	 *
	 *  If a pattern matches, this will return a new FieldConfig using it.
	 *  If no patterns match, this will return NULL.
	 */
	FieldConfig * get(const std::string & fieldname,
			  const std::string & doc_type) const;
    };

    /** A schema, containing the configuration for a set of fields.
     */
    class Schema {
	/** The type that this schema is for.
	 */
	std::string doc_type;

	/** A mapping from fieldname to configuration for that field.
	 */
	std::map<std::string, FieldConfig *> fields;

	/** Cache of mappings from fieldname to indexer for a field.
	 */
	mutable std::map<std::string, FieldIndexer *> indexers;

	FieldConfigPatterns patterns;

	/// Copying not allowed.
	Schema(const Schema &);

	/// Assignment not allowed.
	void operator=(const Schema &);

	/// Build a Xapian query from a JSON query structure.
	Xapian::Query build_query(const CollectionConfig & collconfig,
				  const Xapian::Database & db,
				  const Json::Value & query) const;

	/// Get the list of fields to return, from a search
	void get_fieldlist(Json::Value & result,
			   const Json::Value & search) const;

      public:
	Schema(const std::string & doc_type_) : doc_type(doc_type_) {}

	/// Destructor - frees the FieldConfig objects owned by the schema.
	~Schema();

	/// Clear the schema.
	void clear();

	/// Convert the schema to a JSON object.
	Json::Value & to_json(Json::Value & value) const;

	/// Initialise the schema from a JSON object.
	void from_json(const Json::Value & value);

	/** Merge settings from another schema.
	 *
	 *  Raises an exception if any incompatible changes to a field's
	 *  configuration are required.
	 */
	void merge_from(const Schema & other);

	/** Get a pointer to the config for a field.
	 *
	 *  Returns a borrowed reference.
	 *
	 *  Returns NULL if the field has no config stored.
	 */
	const FieldConfig * get(const std::string & fieldname) const;

	/** Get a pointer to the indexer for a field.
	 *
	 *  Returns a borrowed reference.
	 *
	 *  Returns NULL if there is no indexer for the field.
	 */
	const FieldIndexer * get_indexer(const std::string & fieldname) const;

	/** Set the field config for a field.
	 *
	 *  Takes ownership of the supplied configuration.
	 */
	void set(const std::string & fieldname, FieldConfig * config);

        /** Process a JSON object into a Xapian document.
	 *
	 *  If previously unknown fields are found in the document, the
	 *  patterns will be used to look for appropriate configuration for
	 *  those fields, which will then be added to the schema.
	 *
	 *  @param doc The document to store terms and values in.
	 */
	Xapian::Document process(const Json::Value & value,
	    const CollectionConfig & collconfig,
	    std::string & idterm,
	    IndexingErrors & errors);

	/** Perform a search.
	 */
	void perform_search(const CollectionConfig & collconfig,
			    const Xapian::Database & db,
			    const Json::Value & search,
			    Json::Value & results) const;

	/** Perform a search, given a JSON encoded search.
	 */
	void perform_search(const CollectionConfig & collconfig,
			    const Xapian::Database & db,
			    const std::string & search_str,
			    Json::Value & results) const;

	/** Get a set of stored fields from a Xapian document.
	 */
	void display_doc(const Xapian::Document & doc,
			 const Json::Value & fieldlist,
			 Json::Value & result) const;

	/** Get all stored fields from a Xapian document.
	 */
	void display_doc(const Xapian::Document & doc,
			 Json::Value & result) const;

	/** Get all stored fields from a Xapian document as a string.
	 */
	std::string display_doc_as_string(const Xapian::Document & doc,
					  const Json::Value & fieldlist) const;

	/** Get a set of stored fields from a Xapian document as a string.
	 */
	std::string display_doc_as_string(const Xapian::Document & doc) const;
    };
};

#endif /* RESTPOSE_INCLUDED_SCHEMA_H */
