/** @file schema.cc
 * @brief Search schema implementation.
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

#include <config.h>

#include "schema.h"

#include <cjk-tokenizer.h> // FIXME - this should be moved to a separate file
#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"
#include <memory>
#include <string>
#include <xapian.h>

#include "docdata.h"
#include "indexing.h"
#include "infohandlers.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/utils.h"

using namespace RestPose;
using namespace std;

FieldConfig *
FieldConfig::from_json(const Json::Value & value)
{
    json_check_object(value, "field configuration");
    string type = json_get_string_member(value, "type", string());
    if (type.size() > 0) {
	switch(type[0]) {
	    case 'd':
		if (type == "date") return new DateFieldConfig(value);
		break;
	    case 'e':
		if (type == "exact") return new ExactFieldConfig(value);
		break;
	    case 'i':
		if (type == "id") return new IDFieldConfig(value);
		if (type == "ignore") return new IgnoredFieldConfig();
		break;
	    case 's':
		if (type == "stored") return new StoredFieldConfig(value);
		break;
	    case 't':
		if (type == "text") return new TextFieldConfig(value);
		break;
	}
    }
    throw InvalidValueError("Field configuration type \"" + type + "\" is not supported");
}

FieldConfig::~FieldConfig()
{}

MaxLenFieldConfig::MaxLenFieldConfig(const Json::Value & value)
{
    json_check_object(value, "field configuration");
    max_length = json_get_uint64_member(value, "max_length", 240, 64);
    string action_str = json_get_string_member(value, "too_long_action",
						    "error");
    if (action_str == "error") {
	too_long_action = TOOLONG_ERROR;
    } else if (action_str == "hash") {
	too_long_action = TOOLONG_HASH;
    } else if (action_str == "truncate") {
	too_long_action = TOOLONG_TRUNCATE;
    } else {
	throw InvalidValueError("Field configuration too_long_action \"" +
				action_str + "\" is not supported");
    }
}

MaxLenFieldConfig::~MaxLenFieldConfig()
{}

void
MaxLenFieldConfig::to_json(Json::Value & value) const
{
    json_check_object(value, "field configuration");
    value["max_length"] = max_length;
    switch (too_long_action) {
	case TOOLONG_ERROR:
	    value["too_long_action"] = "error";
	    break;
	case TOOLONG_HASH:
	    value["too_long_action"] = "hash";
	    break;
	case TOOLONG_TRUNCATE:
	    value["too_long_action"] = "truncate";
	    break;
    }
}

IDFieldConfig::IDFieldConfig(const Json::Value & value)
	: MaxLenFieldConfig(value)
{
    store_field = json_get_string_member(value, "store_field", string());
}

FieldIndexer *
IDFieldConfig::indexer() const
{
    return new ExactStringIndexer("\t", store_field, 0,
				  max_length, too_long_action, true);
}

Xapian::Query
IDFieldConfig::query(const string & qtype,
		     const Json::Value & value) const
{
    json_check_array(value, "field query value");
    if (qtype != "is") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for id field");
    }
    vector<string> terms;
    for (Json::Value::const_iterator iter = value.begin();
	 iter != value.end(); ++iter) {
	if ((*iter).isString()) {
	    terms.push_back("\t" + (*iter).asString());
	} else {
	    if (!(*iter).isConvertibleTo(Json::uintValue)) {
		throw InvalidValueError("ID value must be an integer or a string");
	    }
	    if ((*iter) < Json::Value::Int(0)) {
		throw InvalidValueError("JSON value for field was negative - wanted unsigned int");
	    }
	    if ((*iter) > Json::Value::maxUInt64) {
		throw InvalidValueError("JSON value " + (*iter).toStyledString() +
					" was larger than maximum allowed (" +
					Json::valueToString(Json::Value::maxUInt64) +
					")");
	    }
	    terms.push_back("\t" + Json::valueToString((*iter).asUInt64()));
	}
    }
    return Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
}

void
IDFieldConfig::to_json(Json::Value & value) const
{
    MaxLenFieldConfig::to_json(value);
    value["type"] = "id";
    value["store_field"] = store_field;
}

IDFieldConfig::~IDFieldConfig()
{}

ExactFieldConfig::ExactFieldConfig(const Json::Value & value)
	: MaxLenFieldConfig(value)
{
    // MaxLenFieldConfig constructor has already checked that value is an
    // object.
    prefix = json_get_string_member(value, "prefix", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"prefix\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"prefix\""
				" contains invalid character \\t");
    }
    prefix.append("\t");

    store_field = json_get_string_member(value, "store_field", string());
    wdfinc = json_get_uint64_member(value, "wdfinc", Json::Value::maxUInt64, 0);
}

FieldIndexer *
ExactFieldConfig::indexer() const
{
    return new ExactStringIndexer(prefix, store_field, wdfinc,
				  max_length, too_long_action, false);
}

Xapian::Query
ExactFieldConfig::query(const string & qtype,
			const Json::Value & value) const
{
    json_check_array(value, "field query value");
    if (qtype != "is") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for exact field");
    }
    vector<string> terms;
    for (Json::Value::const_iterator iter = value.begin();
	 iter != value.end(); ++iter) {
	if ((*iter).isString()) {
	    terms.push_back(prefix + (*iter).asString());
	} else {
	    if (!(*iter).isConvertibleTo(Json::uintValue)) {
		throw InvalidValueError("Filter value must be an integer or a string");
	    }
	    if ((*iter) < Json::Value::Int(0)) {
		throw InvalidValueError("JSON value for field was negative - wanted unsigned int");
	    }
	    if ((*iter) > Json::Value::maxUInt64) {
		throw InvalidValueError("JSON value " + (*iter).toStyledString() +
					" was larger than maximum allowed (" +
					Json::valueToString(Json::Value::maxUInt64) +
					")");
	    }
	    terms.push_back(prefix + Json::valueToString((*iter).asUInt64()));
	}
    }
    return Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
}

void
ExactFieldConfig::to_json(Json::Value & value) const
{
    MaxLenFieldConfig::to_json(value);
    value["type"] = "exact";
    value["prefix"] = prefix.substr(0, prefix.size() - 1);
    value["store_field"] = store_field;
    value["wdfinc"] = wdfinc;
}

ExactFieldConfig::~ExactFieldConfig()
{}

TextFieldConfig::TextFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    prefix = json_get_string_member(value, "prefix", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"prefix\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"prefix\""
				" contains invalid character \\t");
    }
    prefix.append("\t");
    store_field = json_get_string_member(value, "store_field", string());
    processor = json_get_string_member(value, "processor", string());
}

TextFieldConfig::~TextFieldConfig()
{}

FieldIndexer *
TextFieldConfig::indexer() const
{
    if (processor == "cjk") {
	return new CJKIndexer(prefix, store_field);
    } else if (startswith(processor, "stem_")) {
	return new TermGeneratorIndexer(prefix, store_field, processor.substr(5));
    } else {
	return new TermGeneratorIndexer(prefix, store_field, string());
    }
}

/** Build a CJK query.
 */
