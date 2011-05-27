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
#include <string>
#include <xapian.h>

namespace RestPose {
    // Forward declaration
    class FieldIndexer;

    /** The configuration for an individual field in the schema.
     */
    struct FieldConfig {
	/// Create an indexer for the field.
	virtual FieldIndexer * indexer() const = 0;

	/// Create a query to search this field.
	virtual Xapian::Query query(const std::string & qtype,
				    const Json::Value & value) const = 0;

	/// Add the configuration for a field to a JSON object.
	virtual void to_json(Json::Value & value) const = 0;

	/// Create a new FieldConfig from a JSON object.
	static FieldConfig * from_json(const Json::Value & value);

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

	/// Create from a JSON object.
	IDFieldConfig(const Json::Value & value);

	/// Create from parameters.
	IDFieldConfig(unsigned int max_length_ = 64,
		      MaxLenFieldConfig::TooLongAction too_long_action_ = TOOLONG_ERROR,
		      const std::string & store_field_ = std::string())
		: MaxLenFieldConfig(max_length_, too_long_action_),
		  store_field(store_field_)
	{}

	virtual ~IDFieldConfig();

	/// Create an indexer for the field.
	FieldIndexer * indexer() const;

	/// Create a query to search this field.
	Xapian::Query query(const std::string & qtype,
			    const Json::Value & value) const;

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

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;

      private:
	Xapian::Query query_phrase(const Json::Value & qparams) const;
	Xapian::Query query_parse(const Json::Value & qparams) const;
    };

    struct DateFieldConfig : public FieldConfig {
	/// The prefix to use for the field.
	unsigned int slot;

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

	/// Add the configuration for a field to a JSON object.
	void to_json(Json::Value & value) const;
    };

    /** A schema, containing the configuration for a set of fields.
     */
    class Schema {
	/** A mapping from fieldname to configuration for that field.
	 */
	std::map<std::string, FieldConfig *> fields;

	/** Cache of mappings from fieldname to indexer for a field.
	 */
	mutable std::map<std::string, FieldIndexer *> indexers;

	/// Copying not allowed.
	Schema(const Schema &);

	/// Assignment not allowed.
	void operator=(const Schema &);

	/// Build a Xapian query from a JSON query structure.
	Xapian::Query build_query(const Xapian::Database & db,
				  const Json::Value & query) const;

	/// Get the list of fields to return, from a search
	void get_fieldlist(Json::Value & result,
			   const Json::Value & search) const;

      public:
	Schema() {}

	/// Destructor - frees the FieldConfig objects owned by the schema.
	~Schema();

	/// Clear the schema.
	void clear();

	/// Convert the schema to a JSON object.
	Json::Value & to_json(Json::Value & value) const;

	/// Cnvert the schema to a string encoding of a JSON object.
	std::string to_json_string() const;

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
	 *  @param doc The document to store terms and values in.
	 */
	Xapian::Document process(const Json::Value & value,
				 std::string & idterm) const;

	/** Perform a search.
	 */
	void perform_search(const Xapian::Database & db,
			    const Json::Value & search,
			    Json::Value & results) const;

	/** Perform a search, given a JSON encoded search.
	 */
	void perform_search(const Xapian::Database & db,
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