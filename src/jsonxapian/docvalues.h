/** @file docvalues.h
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

#ifndef RESTPOSE_INCLUDED_DOCVALUES_H
#define RESTPOSE_INCLUDED_DOCVALUES_H

#include <string>
#include <set>
#include <xapian.h>

namespace RestPose {
    /** A set of values stored in a document slot.
     */
    class DocumentValue {
	std::set<std::string> values;

      public:
	typedef std::set<std::string>::const_iterator const_iterator;
	const_iterator begin() const {
	    return values.begin();
	}
	const_iterator end() const {
	    return values.end();
	}

	bool empty() const {
	    return values.empty();
	}

	/// Add a value.
	void add(const std::string & value) {
	    values.insert(value);
	}

	/// Remove a value.
	void remove(const std::string & value) {
	    values.erase(value);
	}

	/** Convert the values to a string, for storage. */
	std::string serialise() const;

	/** Unserialise the values from a string produced by serialise(). */
	void unserialise(const std::string &s);
    };

    class DocumentValues {
        std::map<Xapian::valueno, DocumentValue> entries;
      public:
	typedef std::map<Xapian::valueno, DocumentValue>::const_iterator
		const_iterator;
	typedef std::map<Xapian::valueno, DocumentValue>::iterator iterator;

	const_iterator begin() const {
	    return entries.begin();
	}
	const_iterator end() const {
	    return entries.end();
	}

	void add(Xapian::valueno slot, const std::string & value);
	void remove(Xapian::valueno slot, const std::string & value);

	bool empty(Xapian::valueno slot) const {
	    const_iterator i = entries.find(slot);
	    if (i == entries.end()) {
		return true;
	    }
	    return i->second.empty();
	}

	void apply(Xapian::Document & doc) const;
    };


    /// Encodings for values.
    enum ValueEncoding {
	/// Slot contains a single value.
	ENC_SINGLY_VALUED,

	/// Values preceded by their length, as a vint.
	ENC_VINT_LENGTHS,

	/// Values encoded using the geoencode scheme (6 bytes per value).
	ENC_GEOENCODE
    };


    /** A decoder for reading values from a slot in a document. */
    class SlotDecoder {
	/** The slot being decoded. */
	Xapian::valueno slot;

      protected:
	/** The current value being decoded.
	 */
	std::string value;

	/** Read the value from a new document.
	 */
	void read_value(const Xapian::Document & doc);

      public:
	/** Build a new SlotDecoder.
	 *
	 *  @param slot The slot to read from.
	 */
	SlotDecoder(Xapian::valueno slot_)
		: slot(slot_)
	{}

	/** Create a slot decoder for a given slot and encoding.
	 */
	static SlotDecoder * create(Xapian::valueno slot,
				    ValueEncoding encoding);

	virtual ~SlotDecoder();

	/** Start decoding the value from a new document.
	 */
	virtual void newdoc(const Xapian::Document & doc) = 0;

	/** Return the next value in the slot (by reference).
	 *
	 *  Returns true if a value was found, false otherwise.
	 */
	virtual bool next(const char ** begin, size_t * len) = 0;
    };


    /** A decoder for slots which hold a single value.
     */
    class SinglyValuedSlotDecoder : public SlotDecoder {
	bool read;
	public:
	SinglyValuedSlotDecoder(Xapian::valueno slot_)
		: SlotDecoder(slot_), read(true)
	{}

	void newdoc(const Xapian::Document & doc);

	bool next(const char ** begin_ptr, size_t * len_ptr);
    };


    /** A decoder for slots holding multiple values prefixed by their length.
     *
     *  The lengths are stored in standard Xapian vint form.
     */
    class VintLengthSlotDecoder : public SlotDecoder {
	const char * pos;
	const char * endpos;
	public:
	VintLengthSlotDecoder(Xapian::valueno slot_)
		: SlotDecoder(slot_), pos(NULL), endpos(NULL)
	{}

	void newdoc(const Xapian::Document & doc);

	bool next(const char ** begin_ptr, size_t * len_ptr);
    };


    /** A decoder for slots holding multiple geoencoded values.
     */
    class GeoEncodeSlotDecoder : public SlotDecoder {
	const char * pos;
	const char * endpos;
	public:
	GeoEncodeSlotDecoder(Xapian::valueno slot_)
		: SlotDecoder(slot_), pos(NULL), endpos(NULL)
	{}

	void newdoc(const Xapian::Document & doc);

	bool next(const char ** begin_ptr, size_t * len_ptr);
    };

};

#endif /* RESTPOSE_INCLUDED_DOCVALUES_H */
