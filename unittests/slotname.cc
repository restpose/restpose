/** @file slotname.cc
 * @brief Tests for slot name/number conversions
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
#include "jsonxapian/slotname.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

TEST(SlotName)
{
    {
	Json::Value tmp(Json::objectValue);
	SlotName slot;
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	SlotName slot(0u);
	CHECK_EQUAL(0u, slot.get());
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{\"out\":0}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	SlotName slot(1u);
	CHECK_EQUAL(1u, slot.get());
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{\"out\":1}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	SlotName slot(std::string("1"));
	CHECK_EQUAL(268435538u, slot.get()); // hashed value of "1"
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{\"out\":\"1\"}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	SlotName slot(std::string(""));
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	SlotName slot(std::string("hello world"));
	CHECK_EQUAL(2061196861u, slot.get()); // hashed value of "hello world"
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{\"out\":\"hello world\"}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	json_unserialise("1", tmp);
	SlotName slot(tmp);
	CHECK_EQUAL(1u, slot.get());
	tmp = Json::objectValue;
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{\"out\":1}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	json_unserialise("null", tmp);
	SlotName slot(tmp);
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	tmp = Json::objectValue;
	slot.to_json(tmp, "out");
	CHECK_EQUAL("{}", json_serialise(tmp));
    }

    {
	Json::Value tmp(Json::objectValue);
	json_unserialise("-1", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);

	json_unserialise("9999999999", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);

	json_unserialise("{}", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);
    }
}
