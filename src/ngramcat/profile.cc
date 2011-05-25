/** @file profile.cc
 * @brief Ngram profiles.
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
/* Parts of this file copied from termgenerator.cc in xapian-core:
 *
 * Copyright (C) 2007,2010 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>
#include "ngramcat/profile.h"

#include <algorithm>
#include "utils/jsonutils.h"
#include <limits.h>
#include <xapian/unicode.h>

using namespace RestPose;
using namespace Xapian;

inline unsigned check_wordchar(unsigned ch) {
    if (Xapian::Unicode::is_wordchar(ch)) {
	return Xapian::Unicode::tolower(ch);
    }
    return 0;
}

/** Value representing "ignore this" when returned by check_infix() or
 *  check_infix_digit().
 */
const unsigned UNICODE_IGNORE(-1);

inline unsigned check_infix(unsigned ch) {
    if (ch == '\'' || ch == '&' || ch == 0xb7 || ch == 0x5f4 || ch == 0x2027) {
	// Unicode includes all these except '&' in its word boundary rules,
	// as well as 0x2019 (which we handle below) and ':' (for Swedish
	// apparently, but we ignore this for now as it's problematic in
	// real world cases).
	return ch;
    }
    // 0x2019 is Unicode apostrophe and single closing quote.
    // 0x201b is Unicode single opening quote with the tail rising.
    if (ch == 0x2019 || ch == 0x201b) return '\'';
    if (ch >= 0x200b && (ch <= 0x200d || ch == 0x2060 || ch == 0xfeff))
	return UNICODE_IGNORE;
    return 0;
}

inline unsigned check_infix_digit(unsigned ch) {
    // This list of characters comes from Unicode's word identifying algorithm.
    switch (ch) {
	case ',':
	case '.':
	case ';':
	case 0x037e: // GREEK QUESTION MARK
	case 0x0589: // ARMENIAN FULL STOP
	case 0x060D: // ARABIC DATE SEPARATOR
	case 0x07F8: // NKO COMMA
	case 0x2044: // FRACTION SLASH
	case 0xFE10: // PRESENTATION FORM FOR VERTICAL COMMA
	case 0xFE13: // PRESENTATION FORM FOR VERTICAL COLON
	case 0xFE14: // PRESENTATION FORM FOR VERTICAL SEMICOLON
	    return ch;
    }
    if (ch >= 0x200b && (ch <= 0x200d || ch == 0x2060 || ch == 0xfeff))
	return UNICODE_IGNORE;
    return 0;
}

inline bool
is_digit(unsigned ch) {
    return (Unicode::get_category(ch) == Unicode::DECIMAL_DIGIT_NUMBER);
}

inline unsigned check_suffix(unsigned ch) {
    if (ch == '+' || ch == '#') return ch;
    // FIXME: what about '-'?
    return 0;
}

void
NGramProfile::init_from_sorted_ngram(const SortedNGramProfile & other)
{
    max_ngrams = other.max_ngrams;
    positions.clear();

    std::vector<std::string>::const_iterator i;
    unsigned int pos;
    for (i = other.ngrams.begin(), pos = 0;
	 i != other.ngrams.end();
	 ++i, ++pos) {
	positions[*i] = pos;
    }
}

void
NGramProfile::to_json(Json::Value & value) const
{
    SortedNGramProfile sorted;
    sorted.init_from_ngram(*this);
    sorted.to_json(value);
}

void
NGramProfile::from_json(const Json::Value & value)
{
    SortedNGramProfile sorted;
    sorted.from_json(value);
    init_from_sorted_ngram(sorted);
}

unsigned int
SortedNGramProfile::distance(const NGramProfile & other) const
{
    unsigned int ngram_count = std::min(max_ngrams, other.max_ngrams);
    unsigned int count = 0;
    unsigned int ngrams_size = ngrams.size();

    // If we're short on ngrams, all the missing ones incur the maximum score.
    if (ngrams_size < ngram_count) {
	count += (ngram_count - ngrams_size) * ngram_count;
    }

    unsigned int len = std::min(ngrams_size, ngram_count);
    for (unsigned int i = 0; i < len; ++i) {
	std::map<std::string, unsigned int>::const_iterator
		j = other.positions.find(ngrams[i]);
	if (j == other.positions.end()) {
	    count += ngram_count;
	} else {
	    count += abs(j->second - i);
	}
    }

    return count;
}

void
SortedNGramProfile::init_from_ngram(const NGramProfile & other)
{
    max_ngrams = other.max_ngrams;
    ngrams.clear();
    ngrams.resize(other.positions.size());
    for (std::map<std::string, unsigned int>::const_iterator
	 i = other.positions.begin(); i != other.positions.end(); ++i) {
	ngrams[i->second] = i->first;
    }
}

void
SortedNGramProfile::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    Json::Value & ngram_obj = value["ngrams"] = Json::arrayValue;
    for (std::vector<std::string>::const_iterator i = ngrams.begin();
	 i != ngrams.end(); ++i) {
	ngram_obj.append(*i);
    }
    value["max_ngrams"] = max_ngrams;
}

