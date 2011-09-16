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
#include "docdata.h"
#include "hashterm.h"
#include "indexing.h"
#include "infohandlers.h"
#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"
#include "jsonxapian/collconfig.h"
#include "jsonxapian/indexing.h"
#include "logger/logger.h"
#include <memory>
#include "postingsources/multivaluerange_source.h"
#include <set>
#include "slotname.h"
#include "str.h"
#include <string>
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/stringutils.h"
#include "utils/utils.h"
#include <xapian.h>

using namespace RestPose;
using namespace std;

void
FieldConfig::add_group_if_taxonomy(const std::string &,
				   std::set<std::string> &) const
{
}

FieldConfig *
FieldConfig::from_json(const Json::Value & value,
		       const string & doc_type)
{
    json_check_object(value, "field configuration");
    string type = json_get_string_member(value, "type", string());
    if (type.size() > 0) {
	switch(type[0]) {
	    case 'c':
		if (type == "cat") return new CategoryFieldConfig(value);
		break;
	    case 'd':
		if (type == "date") return new DateFieldConfig(value);
		if (type == "double") return new DoubleFieldConfig(value);
		break;
	    case 'e':
		if (type == "exact") return new ExactFieldConfig(value);
		break;
	    case 'i':
		if (type == "id") return new IDFieldConfig(value, doc_type);
		if (type == "ignore") return new IgnoredFieldConfig();
		break;
	    case 'm':
		if (type == "meta") return new MetaFieldConfig(value);
		break;
	    case 's':
		if (type == "stored") return new StoredFieldConfig(value);
		break;
	    case 't':
		if (type == "text") return new TextFieldConfig(value);
		if (type == "timestamp") return new TimestampFieldConfig(value);
		break;
	}
    }
    throw InvalidValueError("Field configuration type \"" + type + "\" is not supported");
}

FieldConfig::~FieldConfig()
{}


MetaFieldConfig::MetaFieldConfig(const Json::Value & value)
{
    json_check_object(value, "field configuration");
    prefix = json_get_string_member(value, "group", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"group\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"group\""
				" contains invalid character \\t");
    }
    prefix.append("\t");

    slot = value["slot"];
}

MetaFieldConfig::~MetaFieldConfig()
{}

FieldIndexer *
MetaFieldConfig::indexer() const
{
    return new MetaIndexer(prefix, slot.get());
}

Xapian::Query
MetaFieldConfig::query(const string & qtype,
		       const Json::Value & value) const
{
    // Ensure we have a JSON array.
    const Json::Value * value_ptr;
    Json::Value tmp(Json::arrayValue);
    if (!value.isArray()) {
	tmp.append(value);
	value_ptr = &tmp;
    } else {
	value_ptr = &value;
    }

    char code = '\0';
    if (qtype.size() > 1) {
	switch (qtype[1]) {
	    case 'x':
		if (qtype == "exists") {
		    code = 'F';
		}
		break;
	    case 'o':
		if (qtype == "nonempty") {
		    code = 'N';
		}
		break;
	    case 'm':
		if (qtype == "empty") {
		    code = 'M';
		}
		break;
	    case 'r':
		if (qtype == "error") {
		    code = 'E';
		}
		break;
	}
    }
    if (code == '\0') {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for meta field");
    }

    vector<string> terms;
    for (Json::Value::const_iterator iter = value_ptr->begin();
	 iter != value_ptr->end(); ++iter) {
	if ((*iter).isNull()) {
	    if (code == 'F') {
		// We don't store a single term for "any field exists",
		// because it would match almost every document, and rarely
		// be useful.  Instead, search for ("a field exists and is
		// non-empty" OR "a field exists and is empty").
		terms.push_back(prefix + 'N');
		terms.push_back(prefix + 'M');
	    } else {
		terms.push_back(prefix + code);
	    }
	} else if ((*iter).isString()) {
	    terms.push_back(prefix + code + (*iter).asString());
	} else {
	    throw InvalidValueError("Invalid query value (" +
				    json_serialise(*iter) +
				    ") for meta field - must be string or null");
	}
    }
    return Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
}

void
MetaFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "meta";
    value["group"] = prefix.substr(0, prefix.size() - 1);
    slot.to_json(value["slot"]);
}


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

