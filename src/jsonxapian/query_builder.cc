/** @file query_builder.cc
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

#include <config.h>
#include "jsonxapian/query_builder.h"

#include "jsonxapian/collection.h"
#include "jsonxapian/schema.h"
#include "jsonxapian/slotname.h"
#include "logger/logger.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <vector>
#include <xapian.h>

using namespace RestPose;
using namespace std;

Xapian::Query
QueryBuilder::build_query(const Json::Value & jsonquery) const
{
    if (jsonquery.isNull()) {
	return Xapian::Query::MatchNothing;
    }
    json_check_object(jsonquery, "query tree");
    if (jsonquery.size() == 0) {
	return Xapian::Query::MatchNothing;
    }

    if (jsonquery.isMember("matchall")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("MatchAll query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["matchall"];
	if (!queryparams.isBool() || queryparams != true) {
	    throw InvalidValueError("MatchAll query expects a value of true");
	}
	return Xapian::Query::MatchAll;
    }

    if (jsonquery.isMember("matchnothing")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("MatchNothing query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["matchnothing"];
	if (!queryparams.isBool() || queryparams != true) {
	    throw InvalidValueError("MatchNothing query expects a value of true");
	}
	return Xapian::Query::MatchNothing;
    }

    if (jsonquery.isMember("field")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("Field query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["field"];
	json_check_array(queryparams, "field search parameters");
	if (queryparams.isNull()) {
	    throw InvalidValueError("Invalid parameters for field search - null");
	}
	if (queryparams.size() != 3) {
	    throw InvalidValueError("Invalid parameters for field search - length != 3");
	}
	if (!(queryparams[Json::UInt(0u)].isString())) {
	    throw InvalidValueError("Invalid fieldname for field search - not a string");
	}
	if (!(queryparams[Json::UInt(1u)].isString())) {
	    throw InvalidValueError("Invalid type in field search - not a string");
	}

	string fieldname = queryparams[Json::UInt(0u)].asString();
	string querytype = queryparams[Json::UInt(1u)].asString();
	return field_query(fieldname, querytype, queryparams[2u]);
    }

    if (jsonquery.isMember("meta")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("Meta query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["meta"];
	json_check_array(queryparams, "Meta search parameters");
	if (queryparams.isNull()) {
	    throw InvalidValueError("Invalid parameters for meta search - null");
	}
	if (queryparams.size() != 2) {
	    throw InvalidValueError("Invalid parameters for meta search - length != 2");
	}
	if (!(queryparams[Json::UInt(0u)].isString())) {
	    throw InvalidValueError("Invalid type in meta search - not a string");
	}

	string fieldname = collconfig.get_meta_field();
	string querytype = queryparams[Json::UInt(0u)].asString();
	return field_query(fieldname, querytype, queryparams[1u]);
    }

    if (jsonquery.isMember("and")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("AND query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["and"];
	json_check_array(queryparams, "AND search parameters");
	vector<Xapian::Query> queries;
	queries.reserve(queryparams.size());

	for (Json::Value::const_iterator i = queryparams.begin();
	     i != queryparams.end(); ++i) {
	    queries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_AND,
			     queries.begin(), queries.end());
    }

    if (jsonquery.isMember("or")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("OR query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["or"];
	json_check_array(queryparams, "OR search parameters");
	vector<Xapian::Query> queries;
	queries.reserve(queryparams.size());

	for (Json::Value::const_iterator i = queryparams.begin();
	     i != queryparams.end(); ++i) {
	    queries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_OR,
			     queries.begin(), queries.end());
    }

    if (jsonquery.isMember("xor")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("XOR query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["xor"];
	json_check_array(queryparams, "XOR search parameters");
	vector<Xapian::Query> queries;
	queries.reserve(queryparams.size());

	for (Json::Value::const_iterator i = queryparams.begin();
	     i != queryparams.end(); ++i) {
	    queries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_XOR,
			     queries.begin(), queries.end());
    }

    if (jsonquery.isMember("and_not")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("NOT query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["and_not"];
	json_check_array(queryparams, "NOT search parameters");
	if (queryparams.size() < 2) {
	    throw InvalidValueError("Not query must contain at least two subqueries");
	}

	Json::Value::const_iterator i = queryparams.begin();
	Xapian::Query posquery(build_query(*i));
	++i;

	vector<Xapian::Query> negqueries;
	negqueries.reserve(queryparams.size() - 1);

	for (; i != queryparams.end(); ++i) {
	    negqueries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_AND_NOT,
			     posquery,
			     Xapian::Query(Xapian::Query::OP_OR,
					   negqueries.begin(), negqueries.end()));
    }

    if (jsonquery.isMember("and_maybe")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("AND_MAYBE query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["and_maybe"];
	json_check_array(queryparams, "AND_MAYBE search parameters");
	if (queryparams.size() < 2) {
	    throw InvalidValueError("AND_MAYBE query must contain at least two subqueries");
	}

	Json::Value::const_iterator i = queryparams.begin();
	Xapian::Query mainquery(build_query(*i));
	++i;

	vector<Xapian::Query> maybequeries;
	maybequeries.reserve(queryparams.size() - 1);

	for (; i != queryparams.end(); ++i) {
	    maybequeries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_AND_MAYBE,
			     mainquery,
			     Xapian::Query(Xapian::Query::OP_OR,
					   maybequeries.begin(), maybequeries.end()));
    }

    if (jsonquery.isMember("filter")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("FILTER query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["filter"];
	json_check_array(queryparams, "FILTER search parameters");
	if (queryparams.size() < 2) {
	    throw InvalidValueError("FILTER query must contain at least two subqueries");
	}

	Json::Value::const_iterator i = queryparams.begin();
	Xapian::Query mainquery(build_query(*i));
	++i;

	vector<Xapian::Query> filterqueries;
	filterqueries.reserve(queryparams.size() - 1);

	for (; i != queryparams.end(); ++i) {
	    filterqueries.push_back(build_query(*i));
	}
	return Xapian::Query(Xapian::Query::OP_FILTER,
			     mainquery,
			     Xapian::Query(Xapian::Query::OP_AND,
					   filterqueries.begin(), filterqueries.end()));
    }

    if (jsonquery.isMember("scale")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("Scale query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["scale"];

	if (!queryparams.isMember("query")) {
	    throw InvalidValueError("Scale query must contain a query member");
	}
	Xapian::Query subquery = build_query(queryparams["query"]);
	return Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, subquery,
	    json_get_double_member(queryparams, "factor", 0.0));
    }

    throw InvalidValueError("Invalid query specification - no known members in query object (" + json_serialise(jsonquery) + ")");
}

QueryBuilder::QueryBuilder(const CollectionConfig & collconfig_)
	: collconfig(collconfig_)
{
}


CollectionQueryBuilder::CollectionQueryBuilder(
    const CollectionConfig & collconfig_)
	: QueryBuilder(collconfig_)
{
}

Xapian::Query
CollectionQueryBuilder::field_query(const std::string & fieldname,
				    const std::string & querytype,
				    const Json::Value & queryparams) const
{
    vector<Xapian::Query> queries;

    for (map<string, Schema *>::const_iterator i = collconfig.schema_begin();
	 i != collconfig.schema_end(); ++i)
    {
	const FieldConfig * config = i->second->get(fieldname);
	if (config != NULL) {
	    queries.push_back(config->query(querytype, queryparams));
	}
    }

    return Xapian::Query(Xapian::Query::OP_OR,
			 queries.begin(), queries.end());
}

Xapian::Query
CollectionQueryBuilder::build(const Json::Value & jsonquery) const
{
    return build_query(jsonquery);
}

Xapian::doccount
CollectionQueryBuilder::total_docs(const Xapian::Database & db) const
{
    return db.get_doccount();
}

const FieldConfig *
CollectionQueryBuilder::get_field_config(const std::string & fieldname) const
{
    for (map<string, Schema *>::const_iterator i = collconfig.schema_begin();
	 i != collconfig.schema_end(); ++i)
    {
	const FieldConfig * config = i->second->get(fieldname);
	if (config != NULL) {
	    return config;
	}
    }
    return NULL;
}

SlotDecoder *
CollectionQueryBuilder::get_slot_decoder(const std::string & fieldname) const
{
    ValueEncoding encoding(ENC_VINT_LENGTHS);
    Xapian::valueno slot(Xapian::BAD_VALUENO);

    for (map<string, Schema *>::const_iterator i = collconfig.schema_begin();
	 i != collconfig.schema_end(); ++i)
    {
	const FieldConfig * config = i->second->get(fieldname);
	if (config != NULL) {
	    ValueEncoding field_encoding;
	    Xapian::valueno field_slot(config->get_slot(field_encoding));
	    if (field_slot == Xapian::BAD_VALUENO) {
		continue;
	    }
	    if (slot == Xapian::BAD_VALUENO) {
		encoding = field_encoding;
		slot = field_slot;
	    } else {
		if (slot != field_slot || encoding != field_encoding) {
		    throw InvalidValueError("Field '" + fieldname +
			"' has inconsistent configuration for its slot in "
			"the types being searched - support for handling "
			"this is not yet implemented");
		    // Note; support could be added for this - we'd either need
		    // to split the search into multiple sub-searches though,
		    // and then merge the results, or use complicated
		    // PostingSources, KeyMakers, or similar which check what
		    // type a document is before getting and decoding entries
		    // from slots.
		}
	    }
	}
    }

    return SlotDecoder::create(slot, encoding);
}


DocumentTypeQueryBuilder::DocumentTypeQueryBuilder(
    const CollectionConfig & collconfig_,
    const std::string & doc_type)
	: QueryBuilder(collconfig_),
	  schema(collconfig_.get_schema(doc_type))
{
}

Xapian::Query
DocumentTypeQueryBuilder::field_query(const std::string & fieldname,
				      const std::string & querytype,
				      const Json::Value & queryparams) const
{
    // Schema cannot be NULL, because this is checked in build().
    const FieldConfig * config = schema->get(fieldname);
    if (config == NULL) {
	return Xapian::Query::MatchNothing;
    }
    return config->query(querytype, queryparams);
}

Xapian::Query
DocumentTypeQueryBuilder::build(const Json::Value & jsonquery) const
{
    if (schema == NULL) {
	// Will happen if no documents have been added yet with this type, so
	// this isn't an error.
	return Xapian::Query::MatchNothing;
    }

    // Filter to return only documents of this type.
    const FieldConfig * typeconfig = schema->get(collconfig.get_type_field());
    if (typeconfig == NULL) {
	// Should only happen if there isn't a type field, so a type-specific
	// search should return nothing.
	return Xapian::Query::MatchNothing;
    }
    Xapian::Query type_query(typeconfig->query("is", schema->get_doctype()));

    return Xapian::Query(Xapian::Query::OP_FILTER,
			 build_query(jsonquery), type_query);
}

Xapian::doccount
DocumentTypeQueryBuilder::total_docs(const Xapian::Database & db) const
{
    if (schema == NULL) {
	return 0u;
    }

    const FieldConfig * typeconfig = schema->get(collconfig.get_type_field());
    if (typeconfig == NULL) {
	// Should only happen if there isn't a type field, so a type-specific
	// search should return nothing.
	return 0u;
    }

    // FIXME - this could be done a bit more efficiently if we could get the
    // term directly, since we just want to check its frequency.
    Xapian::Query type_query(typeconfig->query("is", schema->get_doctype()));
    Xapian::Enquire enq(db);
    enq.set_query(type_query);
    Xapian::MSet mset(enq.get_mset(0, 0));
    return mset.get_matches_upper_bound();
}

const FieldConfig *
DocumentTypeQueryBuilder::get_field_config(const std::string & fieldname) const
{
    if (schema != NULL) {
	return schema->get(fieldname);
    } else {
	return NULL;
    }
}

SlotDecoder *
DocumentTypeQueryBuilder::get_slot_decoder(const std::string & fieldname) const
{
    if (schema == NULL) {
	// Will happen if no documents have been added yet with this type, so
	// this isn't an error.
	return NULL;
    }

    const FieldConfig * fieldconfig = schema->get(fieldname);
    if (fieldconfig == NULL) {
	// No field configuration - so no slot.
	return NULL;
    }

    ValueEncoding encoding;
    Xapian::valueno slot = fieldconfig->get_slot(encoding);
    return SlotDecoder::create(slot, encoding);
}