static Xapian::Query
build_CJK_query(string prefix, string text, string op, unsigned window)
{
    cjk::tokenizer tknzr;
    vector<pair<string, unsigned> > token_list;
    tknzr.tokenize(text, token_list);
    if (token_list.empty()) {
	return Xapian::Query::MatchNothing;
    }

    vector<string> phraseterms;
    unsigned lastpos = 0;
    for (vector<pair<string, unsigned> >::const_iterator
	 token_iter = token_list.begin();
	 token_iter != token_list.end();
	 ++token_iter) {
	string term(Xapian::Unicode::tolower(token_iter->first));
	if (!phraseterms.empty() && lastpos == token_iter->second) {
	    phraseterms.pop_back();
	}
	phraseterms.push_back(prefix + term);
	lastpos = token_iter->second;
    }

    if (op == "phrase") {
	return Xapian::Query(Xapian::Query::OP_PHRASE, phraseterms.begin(),
			     phraseterms.end(), window);
    } else if (op == "near") {
	return Xapian::Query(Xapian::Query::OP_NEAR, phraseterms.begin(),
			     phraseterms.end(), window);
    } else if (op == "and") {
	return Xapian::Query(Xapian::Query::OP_AND, phraseterms.begin(),
			     phraseterms.end());
    } else if (op == "or") {
	return Xapian::Query(Xapian::Query::OP_OR, phraseterms.begin(),
			     phraseterms.end());
    }
    // Shouldn't get here.
    return Xapian::Query::MatchNothing;
}