IDFieldConfig::IDFieldConfig(const Json::Value & value,
			     const string & doc_type)
	: MaxLenFieldConfig(value),
	  prefix("\t" + doc_type + "\t")
{
    store_field = json_get_string_member(value, "store_field", string());
}

IDFieldConfig::IDFieldConfig(const string & doc_type,
			     unsigned int max_length_,
			     MaxLenFieldConfig::TooLongAction too_long_action_,
			     const string & store_field_)
	: MaxLenFieldConfig(max_length_, too_long_action_),
	  store_field(store_field_),
	  prefix("\t" + doc_type + "\t")
{}

FieldIndexer *
IDFieldConfig::indexer() const
{
    return new ExactStringIndexer(prefix, store_field, 0,
				  max_length, too_long_action, true);
}

Xapian::Query
IDFieldConfig::query(const string & qtype,
		     const Json::Value & value) const
{
    // Ensure we have a JSON array.
    const Json::Value * value_ptr;
    Json::Value tmp(Json::arrayValue);
    if (!value.isArray()) {
	tmp.append(value);
	value_ptr = &tmp;
    } else {
	value_ptr = &value;
    }

    if (qtype != "is") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for id field");
    }
    vector<string> terms;
    for (Json::Value::const_iterator iter = value_ptr->begin();
	 iter != value_ptr->end(); ++iter) {
	string termtext;
	if ((*iter).isString()) {
	    termtext = (*iter).asString();
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
	    termtext = Json::valueToString((*iter).asUInt64());
	}
	if (termtext.size() > max_length) {
	    switch (too_long_action) {
		case MaxLenFieldConfig::TOOLONG_ERROR:
		    // A matching field would have produced an error at
		    // indexing time, but that means it's not in the
		    // database, so we just let it pass; no results will
		    // come from this term (unless the config has changed
		    // since indexing or the user has been indexing to the
		    // database in another way).
		    break;
		case MaxLenFieldConfig::TOOLONG_HASH:
		    // Note - this isn't UTF-8 aware.
		    termtext = hash_long_term(termtext, max_length);
		    break;
		case MaxLenFieldConfig::TOOLONG_TRUNCATE:
		    // Note - this isn't UTF-8 aware.
		    termtext.erase(max_length);
		    break;
	    }
	}
	terms.push_back(prefix + termtext);
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
    prefix = json_get_string_member(value, "group", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"group\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"group\""
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
    // Ensure we have a JSON array.
    const Json::Value * value_ptr;
    Json::Value tmp(Json::arrayValue);
    if (!value.isArray()) {
	tmp.append(value);
	value_ptr = &tmp;
    } else {
	value_ptr = &value;
    }

    if (qtype != "is") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for exact field");
    }
    vector<string> terms;
    for (Json::Value::const_iterator iter = value_ptr->begin();
	 iter != value_ptr->end(); ++iter) {
	string termtext;
	if ((*iter).isString()) {
	    termtext = (*iter).asString();
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
	    termtext = Json::valueToString((*iter).asUInt64());
	}
	if (termtext.size() > max_length) {
	    switch (too_long_action) {
		case MaxLenFieldConfig::TOOLONG_ERROR:
		    // A matching field would have produced an error at
		    // indexing time, but that means it's not in the
		    // database, so we just let it pass; no results will
		    // come from this term (unless the config has changed
		    // since indexing or the user has been indexing to the
		    // database in another way).
		    break;
		case MaxLenFieldConfig::TOOLONG_HASH:
		    // Note - this isn't UTF-8 aware.
		    termtext = hash_long_term(termtext, max_length);
		    break;
		case MaxLenFieldConfig::TOOLONG_TRUNCATE:
		    // Note - this isn't UTF-8 aware.
		    termtext.erase(max_length);
		    break;
	    }
	}
	terms.push_back(prefix + termtext);
    }
    return Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
}

void
ExactFieldConfig::to_json(Json::Value & value) const
{
    MaxLenFieldConfig::to_json(value);
    value["type"] = "exact";
    value["group"] = prefix.substr(0, prefix.size() - 1);
    value["store_field"] = store_field;
    value["wdfinc"] = wdfinc;
}

