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
};

#endif /* RESTPOSE_INCLUDED_DOCVALUES_H */
