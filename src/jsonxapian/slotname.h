/** @file slotname.h
 * @brief Hash a slot name to get a slot number
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

#ifndef RESTPOSE_INCLUDED_SLOTNAME_H
#define RESTPOSE_INCLUDED_SLOTNAME_H

#include "json/value.h"
#include <string>
#include <xapian.h>

namespace RestPose {

/** A named slot.  */
class SlotName {
    std::string name;
    Xapian::valueno num;

  public:
    static const unsigned long int max_explicit_slot_num = 0x0fffffffu;

    static Xapian::valueno hash_slot(const char * ptr, size_t len)
    {
	if (len == 0) {
	    return Xapian::BAD_VALUENO;
	}
	const char * end = ptr + len;
	unsigned long int h = 1;
	for (; ptr != end; ++ptr) {
	    h += (h << 5) + static_cast<unsigned char>(*ptr);
	}
	h = max_explicit_slot_num + 1 + (h & 0xefffffffu);
	return h;
    }

    static Xapian::valueno hash_slot(const std::string & slotname)
    {
	return hash_slot(slotname.data(), slotname.size());
    }

    SlotName();
    SlotName(unsigned long int slotnum);
    SlotName(const std::string & slotname);
    SlotName(const Json::Value & value);

    void to_json(Json::Value & value, const char * slotname) const;

    Xapian::valueno get() const { return num; }
};

}

#endif /* RESTPOSE_INCLUDED_SLOTNAME_H */
