/** @file facetmatchspy.cc
 * @brief MatchSpy classes for faceting.
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
#include "facetmatchspy.h"

#include <algorithm>
#include "serialise.h"
#include "utils/stringutils.h"
#include <vector>

using namespace RestPose;
using namespace std;

BaseFacetMatchSpy::BaseFacetMatchSpy(SlotDecoder * decoder_,
				     const string & fieldname_,
				     Xapian::doccount doc_limit_)
	: decoder(decoder_),
	  fieldname(fieldname_),
	  docs_seen(0),
	  doc_limit(doc_limit_),
	  values_seen(0)
{
}

BaseFacetMatchSpy::~BaseFacetMatchSpy()
{
    delete decoder;
}


void
FacetCountMatchSpy::operator()(const Xapian::Document &doc, Xapian::weight)
{
    if (docs_seen >= doc_limit) return;
    ++docs_seen;
    decoder->newdoc(doc);

    const char * pos;
    size_t len;
    while (decoder->next(&pos, &len)) {
	++values_seen;
	++counts[string(pos, len)];
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
FacetCountMatchSpy::append_value(Json::Value & rcounts,
				 const std::string & str,
				 Xapian::doccount freq) const
{
    Json::Value tmp(Json::arrayValue);
    tmp.append(str);
    tmp.append(freq);
    rcounts.append(tmp);
}

void
FacetCountMatchSpy::get_result(Json::Value & result) const
{
    result = Json::objectValue;
    result["type"] = "facet_count";
    result["fieldname"] = fieldname;
    result["docs_seen"] = docs_seen;
    result["values_seen"] = values_seen;
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

    for (j = 0, k = sorted.begin();
	 j != result_limit && k != sorted.end();
	 ++j, ++k) {
	append_value(rcounts, k->str, k->freq);
    }
}

void
DateFacetCountMatchSpy::append_value(Json::Value & rcounts,
				     const std::string & str,
				     Xapian::doccount freq) const
{
    Json::Value tmp(Json::arrayValue);
    if (str.size() > 2) {
	int day = str[str.size() - 1] - ' ';
	int month = str[str.size() - 2] - ' ';
	Json::Value & tmp2(tmp.append(Json::arrayValue));
	// Unpack the date
	tmp2.append(int(Xapian::sortable_unserialise(str)));
	tmp2.append(month);
	tmp2.append(day);
    } else {
	// Shouldn't happen, but hexesc will ensure it's a valid JSON output.
	tmp.append(hexesc(str));
    }
    tmp.append(freq);
    rcounts.append(tmp);
}
