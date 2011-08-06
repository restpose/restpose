/** @file indexing.cc
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

#include <config.h>

#include "indexing.h"

#include <cjk-tokenizer.h>
#include <cmath>
#include <cstdlib>
#include <logger/logger.h>
#include <map>
#include "str.h"
#include <string>
#include <xapian.h>

#include "docdata.h"
#include "hashterm.h"
#include "jsonxapian/collconfig.h"
#include "jsonxapian/taxonomy.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/validation.h"

using namespace RestPose;
using namespace std;

void
IndexingState::set_idterm(const std::string & fieldname,
			  const std::string & idterm_)
{
    if (idterm.empty()) {
	idterm = idterm_;
    } else if (!idterm_.empty()) {
	append_error(fieldname, "Multiple ID values provided - must have only one");
    }
}

FieldIndexer::~FieldIndexer()
{}

MetaIndexer::~MetaIndexer()
{}

void
MetaIndexer::index(IndexingState & state,
		   const string &,
		   const Json::Value &) const
{
    // Generate terms with the following codes
    // F: fields which exist
    // N: fields which are non empty
    // M: fields which are empty
    // E: fields which had errors
    bool had_nonempty(false);
    bool had_empty(false);
    bool had_errors(false);
    for (map<string, FieldPresence>::const_iterator i = state.presence.begin();
	 i != state.presence.end(); ++i) {
	if (i->first == state.collconfig.get_id_field() ||
	    i->first == state.collconfig.get_type_field()) {
	    continue;
	}
	const string & fieldname(i->first);
	state.doc.add_term(prefix + "F" + fieldname, 0);
	if (i->second.nonempty) {
	    state.doc.add_term(prefix + "N" + fieldname, 0);
	    had_nonempty = true;
	}
	if (i->second.empty) {
	    state.doc.add_term(prefix + "M" + fieldname, 0);
	    had_empty = true;
	}
	if (i->second.errors) {
	    state.doc.add_term(prefix + "E" + fieldname, 0);
	    had_errors = true;
	}
    }
    if (had_nonempty) {
	state.doc.add_term(prefix + "N", 0);
    }
    if (had_empty) {
	state.doc.add_term(prefix + "M", 0);
    }
    if (had_errors) {
	state.doc.add_term(prefix + "E", 0);
    }
}

ExactStringIndexer::~ExactStringIndexer()
{}

void
ExactStringIndexer::index(IndexingState & state,
			  const std::string & fieldname,
			  const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {

	std::string error;
	std::string val = json_get_idstyle_value(*i, error);
	if (!error.empty()) {
	    state.append_error(fieldname, error);
	    continue;
	}
	if (val.empty()) {
	    state.field_empty(fieldname);
	    continue;
	}
	if (isid) {
	    error = validate_doc_id(val);
	    if (!error.empty()) {
		state.append_error(fieldname, error);
		state.errors.total_failure = true;
		return;
	    }
	}
	state.field_nonempty(fieldname);
	if (val.size() > max_length) {
	    switch (too_long_action) {
		case MaxLenFieldConfig::TOOLONG_ERROR:
		    state.append_error(fieldname,
			"Field value of length " +
			str(val.size()) +
			" exceeds maximum permissible length for this field "
			"of " + str(max_length));
		     continue;
		case MaxLenFieldConfig::TOOLONG_HASH:
		    // Note - this isn't UTF-8 aware.
		    val = hash_long_term(val, max_length);
		    break;
		case MaxLenFieldConfig::TOOLONG_TRUNCATE:
		    // Note - this isn't UTF-8 aware.
		    val.erase(max_length);
		    break;
	    }
	}
	state.doc.add_term(prefix + val, wdfinc);
	if (isid) {
	    state.set_idterm(fieldname, prefix + val);
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}

StoredIndexer::~StoredIndexer()
{}

void
StoredIndexer::index(IndexingState & state,
		     const std::string & fieldname,
		     const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	if ((*i).empty() || ((*i).isString() && (*i).asString().empty())) {
	    state.field_empty(fieldname);
	} else {
	    state.field_nonempty(fieldname);
	}
    }
    state.docdata.set(store_field, json_serialise(values));
}


DoubleIndexer::~DoubleIndexer()
{}

void
DoubleIndexer::index(IndexingState & state,
		     const std::string & fieldname,
		     const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	if ((*i).isNull()) {
	    state.field_empty(fieldname);
	} else if ((*i).isConvertibleTo(Json::realValue)) {
	    state.field_nonempty(fieldname);
	    state.docvals.add(slot,
			      Xapian::sortable_serialise((*i).asDouble()));
	} else {
	    state.append_error(fieldname, "Double field must be numeric");
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}


TimeStampIndexer::~TimeStampIndexer()
{}

void
TimeStampIndexer::index(IndexingState & state,
			const std::string & fieldname,
			const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	if ((*i).isNull()) {
	    state.field_empty(fieldname);
	} else if ((*i).isConvertibleTo(Json::realValue)) {
	    state.field_nonempty(fieldname);
	    state.docvals.add(slot,
			      Xapian::sortable_serialise((*i).asDouble()));
	} else {
	    state.append_error(fieldname, "Timestamp field must be numeric");
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}


DateIndexer::~DateIndexer()
{}

void
DateIndexer::index(IndexingState & state,
		   const std::string & fieldname,
		   const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	std::string error;
	std::string parsed = parse_date(*i, error);
	if (!error.empty()) {
	    state.append_error(fieldname, error);
	} else if (parsed.empty()) {
	    state.field_empty(fieldname);
	} else {
	    state.field_nonempty(fieldname);
	    state.docvals.add(slot, parsed);
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}

std::string
DateIndexer::parse_date(const Json::Value & value, std::string & error)
{
    if (value.isNull()) {
	return std::string();
    }

    if (!value.isString()) {
	error = "Non-string value supplied to date field.";
	return std::string();
    }
    std::string value_str(value.asString());

    if (value_str.empty()) {
	return std::string();
    }

    // FIXME - extremely lax parsing now follows.  Should report parse
    // errors.  Parses only the format year-month-day

    char * endptr = NULL;
    double year = strtod(value_str.c_str(), &endptr);

    LOG_DEBUG("Year: " + str(year));
    if (*endptr == '-') {
	++endptr;
    }
    int month = floor(strtod(endptr, &endptr));
    LOG_DEBUG("Month: " + str(month));
    if (month <= 0 || month > 12) {
	error = "Unable to parse date value; got month value (" + str(month) +
		") out of range";
	return std::string();
    }

    if (*endptr == '-') {
	++endptr;
    }
    int day = floor(strtod(endptr, &endptr));
    LOG_DEBUG("Day: " + str(day));
    if (day <= 0 || day > 31) {
	error = "Unable to parse date value; got day value (" + str(day) +
		") out of range";
	return std::string();
    }

    return Xapian::sortable_serialise(year) +
	    std::string(1, ' ' + month) +
	    std::string(1, ' ' + day);
}


CategoryIndexer::~CategoryIndexer()
{}

void
CategoryIndexer::index(IndexingState & state,
		       const std::string & fieldname,
		       const Json::Value & values) const
{
    const Taxonomy * taxonomy =
	    state.collconfig.get_taxonomy(taxonomy_name);
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {

	std::string error;
	std::string val = json_get_idstyle_value(*i, error);
	if (!error.empty()) {
	    state.append_error(fieldname, error);
	    continue;
	}
	if (val.empty()) {
	    state.field_empty(fieldname);
	    continue;
	}
	state.field_nonempty(fieldname);
	if (val.size() > max_length) {
	    switch (too_long_action) {
		case MaxLenFieldConfig::TOOLONG_ERROR:
		    state.append_error(fieldname,
			"Field value of length " + str(val.size()) +
			" exceeds maximum permissible length for this field "
			"of " + str(max_length));
		     continue;
		case MaxLenFieldConfig::TOOLONG_HASH:
		    // Note - this isn't UTF-8 aware.
		    val = hash_long_term(val, max_length);
		    break;
		case MaxLenFieldConfig::TOOLONG_TRUNCATE:
		    // Note - this isn't UTF-8 aware.
		    val.erase(max_length);
		    break;
	    }
	}
	state.doc.add_term(prefix + "C" + val, 0);
	if (taxonomy != NULL) {
	    // Add terms for the parent categories.
	    const Category * cat_ptr = taxonomy->find(val);
	    if (cat_ptr != NULL) {
		for (Categories::const_iterator j = cat_ptr->ancestors.begin();
		     j != cat_ptr->ancestors.end(); ++j) {
		    state.doc.add_term(prefix + "A" + *j, 0);
		}
	    }
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}


TermGeneratorIndexer::~TermGeneratorIndexer()
{}

void
TermGeneratorIndexer::index(IndexingState & state,
			    const std::string & fieldname,
			    const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	if ((*i).isNull()) {
	    state.field_empty(fieldname);
	    continue;
	} else if (!(*i).isString()) {
	    state.append_error(fieldname,
			       "Field value for text field must be a string");
	    continue;
	}
	std::string val = (*i).asString();
	if (val.empty()) {
	    state.field_empty(fieldname);
	    continue;
	}
	state.field_nonempty(fieldname);

	Xapian::TermGenerator tg;
	tg.set_stemmer(Xapian::Stem(stem_lang));
	tg.set_document(state.doc);
	tg.index_text(val, 1 /*weight*/, prefix);
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}

CJKIndexer::~CJKIndexer()
{}

void
CJKIndexer::index(IndexingState & state,
		  const std::string & fieldname,
		  const Json::Value & values) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	if ((*i).isNull()) {
	    state.field_empty(fieldname);
	    continue;
	} else if (!(*i).isString()) {
	    state.append_error(fieldname,
			       "Field value for text field must be a string");
	    continue;
	}
	std::string val = (*i).asString();
	if (val.empty()) {
	    state.field_empty(fieldname);
	    continue;
	}
	state.field_nonempty(fieldname);

	// index the text
	cjk::tokenizer tknzr;
	std::vector<std::pair<std::string, unsigned> > token_list;
	std::vector<std::pair<std::string, unsigned> >::const_iterator token_iter;
	tknzr.tokenize(val, token_list);
	unsigned int offset = 0;
	for (token_iter = token_list.begin();
	     token_iter != token_list.end(); ++token_iter) {
	    std::string term(Xapian::Unicode::tolower(token_iter->first));
	    if (term.size() < 32) {
		state.doc.add_posting(prefix + term, token_iter->second + offset);
	    }
	}
    }

    if (!store_field.empty()) {
	state.docdata.set(store_field, json_serialise(values));
    }
}