/** Build a stemmed phrase query.
 */
static Xapian::Query
build_stem_query(string prefix, string text, string op, unsigned window,
		 string stemmer)
{
    Xapian::QueryParser qp;
    if (!stemmer.empty()) {
	qp.set_stemmer(Xapian::Stem(stemmer));
	qp.set_stemming_strategy(qp.STEM_SOME);
    }

    if (op == "phrase") {
	qp.set_default_op(Xapian::Query::OP_PHRASE);
	// Stemmed words aren't indexed with positions, so need to disable stemming here.
	qp.set_stemming_strategy(qp.STEM_NONE);
    } else if (op == "near") {
	qp.set_default_op(Xapian::Query::OP_NEAR);
	// Stemmed words aren't indexed with positions, so need to disable stemming here.
	qp.set_stemming_strategy(qp.STEM_NONE);
    } else if (op == "and") {
	qp.set_default_op(Xapian::Query::OP_AND);
    } else if (op == "or") {
	qp.set_default_op(Xapian::Query::OP_OR);
    }

    (void) window;  // Can't currently pass a window to queryparser.
    return qp.parse_query(text, 0, prefix);
}

/** Build a query using the query parser.
 */
static Xapian::Query
build_parsed_query(string prefix, string text, string op, string stemmer)
{
    Xapian::QueryParser qp;
    if (!stemmer.empty()) {
	qp.set_stemmer(Xapian::Stem(stemmer));
	qp.set_stemming_strategy(qp.STEM_SOME);
    }

    if (op == "and") {
	qp.set_default_op(Xapian::Query::OP_AND);
    } else if (op == "or") {
	qp.set_default_op(Xapian::Query::OP_OR);
    }

    return qp.parse_query(text,
			  Xapian::QueryParser::FLAG_DEFAULT ||
			  Xapian::QueryParser::FLAG_WILDCARD ||
			  Xapian::QueryParser::FLAG_PURE_NOT,
			  prefix);
}


Xapian::Query
TextFieldConfig::query(const string & qtype,
		       const Json::Value & qparams) const
{
    if (qtype == "text") {
	return query_phrase(qparams);
    } else if (qtype == "parse") {
	return query_parse(qparams);
    }
    throw InvalidValueError("Invalid query type \"" + qtype +
			    "\" for text field");
}

Xapian::Query
TextFieldConfig::query_phrase(const Json::Value & qparams) const
{
    string text;
    string op("phrase");
    unsigned window = 0;
    if (qparams.isString()) {
	text = qparams.asString();
    } else {
	if (!qparams.isObject()) {
	    throw InvalidValueError("Invalid value for text field query - "
				    "must be string or object");
	}
	text = json_get_string_member(qparams, "text", string());
	op = json_get_string_member(qparams, "op", op);
	if (op != "and" && op != "or" && op != "phrase" && op != "near") {
	    throw InvalidValueError("Invalid operator \"" + op +
				    "\" for text query on text field");
	}
	const Json::Value & window_json(qparams["window"]);
	if (!window_json.isNull()) {
	    window = json_get_uint64_member(qparams, "window", Json::Value::maxInt);
	}
    }

    if (processor == "cjk") {
	return build_CJK_query(prefix, text, op, window);
    } else if (startswith(processor, "stem_")) {
	return build_stem_query(prefix, text, op, window, processor.substr(5));
    } else {
	return build_stem_query(prefix, text, op, window, string());
    }
}

