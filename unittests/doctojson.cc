/** @file doctojson.cc
 * @brief Tests for converting documents to JSON.
 */
/* Copyright (c) 2010 Richard Boulton
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
#include "jsonxapian/doctojson.h"

#include "utils/jsonutils.h"

using namespace RestPose;

TEST(DocToJson)
{
    Xapian::Document doc;
    Json::Value tmp;
    CHECK_EQUAL("{}",
		json_serialise(doc_to_json(doc, tmp)));

    doc.add_value(10, "ten");
    CHECK_EQUAL("{\"values\":{\"10\":\"ten\"}}",
		json_serialise(doc_to_json(doc, tmp)));

    doc.clear_values();
    doc.add_boolean_term("foo");
    CHECK_EQUAL("{\"terms\":{\"foo\":{}}}",
		json_serialise(doc_to_json(doc, tmp)));
    doc.add_term("foo", 5);
    CHECK_EQUAL("{\"terms\":{\"foo\":{\"wdf\":5}}}",
		json_serialise(doc_to_json(doc, tmp)));
    doc.add_posting("foo", 1);
    doc.add_posting("foo", 7);
    CHECK_EQUAL("{\"terms\":{\"foo\":{\"positions\":[1,7],\"wdf\":7}}}",
		json_serialise(doc_to_json(doc, tmp)));
}
