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
    Json::Value tmp;
    {
	SlotName slot;
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	CHECK_EQUAL("null", json_serialise(slot.to_json(tmp)));
    }

    {
	SlotName slot(0u);
	CHECK_EQUAL(0u, slot.get());
	CHECK_EQUAL("0", json_serialise(slot.to_json(tmp)));
    }

    {
	SlotName slot(1u);
	CHECK_EQUAL(1u, slot.get());
	CHECK_EQUAL("1", json_serialise(slot.to_json(tmp)));
    }

    {
	SlotName slot(std::string("1"));
	CHECK_EQUAL(268435538u, slot.get()); // hashed value of "1"
	CHECK_EQUAL("\"1\"", json_serialise(slot.to_json(tmp)));
    }

    {
	SlotName slot(std::string(""));
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	CHECK_EQUAL("null", json_serialise(slot.to_json(tmp)));
    }

    {
	SlotName slot(std::string("hello world"));
	CHECK_EQUAL(2061196861u, slot.get()); // hashed value of "hello world"
	CHECK_EQUAL("\"hello world\"", json_serialise(slot.to_json(tmp)));
    }

    {
	json_unserialise("1", tmp);
	SlotName slot(tmp);
	CHECK_EQUAL(1u, slot.get());
	CHECK_EQUAL("1", json_serialise(slot.to_json(tmp)));
    }

    {
	json_unserialise("null", tmp);
	SlotName slot(tmp);
	CHECK_EQUAL(Xapian::BAD_VALUENO, slot.get());
	CHECK_EQUAL("null", json_serialise(slot.to_json(tmp)));
    }

    {
	json_unserialise("-1", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);

	json_unserialise("9999999999", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);

	json_unserialise("{}", tmp);
	CHECK_THROW(SlotName test(tmp), InvalidValueError);
    }
}