Xapian::Query
TextFieldConfig::query_parse(const Json::Value & qparams) const
{
    string text;
    string op;
    if (qparams.isString()) {
	text = qparams.asString();
	op = "and";
    } else {
	if (!qparams.isObject()) {
	    throw InvalidValueError("Invalid value for text field query - "
				    "must be string or object");
	}
	text = json_get_string_member(qparams, "text", string());
	op = json_get_string_member(qparams, "op", op);
	if (op != "and" && op != "or") {
	    throw InvalidValueError("Invalid operator \"" + op +
				    "\" for parse query on text field");
	}
    }

    if (processor == "cjk") {
	throw InvalidValueError("Query parser does not support CJK fields");
    } else if (startswith(processor, "stem_")) {
	return build_parsed_query(prefix, text, op, processor.substr(5));
    } else {
	return build_parsed_query(prefix, text, op, string());
    }
}


void
TextFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "text";
    value["prefix"] = prefix.substr(0, prefix.size() - 1);
    value["store_field"] = store_field;
    value["processor"] = processor;
}

DateFieldConfig::DateFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    slot = json_get_uint64_member(value, "slot", Xapian::BAD_VALUENO);
    store_field = json_get_string_member(value, "store_field", string());
}

DateFieldConfig::~DateFieldConfig()
{}

FieldIndexer *
DateFieldConfig::indexer() const
{
    return new DateIndexer(slot, store_field);
}

Xapian::Query
DateFieldConfig::query(const string & qtype,
		       const Json::Value & value) const
{
    if (qtype != "range") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for date field");
    }
    json_check_array(value, "filter value");
    if (value.size() != 2) {
	throw InvalidValueError("Date field range must have exactly two points");
    }
    string start = Xapian::sortable_serialise(json_get_uint64(value[Json::UInt(0u)]));
    string end = Xapian::sortable_serialise(json_get_uint64(value[1u]));
    return Xapian::Query(Xapian::Query::OP_VALUE_RANGE, slot, start, end);
}

void
DateFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "date";
    value["slot"] = slot;
    value["store_field"] = store_field;
}

StoredFieldConfig::StoredFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    store_field = json_get_string_member(value, "store_field", string());
    if (store_field.empty()) {
	throw InvalidValueError("Field configuration argument \"store_field\""
				" may not be empty");
    }
}

StoredFieldConfig::~StoredFieldConfig()
{}

FieldIndexer *
StoredFieldConfig::indexer() const
{
    return new StoredIndexer(store_field);
}

Xapian::Query
StoredFieldConfig::query(const string &,
			 const Json::Value &) const
{
    throw InvalidValueError("Cannot filter on stored-only field");
}

void
StoredFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "stored";
    value["store_field"] = store_field;
}

IgnoredFieldConfig::~IgnoredFieldConfig()
{}

FieldIndexer *
IgnoredFieldConfig::indexer() const
{
    return NULL;
}

Xapian::Query
IgnoredFieldConfig::query(const string &,
			 const Json::Value &) const
{
    throw InvalidValueError("Cannot search on ignored field");
}

void
IgnoredFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "ignore";
}

Schema::~Schema()
{
    clear();
}

void
Schema::clear()
{
    {
	map<string, FieldConfig *>::iterator i;
	for (i = fields.begin(); i != fields.end(); ++i) {
	    delete i->second;
	}
	fields.clear();
    }

    {
	map<string, FieldIndexer *>::iterator i;
	for (i = indexers.begin(); i != indexers.end(); ++i) {
	    delete i->second;
	}
	indexers.clear();
    }
}

Json::Value &
Schema::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    if (!fields.empty()) {
	static const Json::StaticString fields_key("fields");
	Json::Value & subval(value[fields_key] = Json::objectValue);
	map<string, FieldConfig *>::const_iterator i;
	for (i = fields.begin(); i != fields.end(); ++i) {
	    i->second->to_json(subval[i->first] = Json::objectValue);
	}
    }
    return value;
}

