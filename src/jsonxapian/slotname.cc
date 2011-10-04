/** @file slotname.cc
 * @brief Hash a slot name to get a slot number
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
#include "jsonxapian/slotname.h"

#include "str.h"
#include "utils/rsperrors.h"

using namespace RestPose;
using namespace std;

SlotName::SlotName()
	: name(), num(Xapian::BAD_VALUENO)
{}

SlotName::SlotName(unsigned long int slotnum)
	: name(), num(slotnum)
{}

SlotName::SlotName(const string & slotname)
	: name(slotname), num(hash_slot(name))
{}

SlotName::SlotName(const Json::Value & value)
	: name()
{
    if (value.isNull()) {
	num = Xapian::BAD_VALUENO;
    } else if (value.isIntegral()) {
	if (value < Json::Value::Int(0)) {
	    throw InvalidValueError("Value for slot number was negative");
	}
	if (value > 0xffffffffu) {
	    throw InvalidValueError("Value for slot number was larger than "
				    "maximum allowed (" +
				    str(0xffffffffu) + ")");
	}
	num = value.asUInt();
    } else if (value.isString()) {
	name = value.asString();
	num = hash_slot(name);
    } else {
	throw InvalidValueError("Value for slot number was not an integer "
				"or a string.");
    }
}

Json::Value &
SlotName::to_json(Json::Value & value) const
{
    if (name.empty()) {
	if (num == Xapian::BAD_VALUENO) {
	    value = Json::nullValue;
	} else {
	    value = num;
	}
    } else {
	value = name;
    }
    return value;
}