void
SortedNGramProfile::from_json(const Json::Value & value)
{
    json_check_object(value, "ngram profile");
    
    const Json::Value & ngram_obj = value["ngrams"];
    json_check_array(ngram_obj, "ngram list in ngram profile");

    ngrams.clear();
    for (Json::Value::const_iterator i = ngram_obj.begin();
	 i != ngram_obj.end(); ++i) {
	json_check_string(*i, "ngram in ngram list in ngram profile");
	ngrams.push_back((*i).asString());
    }
    max_ngrams = json_get_uint64_member(value, "max_ngrams", UINT_MAX);
}

void
NGramProfileBuilder::add_ngrams(const std::string & term)
{
    Utf8Iterator pos(term);
    for (unsigned int offset = 0; pos != Utf8Iterator(); ++offset, ++pos) {
	Utf8Iterator subpos(pos);
	std::string ngram;
	for (unsigned int sublen = 0;
	     sublen < max_ngram_length && subpos != Utf8Iterator();
	     ++sublen, ++subpos) {
	    Unicode::append_utf8(ngram, *subpos);
	    ++(counts[ngram]);
	}
    }
}

void
NGramProfileBuilder::add_text(const char * start, size_t text_len)
{
    Utf8Iterator iter(start, text_len);

    while (true) {
	// Advance to the start of the next term.
	unsigned ch;
	while (true) {
	    if (iter == Utf8Iterator()) return;
	    ch = check_wordchar(*iter);
	    if (ch) break;
	    ++iter;
	}

	std::string term;
	while (true) {
	    unsigned prevch;
	    do {
		Unicode::append_utf8(term, ch);
		prevch = ch;
		if (++iter == Utf8Iterator()) goto endofterm;
		ch = check_wordchar(*iter);
	    } while (ch);

	    Utf8Iterator next(iter);
	    ++next;
	    if (next == Utf8Iterator()) break;
	    unsigned nextch = check_wordchar(*next);
	    if (!nextch) break;
	    unsigned infix_ch = *iter;
	    if (is_digit(prevch) && is_digit(*next)) {
		infix_ch = check_infix_digit(infix_ch);
	    } else {
		// Handle things like '&' in AT&T, apostrophes, etc.
		infix_ch = check_infix(infix_ch);
	    }
	    if (!infix_ch) break;
	    if (infix_ch != UNICODE_IGNORE)
		Unicode::append_utf8(term, infix_ch);
	    ch = nextch;
	    iter = next;
	}

	{
	    size_t len = term.size();
	    unsigned count = 0;
	    while ((ch = check_suffix(*iter))) {
		if (++count > 3) {
		    term.resize(len);
		    break;
		}
		Unicode::append_utf8(term, ch);
		if (++iter == Utf8Iterator()) goto endofterm;
	    }
	    // Don't index fish+chips as fish+ chips.
	    if (Unicode::is_wordchar(*iter))
		term.resize(len);
	}

endofterm:
	add_ngrams("|" + term + "|");
    }
}

void
NGramProfileBuilder::build(NGramProfile & profile,
			   unsigned int max_ngrams) const
{
    SortedNGramProfile sorted_profile;
    build(sorted_profile, max_ngrams);
    profile.init_from_sorted_ngram(sorted_profile);
}

struct NGramFreq {
    std::string ngram;
    unsigned int freq;

    NGramFreq(const std::string & ngram_,
	      unsigned int freq_)
	    : ngram(ngram_), freq(freq_)
    {}

    NGramFreq(std::map<std::string, unsigned int>::const_iterator & iter)
	    : ngram(iter->first), freq(iter->second)
    {}

    /** Comparison operator.
     *
     *  Sorts items with the highest frequency lowest, so that standard heap
     *  operations can be used to get the highest frequency items.
     */
    int operator< (const NGramFreq & other)
    {
	if (freq > other.freq) return true;
	if (freq < other.freq) return false;
	return ngram < other.ngram;
    }
};

void
NGramProfileBuilder::build(SortedNGramProfile & profile,
			   unsigned int max_ngrams) const
{
    profile.max_ngrams = max_ngrams;
    profile.ngrams.clear();

    std::vector<NGramFreq> items;
    std::map<std::string, unsigned int>::const_iterator iter(counts.begin());

    // Fill the items with ngrams, and make it a heap.
    while (items.size() < max_ngrams && iter != counts.end()) {
	items.push_back(iter);
	++iter;
    }
    if (items.empty()) return;

    make_heap(items.begin(), items.end());

    // Add items in, keeping it a heap of fixed size.
    unsigned int min_freq = items.front().freq;
    while (iter != counts.end()) {
	if (iter->second > min_freq) {
	    items.push_back(iter);
	    push_heap(items.begin(), items.end());
	    pop_heap(items.begin(), items.end());
	    items.pop_back();
	    min_freq = items.front().freq;
	}
	++iter;
    }

    // Sort the heap into order and then initialise the profile from the sorted
    // items.
    sort_heap(items.begin(), items.end());
    for (std::vector<NGramFreq>::const_iterator i = items.begin();
	 i != items.end(); ++i) {
	profile.ngrams.push_back(i->ngram);
    }
}
