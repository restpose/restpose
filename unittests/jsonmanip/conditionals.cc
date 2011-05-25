/** @file conditionals.cc
 * @brief Tests for JSON Conditional expressions
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

#include "jsonmanip/conditionals.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <json/json.h>

#include <string>
#include <vector>

using namespace RestPose;

/** Apply a conditional test to several docs, returning a string */
static std::string
test_docs(const std::vector<Json::Value> & docs, const Conditional & cond)
{
    std::vector<Json::Value>::const_iterator i;
    std::string result;
    for (i = docs.begin(); i != docs.end(); ++i) {
	if (cond.test(*i)) {
	    result += "T";
	} else {
	    result += "F";
	}
    }
    return result;
}

/// Test null conditional expressions.
TEST(NullConditionals)
{
    Conditional c;
    std::vector<Json::Value> docs;
    Json::Value tmp;

    json_unserialise("{\"name\": \"fred\"}", tmp);
    docs.push_back(tmp);
    
    // Check an uninitialised conditional.
    c.to_json(tmp);
    CHECK_EQUAL("null", json_serialise(tmp));
    CHECK_THROW(test_docs(docs, c), InvalidValueError);

    // Check setting a conditional to null.
    json_unserialise("null", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("null", json_serialise(tmp));
    CHECK_THROW(test_docs(docs, c), InvalidValueError);
}

/// Test constant conditional expressions.
TEST(ConstConditionals)
{
    Conditional c;
    std::vector<Json::Value> docs;
    Json::Value tmp;

    json_unserialise("{\"name\": \"fred\"}", tmp);
    docs.push_back(tmp);
    docs.push_back(Json::nullValue);
    
    // Check an uninitialised conditional.
    json_unserialise("{\"literal\": true}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"literal\":true}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "TT");

    // Check setting a conditional to false.
    json_unserialise("{\"literal\": false}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"literal\":false}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FF");
}

/// Test the "exists" conditional expression.
TEST(ExistsConditionals)
{
    Conditional c;
    Json::Value tmp;

    std::vector<Json::Value> docs;
    json_unserialise("[]", tmp);
    docs.push_back(tmp);
    json_unserialise("{}", tmp);
    docs.push_back(tmp);
    json_unserialise("[\"aunt\"]", tmp);
    docs.push_back(tmp);
    json_unserialise("[\"aunt\", \"uncle\"]", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"name\": \"fred\"}", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"names\": [\"fred\", \"bloggs\"]}", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"names\": {\"first\": \"fred\", \"second\": \"bloggs\"}}", tmp);
    docs.push_back(tmp);

    // Check an empty test
    json_unserialise("{\"exists\": []}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "TTTTTTT");

    // Check a test for an array with 1 or more items.
    json_unserialise("{\"exists\": [0]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[0]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFTTFFF");

    // Check a test for an array with 2 or more items.
    json_unserialise("{\"exists\": [1]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[1]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFTFFF");

    // Check a test for an object with a "name" member.
    json_unserialise("{\"exists\": [\"name\"]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[\"name\"]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFFTFF");

    // Check a test for an object with a "names" member holding a list with at
    // least two members.
    json_unserialise("{\"exists\": [\"names\", 1]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[\"names\",1]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFFFTF");

    // Check a test for an object with a "names" member holding an object with
    // a "first" member.
    json_unserialise("{\"exists\": [\"names\", \"first\"]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"exists\":[\"names\",\"first\"]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFFFFT");
}

/// Test the "equals" and "get" conditional expressions.
TEST(EqualsConditionals)
{
    Conditional c;
    Json::Value tmp;

    std::vector<Json::Value> docs;
    json_unserialise("[]", tmp);
    docs.push_back(tmp);
    json_unserialise("{}", tmp);
    docs.push_back(tmp);
    json_unserialise("[\"aunt\"]", tmp);
    docs.push_back(tmp);
    json_unserialise("[\"aunt\", \"uncle\"]", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"name\": \"fred\"}", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"names\": [\"fred\", \"bloggs\"]}", tmp);
    docs.push_back(tmp);
    json_unserialise("{\"names\": {\"first\": \"fred\", \"second\": \"bloggs\"}}", tmp);
    docs.push_back(tmp);


    // Check an empty test
    json_unserialise("{\"equals\": []}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"equals\":[]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "TTTTTTT");

    // Check an single literal
    json_unserialise("{\"equals\": [{\"literal\": false}]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"equals\":[{\"literal\":false}]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "TTTTTTT");

    // Check two equal constants
    json_unserialise("{\"equals\": [{\"literal\": false}, {\"literal\": false}]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"equals\":[{\"literal\":false},{\"literal\":false}]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "TTTTTTT");

    // Check two unequal constants
    json_unserialise("{\"equals\": [{\"literal\": false}, {\"literal\": true}]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"equals\":[{\"literal\":false},{\"literal\":true}]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFFFFF");

    // Check comparing a value
    json_unserialise("{\"equals\": [{\"get\": [\"name\"]}, {\"literal\": \"fred\"}]}", tmp);
    c.from_json(tmp);
    c.to_json(tmp);
    CHECK_EQUAL("{\"equals\":[{\"get\":[\"name\"]},{\"literal\":\"fred\"}]}", json_serialise(tmp));
    CHECK_EQUAL(test_docs(docs, c), "FFFFTFF");
}
