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

    /// Encodings for values.
    enum ValueEncoding {
	/// Values preceded by their length, as a vint.  (Default encoding.)
	ENC_VINT_LENGTHS,

	/// Slot contains a single value.
	ENC_SINGLY_VALUED,

	/// Values encoded using the geoencode scheme (6 bytes per value).
	ENC_GEOENCODE
    };


    /** A set of values stored in a document slot.
     */
    class DocumentValue {
      protected:
	std::set<std::string> values;

      public:
	typedef std::set<std::string>::const_iterator const_iterator;
	const_iterator begin() const {
	    return values.begin();
	}
	const_iterator end() const {
	    return values.end();
	}

	/// Return true iff no values are stored in the slot.
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
	virtual std::string serialise() const = 0;
    };


    /** A document value holder for slots which hold a single value.
     */
    class SinglyValuedDocumentValue : public DocumentValue {
      public:
	std::string serialise() const;
    };


    /** A document value holder for slots which hold values prefixed by length.
     *
     *  The lengths are stored in standard Xapian vint form.
     */
    class VintLengthDocumentValue : public DocumentValue {
      public:
	std::string serialise() const;
    };


    /** A document value holder for slots holding multiple geoencoded values.
     */
    class GeoEncodeDocumentValue : public DocumentValue {
      public:
	std::string serialise() const;
    };


    class DocumentValues {
        std::map<Xapian::valueno, DocumentValue *> entries;
	std::map<Xapian::valueno, ValueEncoding> formats;
      public:
	typedef std::map<Xapian::valueno, DocumentValue *>::const_iterator
		const_iterator;
	typedef std::map<Xapian::valueno, DocumentValue *>::iterator iterator;

	~DocumentValues();

	const_iterator begin() const {
	    return entries.begin();
	}
	const_iterator end() const {
	    return entries.end();
	}

	/** Set the encoding used to store values in a slot.
	 */
	void set_slot_format(Xapian::valueno slot, ValueEncoding encoding);

	/** Add a value to a slot.
	 */
	void add(Xapian::valueno slot, const std::string & value);

	/** Remove a value from a slot.
	 */
	void remove(Xapian::valueno slot, const std::string & value);

	/** Check if a value slot is empty.
	 */
	bool empty(Xapian::valueno slot) const {
	    const_iterator i = entries.find(slot);
	    if (i == entries.end()) {
		return true;
	    }
	    return i->second->empty();
	}

	/** Apply the values to a document.
	 */
	void apply(Xapian::Document & doc) const;
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