ExactFieldConfig::~ExactFieldConfig()
{}

TextFieldConfig::TextFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    prefix = json_get_string_member(value, "group", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"group\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"group\""
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
    string op("and");
    if (qparams.isString()) {
	text = qparams.asString();
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
    value["group"] = prefix.substr(0, prefix.size() - 1);
    value["store_field"] = store_field;
    value["processor"] = processor;
}


DoubleFieldConfig::DoubleFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    slot = value["slot"];
    store_field = json_get_string_member(value, "store_field", string());
}

DoubleFieldConfig::~DoubleFieldConfig()
{}

FieldIndexer *
DoubleFieldConfig::indexer() const
{
    return new DoubleIndexer(slot.get(), store_field);
}

Xapian::Query
DoubleFieldConfig::query(const string & qtype,
			 const Json::Value & value) const
{
    if (qtype != "range") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for date field");
    }
    json_check_array(value, "filter value");
    if (value.size() != 2) {
	throw InvalidValueError("Numeric field range must have exactly two points");
    }

    if (!value[0].isConvertibleTo(Json::realValue)) {
	throw InvalidValueError(string("JSON value for double field query (") +
				json_serialise(value[0]) +
				") was not convertible to a double");
    }

    if (!value[1].isConvertibleTo(Json::realValue)) {
	throw InvalidValueError(string("JSON value for double field query (") +
				json_serialise(value[1]) +
				") was not convertible to a double");
    }

    string start = Xapian::sortable_serialise(value[0u].asDouble());
    string end = Xapian::sortable_serialise(value[1u].asDouble());

    MultiValueRangeSource source(slot.get(), 1.0, start, end);
    return Xapian::Query(&source);
}

void
DoubleFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "double";
    slot.to_json(value["slot"]);
    value["store_field"] = store_field;
}


TimestampFieldConfig::TimestampFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    slot = value["slot"];
    store_field = json_get_string_member(value, "store_field", string());
}

TimestampFieldConfig::~TimestampFieldConfig()
{}

FieldIndexer *
TimestampFieldConfig::indexer() const
{
    return new TimeStampIndexer(slot.get(), store_field);
}

Xapian::Query
TimestampFieldConfig::query(const string & qtype,
			    const Json::Value & value) const
{
    if (qtype != "range") {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for timestamp field");
    }
    json_check_array(value, "filter value");
    if (value.size() != 2) {
	throw InvalidValueError("Timestamp field range must have exactly two points");
    }
    string start = Xapian::sortable_serialise(json_get_uint64(value[Json::UInt(0u)]));
    string end = Xapian::sortable_serialise(json_get_uint64(value[1u]));

    MultiValueRangeSource source(slot.get(), 1.0, start, end);
    return Xapian::Query(&source);
}

void
TimestampFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "timestamp";
    slot.to_json(value["slot"]);
    value["store_field"] = store_field;
}


DateFieldConfig::DateFieldConfig(const Json::Value & value)
{
    json_check_object(value, "schema object");
    slot = value["slot"];
    store_field = json_get_string_member(value, "store_field", string());
}

DateFieldConfig::~DateFieldConfig()
{}

FieldIndexer *
DateFieldConfig::indexer() const
{
    return new DateIndexer(slot.get(), store_field);
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
    string error;
    string start = DateIndexer::parse_date(value[Json::UInt(0u)], error);
    if (!error.empty()) {
	throw InvalidValueError(error);
    }
    string end = DateIndexer::parse_date(value[1u], error);
    if (!error.empty()) {
	throw InvalidValueError(error);
    }

    MultiValueRangeSource source(slot.get(), 1.0, start, end);
    return Xapian::Query(&source);
}

void
DateFieldConfig::to_json(Json::Value & value) const
{
    value["type"] = "date";
    slot.to_json(value["slot"]);
    value["store_field"] = store_field;
}


