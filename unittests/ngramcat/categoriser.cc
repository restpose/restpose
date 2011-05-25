/** @file categoriser.cc
 * @brief Tests the ngram categoriser
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
#include "ngramcat/categoriser.h"
#include "utils/jsonutils.h"

using namespace RestPose;

TEST(NGramCategoriser)
{
    Json::Value tmp;
    Categoriser cat(1.05, 4, 10, 2);

    cat.add_target_profile("english", "hello welcome");
    cat.add_target_profile("russian", "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xd0\x94\xd0\xbe\xd0\xb1\xd1\x80\xd0\xbe");

    cat.to_json(tmp);
    CHECK_EQUAL("{\"accuracy_threshold\":1.050,"
		 "\"max_candidates\":2,"
		 "\"max_ngram_length\":4,"
		 "\"max_ngrams\":10,"
		 "\"profiles\":{"
		  "\"english\":{\"max_ngrams\":10,\"ngrams\":[\"|\",\"e\",\"l\",\"el\",\"o\",\"c\",\"co\",\"com\",\"come\",\"elc\"]},"
		  "\"russian\":{\"max_ngrams\":10,\"ngrams\":[\"|\",\"\xd0\xbe\",\"\xd1\x80\",\"|\xd0\xb4\",\"|\xd0\xb4\xd0\xbe\",\"|\xd0\xb4\xd0\xbe\xd0\xb1\",\"|\xd0\xbf\",\"|\xd0\xbf\xd1\x80\",\"|\xd0\xbf\xd1\x80\xd0\xb8\",\"\xd0\xb1\"]}"
		 "},"
		 "\"type\":\"ngram_rank\"}", json_serialise(tmp));

    Categoriser cat2;
    cat2.from_json(tmp);
    Json::Value tmp2;
    cat2.to_json(tmp2);
    CHECK_EQUAL(json_serialise(tmp2), json_serialise(tmp));

    std::vector<std::string> cats;

    // "hello" is obviously english.
    cat.categorise("hello", cats);
    CHECK_EQUAL(1u, cats.size());
    CHECK_EQUAL("english", cats[0]);

    // This is obviously russian.
    cat.categorise("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82", cats);
    CHECK_EQUAL(1u, cats.size());
    CHECK_EQUAL("russian", cats[0]);

    // Add some more targets to test an ambiguous categorisation.
    cat.add_target_profile("english2", "hello welcome 2");
    cat.add_target_profile("english3", "hello welcome 3");
    cat.categorise("hello", cats);
    CHECK_EQUAL(0u, cats.size());
}
