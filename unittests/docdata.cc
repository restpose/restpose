/** @file docdata.cc
 * @brief Tests for DocumentData
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

#include "UnitTest++.h"
#include "jsonxapian/docdata.h"
#include "utils/rsperrors.h"

using namespace RestPose;

TEST(DocumentDataGetSet)
{
    DocumentData docdata;
    docdata.set("foo", "bar");
    CHECK_EQUAL(docdata.get("foo"), "bar");
    CHECK_EQUAL(docdata.get("missing"), "");
    docdata.set("foo", "");
    CHECK_EQUAL(docdata.get("foo"), "");
}

TEST(DocumentDataSerialise)
{
    DocumentData docdata;
    docdata.set("foo", "bar");
    std::string s = docdata.serialise();
    docdata.set("foo", "");
    CHECK_EQUAL(docdata.get("foo"), "");
    docdata.unserialise(s);
    CHECK_EQUAL(docdata.get("foo"), "bar");
    docdata.unserialise("");
    CHECK_EQUAL(docdata.get("foo"), "");
    CHECK_THROW(docdata.unserialise(s.substr(s.size() - 1)), UnserialisationError);
    docdata.set("foo", "bar");
    docdata.set("food", "bard");
    CHECK_EQUAL(docdata.get("foo"), "bar");
    CHECK_EQUAL(docdata.get("food"), "bard");
    DocumentData docdata2;
    CHECK_EQUAL(docdata2.get("foo"), "");
    CHECK_EQUAL(docdata2.get("food"), "");
    docdata2.unserialise(docdata.serialise());
    CHECK_EQUAL(docdata2.get("foo"), "bar");
    CHECK_EQUAL(docdata2.get("food"), "bard");

    DocumentData::const_iterator i = docdata2.begin();
    CHECK(i != docdata2.end());
    CHECK_EQUAL(i->first, "foo");
    CHECK_EQUAL(i->second, "bar");
    ++i;
    CHECK_EQUAL(i->first, "food");
    CHECK_EQUAL(i->second, "bard");
    ++i;
    CHECK(i == docdata2.end());
}