CategoryFieldConfig::CategoryFieldConfig(const Json::Value & value)
	: MaxLenFieldConfig(value)
{
    // MaxLenFieldConfig constructor has already checked that value is an
    // object.
    prefix = json_get_string_member(value, "group", string());
    if (prefix.empty()) {
	throw InvalidValueError("Field configuration argument \"group\""
				" may not be empty");
    }
    if (prefix.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"group\""
				" contains invalid character \\t");
    }
    prefix.append("\t");

    taxonomy_name = json_get_string_member(value, "taxonomy", string());
    if (taxonomy_name.empty()) {
	throw InvalidValueError("Field configuration argument \"taxonomy\""
				" may not be empty");
    }
    if (taxonomy_name.find('\t') != string::npos) {
	throw InvalidValueError("Field configuration argument \"taxonomy\""
				" contains invalid character \\t");
    }

    store_field = json_get_string_member(value, "store_field", string());
}

CategoryFieldConfig::~CategoryFieldConfig()
{}

FieldIndexer *
CategoryFieldConfig::indexer() const
{
    return new CategoryIndexer(prefix, taxonomy_name, store_field,
			       max_length, too_long_action);
}

Xapian::Query
CategoryFieldConfig::query(const string & qtype,
			   const Json::Value & value) const
{
    // Ensure we have a JSON array.
    const Json::Value * value_ptr;
    Json::Value tmp(Json::arrayValue);
    if (!value.isArray()) {
	tmp.append(value);
	value_ptr = &tmp;
    } else {
	value_ptr = &value;
    }

    vector<string> term_prefixes;
    if (qtype == "is") {
	// The categories associated with a document are stored with an
	// additional C prefix.
	term_prefixes.push_back(prefix + "C");
    } else if (qtype == "is_descendant") {
	// The ancestors of categories associated with a document are stored
	// with an additional A prefix (the category itself is not stored with
	// this prefix).
	term_prefixes.push_back(prefix + "A");
    } else if (qtype == "is_or_is_descendant") {
	// The ancestors of categories associated with a document are stored
	// with an additional A prefix (the category itself is not stored with
	// this prefix).
	term_prefixes.push_back(prefix + "C");
	term_prefixes.push_back(prefix + "A");
    } else {
	throw InvalidValueError("Invalid query type \"" + qtype +
				"\" for category field");
    }

    vector<string> terms;
    for (Json::Value::const_iterator iter = value_ptr->begin();
	 iter != value_ptr->end(); ++iter) {
	if ((*iter).isString()) {
	    for (vector<string>::const_iterator i = term_prefixes.begin();
		 i != term_prefixes.end(); ++i) {
		terms.push_back(*i + (*iter).asString());
	    }
	} else {
	    if (!(*iter).isConvertibleTo(Json::uintValue)) {
		throw InvalidValueError("Category value must be an integer or a string");
	    }
	    if ((*iter) < Json::Value::Int(0)) {
		throw InvalidValueError("JSON value for category was negative - wanted unsigned int");
	    }
	    if ((*iter) > Json::Value::maxUInt64) {
		throw InvalidValueError("JSON value " + (*iter).toStyledString() +
					" was larger than maximum allowed (" +
					Json::valueToString(Json::Value::maxUInt64) +
					")");
	    }
	    for (vector<string>::const_iterator i = term_prefixes.begin();
		 i != term_prefixes.end(); ++i) {
		terms.push_back(*i + Json::valueToString((*iter).asUInt64()));
	    }
	}
    }
    return Xapian::Query(Xapian::Query::OP_OR, terms.begin(), terms.end());
}

void
CategoryFieldConfig::add_group_if_taxonomy(const string & taxonomy_name_,
					   set<string> & result) const
{
    if (taxonomy_name == taxonomy_name_) {
	result.insert(prefix.substr(0, prefix.size() - 1));
    }
}

