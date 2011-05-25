/** @file walker.cc
 * @brief Tests for JSONWalker
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

#include "jsonmanip/jsonpath.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <json/json.h>
#include "str.h"

#include <string>
#include <vector>

using namespace RestPose;

static std::string
test_walk(const Json::Value & value)
{
    JSONWalker walker(value);
    JSONWalker::Event event;
    std::string result;
    while (walker.next(event)) {
	switch (event.type) {
	    case JSONWalker::Event::START:
		result += "S";
		break;
	    case JSONWalker::Event::LEAF:
		result += "I";
		break;
	    case JSONWalker::Event::END:
		result += "E";
		break;
	    default:
		result += "!";
	}

	switch (event.component.type) {
	    case JSONPathComponent::JSONPATH_KEY:
		result += '\'';
		result += event.component.key;
		result += '\'';
		break;
	    case JSONPathComponent::JSONPATH_INDEX:
		result += str(event.component.index);
		break;
	    default:
		result += "!";
	}
	result += json_serialise(*(event.value));
	result += '\n';
    }
    return result;
}

/// Test null conditional expressions.
TEST(BasicWalker)
{
    Json::Value doc;

    json_unserialise("{\"name\": \"fred\"}", doc);
    CHECK_EQUAL("S0{\"name\":\"fred\"}\n"
		"I'name'\"fred\"\n"
		"E0{\"name\":\"fred\"}\n",
		test_walk(doc));

    json_unserialise("[\"fred\"]", doc);
    CHECK_EQUAL("S0[\"fred\"]\n"
		"I0\"fred\"\n"
		"E0[\"fred\"]\n",
		test_walk(doc));

    json_unserialise("[\"fred\", [2, 3]]", doc);
    CHECK_EQUAL("S0[\"fred\",[2,3]]\n"
		"I0\"fred\"\n"
		"S1[2,3]\n"
		"I02\n"
		"I13\n"
		"E1[2,3]\n"
		"E0[\"fred\",[2,3]]\n",
		test_walk(doc));

    json_unserialise("[\"fred\", [[1, 2], 3]]", doc);
    CHECK_EQUAL("S0[\"fred\",[[1,2],3]]\n"
		"I0\"fred\"\n"
		"S1[[1,2],3]\n"
		"S0[1,2]\n"
		"I01\n"
		"I12\n"
		"E0[1,2]\n"
		"I13\n"
		"E1[[1,2],3]\n"
		"E0[\"fred\",[[1,2],3]]\n",
		test_walk(doc));

    json_unserialise("{\"a\": {\"b\": \"c\", \"d\": {\"e\": []}}}", doc);
    CHECK_EQUAL("S0{\"a\":{\"b\":\"c\",\"d\":{\"e\":[]}}}\n"
		"S'a'{\"b\":\"c\",\"d\":{\"e\":[]}}\n"
		"I'b'\"c\"\n"
		"S'd'{\"e\":[]}\n"
		"S'e'[]\n"
		"E'e'[]\n"
		"E'd'{\"e\":[]}\n"
		"E'a'{\"b\":\"c\",\"d\":{\"e\":[]}}\n"
		"E0{\"a\":{\"b\":\"c\",\"d\":{\"e\":[]}}}\n",
		test_walk(doc));
}
