/** @file categoriser.h
 * @brief Ngram profiles categorisation.
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

#ifndef RESTPOSE_INCLUDED_CATEGORISER_H
#define RESTPOSE_INCLUDED_CATEGORISER_H

#include "json/value.h"
#include "ngramcat/profile.h"

#include <set>
#include <string>
#include <vector>

namespace RestPose {

    /** A categoriser, using a set of ngram profiles.
     */
    class Categoriser {
	/** The profiles to test.
	 */
	std::vector<std::pair<std::string, NGramProfile> > profiles;

	/** The profile labels.
	 */
	std::set<std::string> labels;

	/** The proportion of the best score that other profiles must be within
	 *  to be matched.
	 */
	double accuracy_threshold;

	/** Maximum number of characters to use in ngrams.
	 */
	unsigned int max_ngram_length;

	/** Maximum number of ngrams to use when categorising.
	 */
	unsigned int max_ngrams;

	/** Maximum number of candidates to allow when categorising.
	 *
	 *  If more candidates than this are within the threshold, no match is
	 *  reported (on the assumption that this means the categorisation is
	 *  too ambiguous).
	 */
	unsigned int max_candidates;

      public:
	Categoriser();

	Categoriser(double accuracy_threshold_,
		    unsigned int max_ngram_length_,
		    unsigned int max_ngrams_,
		    unsigned int max_candidates_);

	/** Add a target profile to the categoriser.
	 *
	 *  @param label The label to return when the profile matches.
	 *  @param profile The profile to add.
	 *
	 *  Throws an InvalidValueError if a profile has already been added
	 *  with the target profile.
	 */
	void add_target_profile(const std::string & label,
				const NGramProfile & profile);

	/** Add a target profile on the basis of some sample text.
	 */
	void add_target_profile(const std::string & label,
				const std::string & sample_text);

	/// Serialise the Categoriser to a JSON object.
	void to_json(Json::Value & value) const;

	/// Unserialise the Categoriser from a JSON object.
	void from_json(const Json::Value & value);

	/** Categorise a sorted profile.
	 *
	 *  Returns the most likely matches in `results`, in decreasing order
	 *  of quality of match.  If the profile is ambiguous, `results` will
	 *  be empty.
	 */
	void categorise(const SortedNGramProfile & profile,
			std::vector<std::string> & results) const;

	/** Categorise a piece of text.
	 *
	 *  Returns the most likely matches in `results`, in decreasing order
	 *  of quality of match.  If the profile is ambiguous, `results` will
	 *  be empty.
	 */
	void categorise(const std::string & text,
			std::vector<std::string> & results) const;
    };

};

#endif /* RESTPOSE_INCLUDED_CATEGORISER_H */
