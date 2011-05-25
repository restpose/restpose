/** @file profile.cc
 * @brief Tests for profiles for ngram categoriser
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
#include "UnitTest++.h"
#include "ngramcat/profile.h"
#include "utils/jsonutils.h"

using namespace RestPose;

TEST(NGramCatProfileBuilder)
{
    Json::Value tmp, tmp2;
    NGramProfileBuilder builder(5);
    std::string sample("hi");

    builder.add_text(sample.data(), sample.size());

    // Test building a very simple profile.
    SortedNGramProfile profile;
    builder.build(profile, 9);
    profile.to_json(tmp);
    CHECK_EQUAL("{\"max_ngrams\":9,\"ngrams\":["
		"\"|\",\"h\",\"hi\",\"hi|\",\"i\",\"i|\",\"|h\",\"|hi\",\"|hi|\""
		"]}", json_serialise(tmp));

    /* Check that profiles round-trip via json correctly. */
    SortedNGramProfile profile2;
    profile2.from_json(tmp);
    profile2.to_json(tmp2);
    CHECK_EQUAL("{\"max_ngrams\":9,\"ngrams\":["
		"\"|\",\"h\",\"hi\",\"hi|\",\"i\",\"i|\",\"|h\",\"|hi\",\"|hi|\""
		"]}", json_serialise(tmp2));
}

TEST(NGramCatProfileBuilder2)
{
    Json::Value tmp, tmp2;
    NGramProfileBuilder builder(2);
    std::string sample("abbbaa");

    builder.add_text(sample.data(), sample.size());

    // Test building a very simple profile.
    NGramProfile profile;
    SortedNGramProfile sorted_profile;
    builder.build(profile, 100);
    builder.build(sorted_profile, 100);
    profile.to_json(tmp);
    sorted_profile.to_json(tmp2);
    CHECK_EQUAL("{\"max_ngrams\":100,\"ngrams\":["
		"\"a\"," // frequency = 3
		"\"b\"," // 3
		"\"bb\"," // 2
		"\"|\"," // 2
		"\"aa\"," // 1
		"\"ab\"," // 1
		"\"a|\"," // 1
		"\"ba\"," // 1
		"\"|a\"" // 1
		"]}", json_serialise(tmp));
    CHECK_EQUAL(json_serialise(tmp2), json_serialise(tmp));

    // Test restricting the number of ngrams more.
    builder.build(profile, 3);
    builder.build(sorted_profile, 3);
    profile.to_json(tmp);
    sorted_profile.to_json(tmp2);
    CHECK_EQUAL("{\"max_ngrams\":3,\"ngrams\":["
		"\"a\"," // frequency = 3
		"\"b\"," // 3
		"\"bb\"" // 2
		"]}", json_serialise(tmp));
    CHECK_EQUAL(json_serialise(tmp2), json_serialise(tmp));
}

TEST(NGramProfileDistances)
{
    NGramProfile target1, target2, target3, target4;
    SortedNGramProfile sample1, sample2;

    NGramProfileBuilder builder(5);
    builder.add_text("Hello everyone");
    builder.build(target1, 10);

    builder.clear();
    builder.add_text("Goodbye");
    builder.build(target2, 10);

    builder.clear();
    builder.add_text("hello world");
    builder.build(target3, 10);

    builder.clear();
    builder.add_text("Hello world");
    builder.build(sample1, 10);

    // Check some measures which I've calculated the correct answers for by
    // hand.
    CHECK_EQUAL(36u, sample1.distance(target1));
    CHECK_EQUAL(76u, sample1.distance(target2));
    CHECK_EQUAL(0u, sample1.distance(target3));

    // Check that the same profiles give the same answer if compared the other way around.
    target4.init_from_sorted_ngram(sample1);
    sample2.init_from_ngram(target1);
    CHECK_EQUAL(36u, sample2.distance(target4));
    sample2.init_from_ngram(target2);
    CHECK_EQUAL(76u, sample2.distance(target4));
    sample2.init_from_ngram(target3);
    CHECK_EQUAL(0u, sample2.distance(target4));
}
