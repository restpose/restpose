#include <cstdio>
/** @file categoriser.cc
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

#include <config.h>
#include "ngramcat/categoriser.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <algorithm>
#include <limits.h>

using namespace RestPose;

static const double def_accuracy_threshold = 1.03;
static const unsigned int def_max_ngram_length = 5u;
static const unsigned int def_max_ngrams = 400u;
static const unsigned int def_max_candidates = 3u;

Categoriser::Categoriser()
	: accuracy_threshold(def_accuracy_threshold),
	  max_ngram_length(def_max_ngram_length),
	  max_ngrams(def_max_ngrams),
	  max_candidates(def_max_candidates)
{}

Categoriser::Categoriser(double accuracy_threshold_,
			 unsigned int max_ngram_length_,
			 unsigned int max_ngrams_,
			 unsigned int max_candidates_)
	: accuracy_threshold(accuracy_threshold_),
	  max_ngram_length(max_ngram_length_),
	  max_ngrams(max_ngrams_),
	  max_candidates(max_candidates_)
{}

void
Categoriser::add_target_profile(const std::string & label,
				const NGramProfile & profile)
{
    if (labels.find(label) != labels.end()) {
	throw InvalidValueError("Can't add target profile to categoriser; "
				"already have a target with same label");
    }
    profiles.push_back(make_pair(label, profile));
    labels.insert(label);
}

void
Categoriser::add_target_profile(const std::string & label,
				const std::string & profile_text)
{
    if (labels.find(label) != labels.end()) {
	throw InvalidValueError("Can't add target profile to categoriser; "
				"already have a target with same label");
    }

    NGramProfileBuilder builder(max_ngram_length);
    builder.add_text(profile_text);
    profiles.push_back(std::make_pair(label, NGramProfile()));
    builder.build(profiles.back().second, max_ngrams);
}

void
Categoriser::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    Json::Value & profiles_obj = value["profiles"] = Json::objectValue;
    for (std::vector<std::pair<std::string, NGramProfile> >::const_iterator
	 i = profiles.begin(); i != profiles.end(); ++i) {
	Json::Value & profile_obj = profiles_obj[i->first];
	i->second.to_json(profile_obj);
    }

    value["accuracy_threshold"] = accuracy_threshold;
    value["max_ngram_length"] = max_ngram_length;
    value["max_ngrams"] = max_ngrams;
    value["max_candidates"] = max_candidates;
    value["type"] = "ngram_rank"; // Hardcoded - the only categoriser type we support currently.
}

void
Categoriser::from_json(const Json::Value & value)
{
    json_check_object(value, "categoriser");
    if (value["type"] != "ngram_rank") {
	throw InvalidValueError("Unknown categoriser type");
    }

    accuracy_threshold = json_get_double_member(value, "accuracy_threshold",
						def_accuracy_threshold);


    // Set maximum ngram length permissible to 100, which would be extremely
    // large - no point in allowing any larger.
    max_ngram_length = json_get_uint64_member(value, "max_ngram_length",
					      100u,
					      def_max_ngram_length);

    // Set maximum number of ngrams to use to 65536.  It's vaguely conceivable
    // that more might be wanted, but that would almost certainly become
    // inefficient with this implementation, so might as well set this limit
    // for now.
    max_ngrams = json_get_uint64_member(value, "max_ngrams",
					65536u,
					def_max_ngrams);

    max_candidates = json_get_uint64_member(value, "max_candidates",
					    UINT_MAX,
					    def_max_candidates);

    const Json::Value & profiles_obj = value["profiles"];
    if (profiles_obj.isNull()) {
	throw InvalidValueError("Missing profiles property in categoriser");
    }
    for (Json::Value::const_iterator i = profiles_obj.begin();
	 i != profiles_obj.end(); ++i) {
	profiles.push_back(std::make_pair(i.memberName(), NGramProfile()));
	profiles.back().second.from_json(*i);
    }
}

void
Categoriser::categorise(const SortedNGramProfile & profile,
			std::vector<std::string> & results) const
{
    results.clear();
    std::vector<std::pair<unsigned int, std::string> > scores;
    for (std::vector<std::pair<std::string, NGramProfile> >::const_iterator
	 i = profiles.begin(); i != profiles.end(); ++i) {
	scores.push_back(make_pair(profile.distance(i->second), i->first));
    }
    if (scores.empty()) return;
    std::sort(scores.begin(), scores.end());

    // Pick scores within threshold of top, and clear if that leaves too many.
    double max_allowed = double(scores.front().first) * accuracy_threshold;
    unsigned int max_results = max_candidates;
    if (scores.size() > max_results) {
	// Check for more than max_results having a score within threshold.
	if (scores[max_results].first <= max_allowed) {
	    // Too many close scores - declare this ambiguous.
	    return;
	}
    } else {
	max_results = scores.size();
    }

    results.push_back(scores[0].second);
    for (unsigned int j = 1; j < max_results; ++j) {
	if (scores[j].first > max_allowed)
	    break;
	results.push_back(scores[j].second);
    }
}

void
Categoriser::categorise(const std::string & text,
			std::vector<std::string> & results) const
{
    NGramProfileBuilder builder(max_ngram_length);
    builder.add_text(text);
    SortedNGramProfile profile;
    builder.build(profile, max_ngrams);
    return categorise(profile, results);
}
