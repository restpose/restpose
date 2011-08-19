/** @file docvalues.cc
 * @brief Abstraction for modifying document values.
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
#include "jsonxapian/docvalues.h"

#include "serialise.h"

using namespace RestPose;
using namespace std;

string
DocumentValue::serialise() const
{
    string result;
    for (const_iterator i = begin(); i != end(); ++i) {
	result += encode_length(i->size());
	result += *i;
    }
    return result;
}

void
DocumentValue::unserialise(const string & s)
{
    values.clear();
    const char * pos = s.data();
    const char * endpos = pos + s.size();
    while (pos != endpos) {
	size_t len = decode_length(&pos, endpos, true);
	if (values.empty()) {
	    values.insert(string(pos, len));
	} else {
	    set<string>::iterator insertpos = values.end();
	    --insertpos;
	    values.insert(insertpos, string(pos, len));
	}
	pos += len;
    }
}


void
DocumentValues::add(Xapian::valueno slot, const std::string & value)
{
    DocumentValue * valptr;
    iterator i = entries.find(slot);
    if (i == entries.end()) {
	valptr = &(entries[slot]);
    } else {
	valptr = &(i->second);
    }
    valptr->add(value);
}

void
DocumentValues::remove(Xapian::valueno slot, const std::string & value)
{
    iterator i = entries.find(slot);
    if (i == entries.end()) {
	return;
    }
    DocumentValue & val(i->second);
    val.remove(value);
    if (val.empty()) {
	entries.erase(i);
    }
}

void
DocumentValues::apply(Xapian::Document & doc) const
{
    for (const_iterator i = begin(); i != end(); ++i) {
	doc.add_value(i->first, i->second.serialise());
    }
}