string
Schema::to_json_string() const
{
    Json::Value value(Json::objectValue);
    to_json(value);
    Json::FastWriter writer;
    return writer.write(value);
}

void
Schema::from_json(const Json::Value & value)
{
    clear();
    json_check_object(value, "schema");

    // If needed, do custom processing to handle old schema formats here.
    Json::Value fields_value = value.get("fields", Json::Value::null);
    if (!fields_value.isNull()) {
	json_check_object(fields_value, "fields in schema");
	for (Json::Value::iterator i = fields_value.begin();
	     i != fields_value.end();
	     ++i) {
	    set(i.memberName(), FieldConfig::from_json(*i));
	}
    }
}

void
Schema::merge_from(const Schema & other)
{
    for (map<string, FieldConfig *>::const_iterator
	 i = other.fields.begin(); i != other.fields.end(); ++i) {
	map<string, FieldConfig *>::iterator j
		= fields.find(i->first);
	Json::Value tmp;
	i->second->to_json(tmp);
	if (j == fields.end()) {
	    // FIXME - copy directly, rather than going through json.
	    set(i->first, FieldConfig::from_json(tmp));
	} else {
	    // Complain if configuration is not identical.
	    //
	    // Note, if this is changed in future to allow field
	    // configuratino to be changed, don't forget to clear the cache
	    // (in indexers).

	    Json::Value tmp2;
	    j->second->to_json(tmp2);
	    if (json_serialise(tmp) != json_serialise(tmp2)) {
		throw InvalidValueError("Cannot change configuration in schema for field \"" + i->first + "\".");
	    }
	}
    }
}

const FieldConfig *
Schema::get(const string & fieldname) const
{
    map<string, FieldConfig *>::const_iterator i;
    i = fields.find(fieldname);
    if (i == fields.end())
	return NULL;
    return i->second;
}

const FieldIndexer *
Schema::get_indexer(const string & fieldname) const
{
    map<string, FieldIndexer *>::const_iterator i;
    i = indexers.find(fieldname);
    if (i != indexers.end()) {
	return i->second;
    }

    map<string, FieldConfig *>::const_iterator j;
    j = fields.find(fieldname);
    if (j == fields.end()) {
	return NULL;
    }

    // Make a new FieldIndexer, store it, and return it.
    auto_ptr<FieldIndexer> newptr(j->second->indexer());
    const FieldIndexer * result = newptr.get();
    pair<map<string, FieldIndexer *>::iterator, bool> ret;
    pair<string, FieldIndexer *> item(fieldname, NULL);
    ret = indexers.insert(item);
    ret.first->second = newptr.release();
    return result;
}

void
Schema::set(const string & fieldname, FieldConfig * config)
{
    if (config == NULL) {
	map<string, FieldConfig *>::iterator i;
	i = fields.find(fieldname);
	if (i != fields.end()) {
	    delete i->second;
	    i->second = NULL;
	    fields.erase(i);
	}
	return;
    }

    auto_ptr<FieldConfig> configptr(config);
    pair<string, FieldConfig*> item(fieldname, NULL);
    pair<map<string, FieldConfig *>::iterator, bool> ret;
    ret = fields.insert(item);
    delete(ret.first->second);
    ret.first->second = configptr.release();
}

Xapian::Document
Schema::process(const Json::Value & value,
		string & idterm) const
{
    json_check_object(value, "input document");

    Xapian::Document result;
    DocumentData docdata;

    for (Json::Value::const_iterator viter = value.begin();
	 viter != value.end();
	 ++viter) {
	const FieldIndexer * indexer = get_indexer(viter.memberName());
	if (indexer) {
	    //fprintf(stderr, "field '%s'\n", viter.memberName());
	    if ((*viter).isNull()) {
		//fprintf(stderr, "null value ignored\n");
		continue;
	    }
	    if ((*viter).isArray()) {
		if ((*viter).size() > 0) {
		    indexer->index(result, docdata, *viter, idterm);
		} else {
		    //fprintf(stderr, "empty array value ignored\n");
		}
	    } else {
		Json::Value arrayval(Json::arrayValue);
		arrayval.append(*viter);
		indexer->index(result, docdata, arrayval, idterm);
	    }
	} else {
	    //fprintf(stderr, "Ignoring unknown field '%s'\n", viter.memberName());
	}
    }

    result.set_data(docdata.serialise());
    return result;
}

