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
#include <json/writer.h>
#include <string>
#include <map>

#include "docdata.h"
#include "hashterm.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

FieldIndexer::~FieldIndexer()
{}

ExactStringIndexer::~ExactStringIndexer()
{}

void
ExactStringIndexer::index(Xapian::Document & doc,
			  DocumentData & docdata,
			  const Json::Value & values,
			  std::string & idterm) const
{
    if (isid && values.size() != 1) {
	throw InvalidValueError("Multiple values supplied to ID field - must have only one");
    }
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {

	std::string val = json_get_idstyle_value(*i);
	if (val.size() > max_length) {
	    switch (too_long_action) {
		case MaxLenFieldConfig::TOOLONG_ERROR:
		    throw InvalidValueError("Field value of length " +
					    Json::valueToString(Json::UInt64(val.size())) +
					    " exceeds maximum permissible length"
					    " for this field of " +
					    Json::valueToString(Json::UInt64(max_length)));
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
	doc.add_term(prefix + val, wdfinc);
	if (isid) {
	    idterm = prefix + val;
	}
    }

    if (!store_field.empty()) {
	Json::FastWriter writer;
	std::string storeval = writer.write(values);
	docdata.set(store_field, storeval);
    }
}

StoredIndexer::~StoredIndexer()
{}

void
StoredIndexer::index(Xapian::Document &,
		     DocumentData & docdata,
		     const Json::Value & values,
		     std::string &) const
{
    Json::FastWriter writer;
    std::string val = writer.write(values);
    docdata.set(store_field, val);
}

DateIndexer::~DateIndexer()
{}

void
DateIndexer::index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & values,
		   std::string &) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	unsigned int secs = json_get_uint64(*i);
	doc.add_value(slot, Xapian::sortable_serialise(secs));
    }

    if (!store_field.empty()) {
	Json::FastWriter writer;
	std::string storeval = writer.write(values);
	docdata.set(store_field, storeval);
    }
}

TermGeneratorIndexer::~TermGeneratorIndexer()
{}

void
TermGeneratorIndexer::index(Xapian::Document & doc,
			    DocumentData & docdata,
			    const Json::Value & values,
			    std::string &) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	json_check_string(*i, "Text field value");
	std::string val = (*i).asString();

	Xapian::TermGenerator tg;
	tg.set_stemmer(Xapian::Stem(stem_lang));
	tg.set_document(doc);
	tg.index_text(val, 1 /*weight*/, prefix);
    }

    if (!store_field.empty()) {
	Json::FastWriter writer;
	std::string storeval = writer.write(values);
	docdata.set(store_field, storeval);
    }
}

CJKIndexer::~CJKIndexer()
{}

void
CJKIndexer::index(Xapian::Document & doc,
		  DocumentData & docdata,
		  const Json::Value & values,
		  std::string &) const
{
    for (Json::Value::const_iterator i = values.begin();
	 i != values.end(); ++i) {
	json_check_string(*i, "Text field value");
	std::string val = (*i).asString();

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
		doc.add_posting(prefix + term, token_iter->second + offset);
	    }
	}
    }

    if (!store_field.empty()) {
	Json::FastWriter writer;
	std::string storeval = writer.write(values);
	docdata.set(store_field, storeval);
    }
}
