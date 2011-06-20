/** @file termoccurmatchspy.cc
 * @brief MatchSpy for counting the occurrences of terms.
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
#include "termoccurmatchspy.h"

#include <algorithm>
#include "utils/stringutils.h"
#include <vector>

using namespace RestPose;
using namespace std;

BaseTermOccurMatchSpy::BaseTermOccurMatchSpy(const string & prefix_,
					     Xapian::doccount doc_limit_,
					     Xapian::doccount result_limit_,
					     bool get_termfreqs_,
					     const Xapian::Database * db_)
	: docs_seen(0),
	  doc_limit(doc_limit_),
	  terms_seen(0),
	  result_limit(result_limit_),
	  prefix(prefix_),
	  orig_prefix(prefix_),
	  get_termfreqs(get_termfreqs_),
	  db(db_)
{
    if (!prefix.empty()) {
	prefix += "\t";
    }
}

void
BaseTermOccurMatchSpy::add_stopword(const std::string & word)
{
    stopwords.insert(word);
}


void
TermOccurMatchSpy::operator()(const Xapian::Document &doc, Xapian::weight)
{
    if (docs_seen >= doc_limit) return;
    ++docs_seen;
    Xapian::TermIterator i = doc.termlist_begin();
    if (i == doc.termlist_end()) {
	return;
    }
    i.skip_to(prefix);
    while (i != doc.termlist_end()) {
	if (!string_startswith(*i, prefix)) {
	    break;
	}
	string suffix((*i).substr(prefix.size()));
	if (stopwords.find(suffix) == stopwords.end()) {
	    ++counts[suffix];
	    ++terms_seen;
	}
	++i;
    }
}

struct StringAndFreq {
    string str;
    Xapian::doccount freq;

    StringAndFreq(const string & str_, Xapian::doccount freq_)
	    : str(str_), freq(freq_)
    {}

    /** Compare in reverse order, so that sorting puts these most-frequent
     *  first.
     */
    bool operator<(const StringAndFreq & other) const {
	return freq > other.freq;
    }
};

void
TermOccurMatchSpy::get_result(Json::Value & result) const
{
    result = Json::objectValue;
    result["type"] = "occur";
    result["prefix"] = orig_prefix;
    result["docs_seen"] = docs_seen;
    result["terms_seen"] = terms_seen;
    Json::Value & rcounts = result["counts"] = Json::arrayValue;

    // FIXME - we should only sort the top items (which are wanted) here; see
    // get_most_frequent_items in xapian-core/api/matchspy.cc for an approach
    // which avoids sorting everything.

    vector<StringAndFreq> sorted;
    sorted.reserve(counts.size());
    for (std::map<std::string, Xapian::doccount>::const_iterator
	 i = counts.begin(); i != counts.end(); ++i) {
	sorted.push_back(StringAndFreq(i->first, i->second));
    }
    sort(sorted.begin(), sorted.end());

    Xapian::doccount j;
    vector<StringAndFreq>::const_iterator k;

    // Get the termfreqs, if they're wanted.
    // We want to get the termfreqs in sorted order, to try and reduce random
    // seeking in the lookup.
    map<string, Xapian::doccount> termfreqs;
    if (get_termfreqs) {
	for (j = 0, k = sorted.begin();
	     j != result_limit && k != sorted.end();
	     ++j, ++k) {
	    termfreqs[k->str] = 0;
	}
	Xapian::TermIterator ti = db->allterms_begin();
	for (map<string, Xapian::doccount>::iterator l = termfreqs.begin();
	     l != termfreqs.end(); ++l) {
	    ti.skip_to(prefix + l->first);
	    if (ti == db->allterms_end() || !string_startswith(*ti, prefix)) {
		break;
	    }
	    if (*ti == prefix + l->first) {
		l->second = ti.get_termfreq();
	    }
	}
    }

    for (j = 0, k = sorted.begin();
	 j != result_limit && k != sorted.end();
	 ++j, ++k) {
	Json::Value tmp(Json::arrayValue);
	tmp.append(k->str);
	tmp.append(k->freq);
	if (get_termfreqs) {
	    tmp.append(termfreqs[k->str]);
	}
	rcounts.append(tmp);
    }
}

