/** @file profile.h
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

#ifndef RESTPOSE_INCLUDED_PROFILE_H
#define RESTPOSE_INCLUDED_PROFILE_H

#include "json/value.h"
#include <string>
#include <map>
#include <vector>
#include <xapian/unicode.h>

namespace RestPose {

    struct SortedNGramProfile;

    /** An ngram profile, for a piece of text.
     *
     *  This is the format used for a stored profile that each input is to
     *  be tested against.
     */
    struct NGramProfile {
	/** Maximum number of ngrams allowed for the profile.
	 */
	unsigned int max_ngrams;

	/** Map from ngram to the position in the list of ngrams in sorted
	 *  frequency order.
	 */
	std::map<std::string, unsigned int> positions;

	/** Initialise this profile from a sorted ngram profile.
	 */
	void init_from_sorted_ngram(const SortedNGramProfile & other);

	/** Write this profile to a JSON value.
	 */
	void to_json(Json::Value & value) const;

	/** Initialise this profile from a JSON value.
	 */
	void from_json(const Json::Value & value);
    };

    /** An ngram profile, for a piece of text, in sorted order of frequency.
     *
     *  This is the format used for a profile generated from text to be tested.
     */
    struct SortedNGramProfile {
	/** Maximum number of ngrams allowed for the profile. */
	unsigned int max_ngrams;

	/** Ordered list of ngrams, in descending frequency order. */
	std::vector<std::string> ngrams;

	/** Get the distance from this profile to another. */
	unsigned int distance(const NGramProfile & other) const;

	/** Initialise this profile from an ngram profile.
	 */
	void init_from_ngram(const NGramProfile & other);

	/** Write this profile to a JSON value.
	 */
	void to_json(Json::Value & value) const;

	/** Initialise this profile from a JSON value.
	 */
	void from_json(const Json::Value & value);
    };

    /** Build a profile from pieces of text.
     */
    class NGramProfileBuilder {

	unsigned int max_ngram_length;

	std::map<std::string, unsigned int> counts;

	void add_ngrams(const std::string & term);

      public:
	NGramProfileBuilder(unsigned int max_ngram_length_)
		: max_ngram_length(max_ngram_length_)
	{}

	/** Clear any text previously added to the profile.
	 */
	void clear() {
	    counts.clear();
	}

	/** Add a piece of text to the profile.
	 */
	void add_text(const char * start, size_t len);

	/** Add a piece of text to the profile.
	 */
	void add_text(const std::string & input) {
	    add_text(input.data(), input.size());
	}

	/** Build an NGramProfile from the ngrams generated from text
	 *  previously added to this builder.
	 */
	void build(NGramProfile & profile,
		   unsigned int max_ngrams) const;

	/** Build a SortedNGramProfile from the ngrams generated from text
	 *  previously added to this builder.
	 */
	void build(SortedNGramProfile & profile,
		   unsigned int max_ngrams) const;

    };

};

#endif /* RESTPOSE_INCLUDED_PROFILE_H */