Xapian::Query
Schema::build_query(const Xapian::Database & db,
		    const Json::Value & jsonquery) const
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
	string ftype = queryparams[Json::UInt(1u)].asString();
	map<string, FieldConfig *>::const_iterator i;
	i = fields.find(fieldname);
	if (i == fields.end()) {
	    return Xapian::Query::MatchNothing;
	}
	return i->second->query(ftype, queryparams[2]);
    }

    if (jsonquery.isMember("filter")) {
	if ((jsonquery.isMember("query") && jsonquery.size() != 2) ||
	    (!jsonquery.isMember("query") && jsonquery.size() != 1)) {
	    throw InvalidValueError("Filter query must contain only filter and query members");
	}

	Xapian::Query xprimary;
	const Json::Value & primary = jsonquery["query"];
	if (primary.isNull()) {
	    xprimary = Xapian::Query::MatchAll;
	} else {
	    xprimary = build_query(db, primary);
	}

	const Json::Value & filter = jsonquery["filter"];
	Xapian::Query xfilter(build_query(db, filter));

	return Xapian::Query(Xapian::Query::OP_FILTER, xprimary, xfilter);
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
	    queries.push_back(build_query(db, *i));
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
	    queries.push_back(build_query(db, *i));
	}
	return Xapian::Query(Xapian::Query::OP_OR,
			     queries.begin(), queries.end());
    }

    if (jsonquery.isMember("not")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("NOT query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["not"];
	json_check_array(queryparams, "NOT search parameters");
	if (queryparams.size() < 2) {
	    throw InvalidValueError("Not query must contain at least two subqueries");
	}

	Json::Value::const_iterator i = queryparams.begin();
	Xapian::Query posquery(build_query(db, *i));
	++i;

	vector<Xapian::Query> negqueries;
	negqueries.reserve(queryparams.size() - 1);

	for (; i != queryparams.end(); ++i) {
	    negqueries.push_back(build_query(db, *i));
	}
	return Xapian::Query(Xapian::Query::OP_AND_NOT,
			     posquery,
			     Xapian::Query(Xapian::Query::OP_OR,
					   negqueries.begin(), negqueries.end()));
    }

    if (jsonquery.isMember("scale")) {
	if (jsonquery.size() != 1) {
	    throw InvalidValueError("Scale query must contain exactly one member");
	}
	const Json::Value & queryparams = jsonquery["scale"];

	if (!queryparams.isMember("query")) {
	    throw InvalidValueError("Scale query must contain a query member");
	}
	Xapian::Query subquery = build_query(db, queryparams["query"]);
	return Xapian::Query(Xapian::Query::OP_SCALE_WEIGHT, subquery,
	    json_get_double_member(queryparams, "factor", 0.0));
    }

    throw InvalidValueError("Invalid query specification - no known members in query object");
}

void
Schema::get_fieldlist(Json::Value & result, const Json::Value & search) const
{
    result = search["display"];
    if (result.isNull()) {
	result = Json::Value(Json::arrayValue);
	map<string, FieldConfig *>::const_iterator fiter;
	for (fiter = fields.begin(); fiter != fields.end(); ++fiter) {
	    result.append(fiter->first);
	}
    } else {
	json_check_array(result, "list of fields to display");
    }
}