void
CategoryFieldConfig::to_json(Json::Value & value) const
{
    MaxLenFieldConfig::to_json(value);
    value["type"] = "cat";
    value["group"] = prefix.substr(0, prefix.size() - 1);
    value["taxonomy"] = taxonomy_name;
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

FieldConfigPattern::FieldConfigPattern()
	: leading_wildcard(false)
{}

void
FieldConfigPattern::from_json(const Json::Value & value)
{
    json_check_array(value, "schema pattern");
    if (value.size() != 2) {
	throw InvalidValueError("Schema patterns must be arrays of length 2");
    }
    const Json::Value & pattern_obj = value[0];
    const Json::Value & config_obj = value[1];
    json_check_string(pattern_obj, "field in schema pattern");
    json_check_object(config_obj, "config in schema pattern");

    string pattern(pattern_obj.asString());
    if (string_startswith(pattern, "*")) {
	leading_wildcard = true;
	pattern = pattern.substr(1);
    } else {
	leading_wildcard = false;
    }
    if (pattern.find("*") != pattern.npos) {
	throw InvalidValueError("fields in schema patterns must not contain a * other than at the start");
    }

    ending = pattern;
    config = config_obj;
}

Json::Value &
FieldConfigPattern::to_json(Json::Value & value) const
{
    value = Json::arrayValue;
    if (leading_wildcard) {
	value.append("*" + ending);
    } else {
	value.append(ending);
    }
    value.append(config);
    return value;
}

FieldConfig *
FieldConfigPattern::test(const string & fieldname,
			 const string & doc_type) const
{
    if (leading_wildcard) {
	if (string_endswith(fieldname, ending)) {
	    string prefix(fieldname.substr(0, fieldname.size() - ending.size()));
	    Json::Value newconfig;
	    for (Json::Value::iterator i = config.begin(); i != config.end(); ++i) {
		Json::Value & item = newconfig[i.memberName()] = *i;
		if (item.isString()) {
		    // Substitute any * characters in the config with the prefix.
		    string itemstr(item.asString());
		    size_t starpos(itemstr.find("*"));
		    if (starpos != itemstr.npos) {
			if (starpos + 1 < itemstr.size()) {
			    item = itemstr.substr(0, starpos) + prefix +
				    itemstr.substr(starpos + 1);
			} else {
			    item = itemstr.substr(0, starpos) + prefix;
			}
		    }
		}
	    }
	    return FieldConfig::from_json(newconfig, doc_type);
	}
    } else {
	if (fieldname == ending) {
	    return FieldConfig::from_json(config, doc_type);
	}
    }
    return NULL;
}

FieldConfigPatterns::FieldConfigPatterns()
{
}

void
FieldConfigPatterns::from_json(const Json::Value & value)
{
    patterns.clear();
    json_check_array(value, "schema pattern list");
    for (Json::Value::const_iterator
	 i = value.begin(); i != value.end(); ++i) {
	patterns.push_back(FieldConfigPattern());
	patterns.back().from_json(*i);
    }
}

Json::Value &
FieldConfigPatterns::to_json(Json::Value & value) const
{
    value = Json::arrayValue;
    for (vector<FieldConfigPattern>::const_iterator
	 i = patterns.begin(); i != patterns.end(); ++i) {
	i->to_json(value.append(Json::arrayValue));
    }
    return value;
}

void
FieldConfigPatterns::merge_from(const FieldConfigPatterns & other)
{
    // Very simple merge algorithm for now - if the other schema has any
    // patterns, replace all our patterns with its.
    if (!other.patterns.empty()) {
	patterns = other.patterns;
    }
}

FieldConfig *
FieldConfigPatterns::get(const string & fieldname,
			 const string & doc_type) const
{
    LOG_DEBUG("Searching through " + str(patterns.size()) +
	      " patterns for field config pattern matching '" +
	      fieldname + "'");
    for (vector<FieldConfigPattern>::const_iterator
	 i = patterns.begin(); i != patterns.end(); ++i) {
	FieldConfig * result = i->test(fieldname, doc_type);
	if (result != NULL) {
	    return result;
	}
    }
    LOG_DEBUG("No matching pattern found");
    return NULL;
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
	Json::Value & subval(value["fields"]);
	subval = Json::objectValue;
	map<string, FieldConfig *>::const_iterator i;
	for (i = fields.begin(); i != fields.end(); ++i) {
	    i->second->to_json(subval[i->first] = Json::objectValue);
	}
    }
    patterns.to_json(value["patterns"]);
    return value;
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
	    set(i.memberName(), FieldConfig::from_json(*i, doc_type));
	}
    }
    patterns.from_json(value.get("patterns", Json::Value::null));
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
	    set(i->first, FieldConfig::from_json(tmp, doc_type));
	} else {
	    // Complain if configuration is not identical.
	    //
	    // Note, if this is changed in future to allow field
	    // configuration to be changed, don't forget to clear the cache
	    // (in indexers).

	    Json::Value tmp2;
	    j->second->to_json(tmp2);
	    if (json_serialise(tmp) != json_serialise(tmp2)) {
		throw InvalidValueError("Cannot change configuration in schema for field \"" + i->first + "\".");
	    }
	}
    }
    patterns.merge_from(other.patterns);
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
	LOG_DEBUG("Removing config for field '" + fieldname + "'");
	map<string, FieldConfig *>::iterator i;
	i = fields.find(fieldname);
	if (i != fields.end()) {
	    delete i->second;
	    i->second = NULL;
	    fields.erase(i);
	}
	return;
    }

    LOG_DEBUG("Setting config for field '" + fieldname + "'");
    auto_ptr<FieldConfig> configptr(config);
    pair<string, FieldConfig*> item(fieldname, NULL);
    pair<map<string, FieldConfig *>::iterator, bool> ret;
    ret = fields.insert(item);
    delete(ret.first->second);
    ret.first->second = configptr.release();
}