void
TermCoOccurMatchSpy::operator()(const Xapian::Document &doc, Xapian::weight)
{
    if (docs_seen >= doc_limit) return;
    ++docs_seen;
    Xapian::TermIterator i = doc.termlist_begin();
    if (i == doc.termlist_end()) {
	return;
    }
    vector<string> items;
    i.skip_to(prefix);
    while (i != doc.termlist_end()) {
	if (!string_startswith(*i, prefix)) {
	    break;
	}
	string suffix((*i).substr(prefix.size()));
	if (stopwords.find(suffix) == stopwords.end()) {
	    items.push_back((*i).substr(prefix.size()));
	    ++terms_seen;
	}
	++i;
    }
    for (vector<string>::const_iterator j = items.begin();
	 j != items.end(); ++j) {
	vector<string>::const_iterator k = j;
	for (++k; k != items.end(); ++k) {
	    ++counts[*j + string("\0", 1) + *k];
	}
    }
}

struct PairAndFreq {
    string str1;
    string str2;
    Xapian::doccount freq;

    PairAndFreq(const string & strs, Xapian::doccount freq_)
	    : freq(freq_)
    {
	size_t zeropos = strs.find('\0');
	str1 = strs.substr(0, zeropos);
	str2 = strs.substr(zeropos + 1);
    }

    /** Compare in reverse order, so that sorting puts these most-frequent
     *  first.
     */
    bool operator<(const PairAndFreq & other) const {
	return freq > other.freq;
    }
};

void
TermCoOccurMatchSpy::get_result(Json::Value & result) const
{
    result = Json::objectValue;
    result["type"] = "cooccur";
    result["prefix"] = orig_prefix;
    result["docs_seen"] = docs_seen;
    result["terms_seen"] = terms_seen;
    Json::Value & rcounts = result["counts"] = Json::arrayValue;

    // FIXME - we should only sort the top items (which are wanted) here; see
    // get_most_frequent_items in xapian-core/api/matchspy.cc for an approach
    // which avoids sorting everything.

    vector<PairAndFreq> sorted;
    sorted.reserve(counts.size());
    for (std::map<std::string, Xapian::doccount>::const_iterator
	 i = counts.begin(); i != counts.end(); ++i) {
	sorted.push_back(PairAndFreq(i->first, i->second));
    }
    sort(sorted.begin(), sorted.end());

    Xapian::doccount j;
    vector<PairAndFreq>::const_iterator k;

    // Get the termfreqs, if they're wanted.
    // We want to get the termfreqs in sorted order, to try and reduce random
    // seeking in the lookup.
    map<string, Xapian::doccount> termfreqs;
    if (get_termfreqs) {
	for (j = 0, k = sorted.begin();
	     j != result_limit && k != sorted.end();
	     ++j, ++k) {
	    termfreqs[k->str1] = 0;
	    termfreqs[k->str2] = 0;
	}
	Xapian::TermIterator ti = db->allterms_begin();
	for (map<string, Xapian::doccount>::iterator l = termfreqs.begin();
	     l != termfreqs.end(); ++l) {
	    ti.skip_to(prefix + l->first);
	    if (ti == db->allterms_end() || !string_startswith(*ti, prefix)) {
		break;
	    }
	    if (*ti == prefix + l->first) {
		l->second = ti.get_termfreq();
	    }
	}
    }

    for (j = 0, k = sorted.begin();
	 j != result_limit && k != sorted.end();
	 ++j, ++k) {
	Json::Value tmp(Json::arrayValue);
	tmp.append(k->str1);
	tmp.append(k->str2);
	tmp.append(k->freq);
	if (get_termfreqs) {
	    tmp.append(termfreqs[k->str1]);
	    tmp.append(termfreqs[k->str2]);
	}
	rcounts.append(tmp);
    }
}