void
Schema::perform_search(const Xapian::Database & db,
		       const Json::Value & search,
		       Json::Value & results) const
{
    results = Json::objectValue;
    json_check_object(search, "search");
    bool verbose = json_get_bool(search, "verbose", false);

    Xapian::Query query(build_query(db, search["query"]));

    Xapian::doccount from, size, checkatleast;
    from = json_get_uint64_member(search, "from", Json::Value::maxUInt, 0);

    if (search["size"] == -1) {
	size = db.get_doccount();
    } else {
	size = json_get_uint64_member(search, "size", Json::Value::maxUInt, 10);
    }

    if (search["checkatleast"] == -1) {
	checkatleast = db.get_doccount();
    } else {
	checkatleast = json_get_uint64_member(search, "checkatleast", Json::Value::maxUInt, 0);
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
	    info_handlers.add_handler(*i, enq, &db, this, checkatleast);
	}
    }

    Xapian::MSet mset(enq.get_mset(from, size, checkatleast));

    // Get the list of fields to return.
    Json::Value fieldlistval;
    get_fieldlist(fieldlistval, search);

    // Write the results
    info_handlers.write_results(results, mset);
    results["from"] = from;
    results["size"] = size;
    results["checkatleast"] = checkatleast;
    results["matches_lower_bound"] = mset.get_matches_lower_bound();
    results["matches_estimated"] = mset.get_matches_estimated();
    results["matches_upper_bound"] = mset.get_matches_upper_bound();
    Json::Value & items = results["items"] = Json::arrayValue;
    for (Xapian::MSetIterator i = mset.begin(); i != mset.end(); ++i) {
	Xapian::Document doc(i.get_document());
	DocumentData docdata;
	docdata.unserialise(doc.get_data());
	Json::Value item(Json::objectValue);
	display_doc(doc, fieldlistval, item);
	items.append(item);
    }
    if (verbose) {
	// FIXME - give debugging details about the search executed.
	// Note - we can't just include query.get_description() in the output,
	// because this isn't always a valid unicode string.
    }
}

void
Schema::perform_search(const Xapian::Database & db,
		       const string & search_str,
		       Json::Value & results) const
{
    Json::Reader reader;
    Json::Value search;
    bool ok = reader.parse(search_str, search, false);
    if (!ok) {
	throw InvalidValueError("Invalid JSON supplied for search: " +
				reader.getFormatedErrorMessages());
    }
    perform_search(db, search, results);
}

void
Schema::display_doc(const Xapian::Document & doc,
		    const Json::Value & fieldlist,
		    Json::Value & result) const
{
    json_check_array(fieldlist, "display field list");
    json_check_object(result, "display result");
    DocumentData docdata;
    docdata.unserialise(doc.get_data());
    Json::Reader reader;
    for (Json::Value::const_iterator fiter = fieldlist.begin();
	 fiter != fieldlist.end();
	 ++fiter) {
	if (!(*fiter).isString()) {
	    throw InvalidValueError("Item in display field list was not a string");
	}
	string fieldname((*fiter).asString());
	string val(docdata.get(fieldname));
	if (!val.empty()) {
	    Json::Value fieldval;
	    bool ok = reader.parse(val, fieldval, false);
	    if (!ok) {
		throw InvalidValueError("Invalid JSON found in document: " +
					reader.getFormatedErrorMessages());
	    }
	    result[fieldname] = fieldval;
	}
    }
}

void
Schema::display_doc(const Xapian::Document & doc,
		    Json::Value & result) const
{
    Json::Value fieldlist(Json::arrayValue);
    for (map<string, FieldConfig *>::const_iterator
	 i = fields.begin(); i != fields.end(); ++i) {
	fieldlist.append(i->first);
    }
    display_doc(doc, fieldlist, result);
}

string
Schema::display_doc_as_string(const Xapian::Document & doc,
			      const Json::Value & fieldlist) const
{
    Json::Value result(Json::objectValue);
    display_doc(doc, fieldlist, result);
    Json::FastWriter writer;
    return writer.write(result);
}

string
Schema::display_doc_as_string(const Xapian::Document & doc) const
{
    Json::Value result(Json::objectValue);
    display_doc(doc, result);
    Json::FastWriter writer;
    return writer.write(result);
}