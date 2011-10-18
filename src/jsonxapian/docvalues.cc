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

#include <memory>
#include "omassert.h"
#include "serialise.h"
#include "utils/rsperrors.h"

using namespace RestPose;
using namespace std;

string
SinglyValuedDocumentValue::serialise() const
{
    if (values.empty()) {
	return string();
    }
    return *(values.begin());
}


string
VintLengthDocumentValue::serialise() const
{
    string result;
    for (const_iterator i = begin(); i != end(); ++i) {
	result += encode_length(i->size());
	result += *i;
    }
    return result;
}


string
GeoEncodeDocumentValue::serialise() const
{
    string result;
    for (const_iterator i = begin(); i != end(); ++i) {
	Assert(i->size() == 6);
	result += *i;
    }
    return result;
}


DocumentValues::~DocumentValues()
{
    for (iterator i = entries.begin(); i != entries.end(); ++i) {
	delete i->second;
    }
}

void
DocumentValues::set_slot_format(Xapian::valueno slot, ValueEncoding encoding)
{
    formats[slot] = encoding;
}

void
DocumentValues::add(Xapian::valueno slot, const std::string & value)
{
    DocumentValue * valptr;
    iterator i = entries.find(slot);
    if (i == entries.end()) {
	ValueEncoding encoding(ENC_VINT_LENGTHS);
	map<Xapian::valueno, ValueEncoding>::const_iterator format =
		formats.find(slot);
	if (format != formats.end()) {
	    encoding = format->second;
	}

	auto_ptr<DocumentValue> newvalue(NULL);
	switch (encoding) {
	    case ENC_VINT_LENGTHS:
		newvalue =
			auto_ptr<DocumentValue>(new VintLengthDocumentValue);
		break;
	    case ENC_SINGLY_VALUED:
		newvalue =
			auto_ptr<DocumentValue>(new SinglyValuedDocumentValue);
		break;
	    case ENC_GEOENCODE:
		newvalue =
			auto_ptr<DocumentValue>(new GeoEncodeDocumentValue);
		break;
	}

	pair<iterator, bool> ret;
	pair<Xapian::valueno, DocumentValue *> item(slot, NULL);
	ret = entries.insert(item);
	valptr = newvalue.get();
	ret.first->second = newvalue.release();
    } else {
	valptr = i->second;
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
    DocumentValue & val(*(i->second));
    val.remove(value);
    if (val.empty()) {
	entries.erase(i);
    }
}

void
DocumentValues::apply(Xapian::Document & doc) const
{
    for (const_iterator i = begin(); i != end(); ++i) {
	doc.add_value(i->first, i->second->serialise());
    }
}


void
SlotDecoder::read_value(const Xapian::Document & doc)
{
    value = doc.get_value(slot);
}

SlotDecoder *
SlotDecoder::create(Xapian::valueno slot,
		    ValueEncoding encoding)
{
    if (slot == Xapian::BAD_VALUENO) {
	return NULL;
    }
    switch (encoding) {
	case ENC_SINGLY_VALUED:
	    return new SinglyValuedSlotDecoder(slot);
	case ENC_VINT_LENGTHS:
	    return new VintLengthSlotDecoder(slot);
	case ENC_GEOENCODE:
	    return new GeoEncodeSlotDecoder(slot);
    }
    return NULL;
}

SlotDecoder::~SlotDecoder()
{
}


void
SinglyValuedSlotDecoder::newdoc(const Xapian::Document & doc)
{
    read_value(doc);
    read = false;
}

bool
SinglyValuedSlotDecoder::next(const char ** begin_ptr, size_t * len_ptr)
{
    if (read) {
	return false;
    }
    *begin_ptr = value.data();
    *len_ptr = value.size();
    read = true;
    return true;
}


void
VintLengthSlotDecoder::newdoc(const Xapian::Document & doc)
{
    read_value(doc);
    pos = value.data();
    endpos = pos + value.size();
}

bool
VintLengthSlotDecoder::next(const char ** begin_ptr, size_t * len_ptr)
{
    if (pos == endpos) {
	return false;
    }
    size_t len = decode_length(&pos, endpos, true);
    *begin_ptr = pos;
    *len_ptr = len;
    pos += len;
    return true;
}

void
GeoEncodeSlotDecoder::newdoc(const Xapian::Document & doc)
{
    read_value(doc);
    pos = value.data();
    endpos = pos + value.size();
}

bool
GeoEncodeSlotDecoder::next(const char ** begin_ptr, size_t * len_ptr)
{
    if (pos == endpos) {
	return false;
    }
    if (endpos - pos < 6) {
	throw RestPose::UnserialisationError("Bad geoencoded value: invalid encoded length");
    }
    *begin_ptr = pos;
    *len_ptr = 6;
    pos += 6;
    return true;
}
