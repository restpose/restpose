/** @file occurinfohandler.cc
 * @brief Information handler for counting occurrences of terms.
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
#include "jsonxapian/occurinfohandler.h"

#include <limits.h>
#include "matchspies/termoccurmatchspy.h"
#include "utils/jsonutils.h"

using namespace RestPose;
using namespace std;

BaseOccurInfoHandler::~BaseOccurInfoHandler()
{
    delete spy;
}

void
BaseOccurInfoHandler::write_results(Json::Value & results,
				    const Xapian::MSet &) const
{
    if (!results.isMember("info")) {
	results["info"] = Json::arrayValue;
    }
    Json::Value & info = results["info"].append(Json::objectValue);
    spy->get_result(info);
}


OccurInfoHandler::OccurInfoHandler(const Json::Value & params,
				   Xapian::Enquire & enq,
				   const Xapian::Database * db_,
				   const Schema *,
				   Xapian::doccount & checkatleast)
	: BaseOccurInfoHandler(db_)
{
    string prefix = json_get_string_member(params, "prefix", "");
    Xapian::doccount doc_limit = json_get_uint64_member(params,
	"doc_limit", UINT_MAX, db->get_doccount());
    Xapian::doccount result_limit = json_get_uint64_member(params,
	"result_limit", UINT_MAX, UINT_MAX);
    bool get_termfreqs = json_get_bool(params, "get_termfreqs", false);
    spy = new TermOccurMatchSpy(prefix, doc_limit, result_limit,
				get_termfreqs, db);
    const Json::Value & stopwords = params["stopwords"];
    if (!stopwords.isNull()) {
	json_check_array(stopwords, "list of stopwords");
	for (Json::Value::const_iterator i = stopwords.begin();
	     i != stopwords.end(); ++i) {
	    const Json::Value & stopword(*i);
	    json_check_string(stopword, "stopword");
	    spy->add_stopword(stopword.asString());
	}
    }

    if (checkatleast < result_limit) {
	checkatleast = result_limit;
    }
    enq.add_matchspy(spy);
}

CoOccurInfoHandler::CoOccurInfoHandler(const Json::Value & params,
				       Xapian::Enquire & enq,
				       const Xapian::Database * db_,
				       const Schema *,
				       Xapian::doccount & checkatleast)
	: BaseOccurInfoHandler(db_)
{
    string prefix = json_get_string_member(params, "prefix", "");
    Xapian::doccount doc_limit = json_get_uint64_member(params,
	"doc_limit", UINT_MAX, db->get_doccount());
    Xapian::doccount result_limit = json_get_uint64_member(params,
	"result_limit", UINT_MAX, UINT_MAX);
    bool get_termfreqs = json_get_bool(params, "get_termfreqs", false);
    spy = new TermCoOccurMatchSpy(prefix, doc_limit, result_limit,
				  get_termfreqs, db);
    const Json::Value & stopwords = params["stopwords"];
    if (!stopwords.isNull()) {
	json_check_array(stopwords, "list of stopwords");
	for (Json::Value::const_iterator i = stopwords.begin();
	     i != stopwords.end(); ++i) {
	    const Json::Value & stopword(*i);
	    json_check_string(stopword, "stopword");
	    spy->add_stopword(stopword.asString());
	}
    }


    if (checkatleast < result_limit) {
	checkatleast = result_limit;
    }
    enq.add_matchspy(spy);
}
