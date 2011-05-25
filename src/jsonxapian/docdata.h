/** @file docdata.h
 * @brief Abstraction for storing document data in fields.
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

#ifndef RESTPOSE_INCLUDED_DOCDATA_H
#define RESTPOSE_INCLUDED_DOCDATA_H

#include <string>
#include <map>

namespace RestPose {
    /** Data to be stored in a document.
     *
     *  This is an abstraction on top of Xapian's Document data storage, which
     *  provides separated storage for each field.
     */
    class DocumentData {
	std::map<std::string, std::string> fields;

      public:
	typedef std::map<std::string, std::string>::const_iterator const_iterator;
	const_iterator begin() const {
	    return fields.begin();
	}
	const_iterator end() const {
	    return fields.end();
	}

	/** Set the value associated with a given field.
	 */
	void set(const std::string & field, const std::string & value) {
	    if (value.empty()) {
		fields.erase(field);
	    } else {
		fields[field] = value;
	    }
	}

	/** Get the value associated with a given field.
	 *
	 *  Returns the empty string if no value is associated with the field.
	 */
	std::string get(const std::string & field) const {
	    std::map<std::string, std::string>::const_iterator i;
	    i = fields.find(field);
	    if (i == fields.end()) {
		return std::string();
	    } else {
		return i->second;
	    }
	}

	/** Convert the document data to a string, for storage. */
	std::string serialise() const;

	/** Unserialise the document data from a string produced by
	 *  serialise(). */
	void unserialise(const std::string &s);
    };
};

#endif /* RESTPOSE_INCLUDED_DOCDATA_H */
