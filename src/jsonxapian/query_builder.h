/** @file query_builder.h
 * @brief Classes used to build Xapian Queries.
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

#ifndef RESTPOSE_INCLUDED_QUERY_BUILDER_H
#define RESTPOSE_INCLUDED_QUERY_BUILDER_H

#include "json/value.h"
#include "jsonxapian/slotname.h"
#include <xapian.h>

namespace RestPose {
    class Collection;
    class CollectionConfig;
    class FieldConfig;
    class Schema;
    class SlotDecoder;

    /** Base class of query builders.
     *
     *  Implements all the query operator interpretations, but doesn't know how
     *  to build the leaf queries.
     */
    class QueryBuilder {
      protected:
	/** The configuration for the collection being searched.. */
	const CollectionConfig & collconfig;

	/** Build a query from a JSON query specification.
	 */
	Xapian::Query build_query(const Json::Value & jsonquery) const;

	/** Build a query for a particular field.
	 */
	virtual Xapian::Query
		field_query(const std::string & fieldname,
			    const std::string & querytype,
			    const Json::Value & queryparams) const = 0;

      public:
	QueryBuilder(const CollectionConfig & collconfig_);

	/** Build a query from a JSON query specification.
	 */
	virtual Xapian::Query build(const Json::Value & jsonquery) const = 0;

	/** Get the total number of documents searched in the database
	 *  specified by queries built by this builder.
	 */
	virtual Xapian::doccount
		total_docs(const Xapian::Database & db) const = 0;

	/** Get the config for a given field.
	 *
	 *  If the configuration for the field varies for different document
	 *  types, an arbitrary one of the configurations is used.
	 *
	 *  Returns NULL if there is no config for the field.
	 */
	virtual const FieldConfig *
		get_field_config(const std::string & fieldname) const = 0;

	/** Get a slot decoder to return the values stored in a given field.
	 *
	 *  Raise an exception if the slot used is inconsistent for the types
	 *  that the query builder is for.
	 */
	virtual SlotDecoder *
		get_slot_decoder(const std::string & fieldname) const = 0;
    };

    /** A query builder for searches across a whole collection.
     *
     *  Implements leaf queries which search the whole collection.
     */
    class CollectionQueryBuilder : public QueryBuilder {
	/** Build a query for a particular field.
	 */
	Xapian::Query field_query(const std::string & fieldname,
				  const std::string & querytype,
				  const Json::Value & queryparams) const;

      public:
	CollectionQueryBuilder(const CollectionConfig & collconfig_);

	/** Build a query from a JSON query specification.
	 */
	Xapian::Query build(const Json::Value & jsonquery) const;

	Xapian::doccount total_docs(const Xapian::Database & db) const;

	const FieldConfig *
		get_field_config(const std::string & fieldname) const;

	SlotDecoder * get_slot_decoder(const std::string & fieldname) const;
    };

    /** A query builder for searching a particular document type.
     *
     *  Implements leaf queries which search all documents of a given type in
     *  the whole collection.
     */
    class DocumentTypeQueryBuilder : public QueryBuilder {
	const Schema * schema;

	/** Build a query for a particular field.
	 */
	Xapian::Query field_query(const std::string & fieldname,
				  const std::string & querytype,
				  const Json::Value & queryparams) const;

      public:
	DocumentTypeQueryBuilder(const CollectionConfig & collconfig_,
				 const std::string & doc_type);

	Xapian::Query build(const Json::Value & jsonquery) const;

	Xapian::doccount total_docs(const Xapian::Database & db) const;

	const FieldConfig *
		get_field_config(const std::string & fieldname) const;

	SlotDecoder * get_slot_decoder(const std::string & fieldname) const;
    };
};

#endif /* RESTPOSE_INCLUDED_QUERY_BUILDER_H */