void
Schema::get_taxonomy_groups(const string & taxonomy_name,
			    std::set<string> & result) const
{
    // iterate through fields, looking for category fields which have taxonomy
    // equal to taxonomy_name, and add the groups of these to the result.
    for (map<string, FieldConfig *>::const_iterator i = fields.begin();
	 i != fields.end(); ++i) {
	i->second->add_group_if_taxonomy(taxonomy_name, result);
    }
}

Xapian::Document
Schema::process(const Json::Value & value,
		const CollectionConfig & collconfig,
		string & idterm,
		IndexingErrors & errors,
		bool & new_fields)
{
    json_check_object(value, "input document");

    IndexingState state(collconfig, idterm, errors);

    string meta_field(collconfig.get_meta_field());

    for (Json::Value::const_iterator viter = value.begin();
	 viter != value.end();
	 ++viter) {
	const string & fieldname = viter.memberName();

	if (fieldname == meta_field) {
	    state.append_error(fieldname, "Value provided in metadata field - "
			       "should be empty");
	    continue;
	}

	const FieldIndexer * indexer = get_indexer(fieldname);
	if (!indexer) {
	    LOG_DEBUG(string("New field type: ") + fieldname);
	    set(fieldname, patterns.get(fieldname, doc_type));
	    new_fields = true;
	    indexer = get_indexer(fieldname);
	}

	if (indexer) {
	    if ((*viter).isNull()) {
		state.field_empty(fieldname);
		continue;
	    }
	    if ((*viter).isArray()) {
		indexer->index(state, fieldname, *viter);
	    } else {
		Json::Value arrayval(Json::arrayValue);
		arrayval.append(*viter);
		indexer->index(state, fieldname, arrayval);
	    }
	}
    }

    if (!meta_field.empty()) {
	const FieldIndexer * indexer = get_indexer(meta_field);
	if (!indexer) {
	    LOG_DEBUG(string("New meta field type: ") + meta_field);
	    set(meta_field, patterns.get(meta_field, doc_type));
	    new_fields = true;
	    indexer = get_indexer(meta_field);
	}
	if (indexer) {
	    indexer->index(state, meta_field, Json::nullValue);
	}
    }

    state.doc.set_data(state.docdata.serialise());
    state.docvals.apply(state.doc);
    return state.doc;
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
Schema::display_doc(const Xapian::Document & doc,
		    const Json::Value & fieldlist,
		    Json::Value & result) const
{
    json_check_array(fieldlist, "display field list");
    result = Json::objectValue;
    DocumentData docdata;
    docdata.unserialise(doc.get_data());
    for (Json::Value::const_iterator fiter = fieldlist.begin();
	 fiter != fieldlist.end();
	 ++fiter) {
	if (!(*fiter).isString()) {
	    throw InvalidValueError("Item in display field list was not a string");
	}
	string fieldname((*fiter).asString());
	string val(docdata.get(fieldname));
	if (!val.empty()) {
	    Json::Value tmp;
	    result[fieldname] = json_unserialise(val, tmp);
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
	if (i->second != NULL) {
	    string stored_field = i->second->stored_field();
	    if (!stored_field.empty()) {
		fieldlist.append(stored_field);
	    }
	}
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
