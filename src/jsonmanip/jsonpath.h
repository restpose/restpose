/** @file jsonpath.h
 * @brief Path handling and manipultion for JSON documents.
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

#ifndef RESTPOSE_INCLUDED_JSONPATH_H
#define RESTPOSE_INCLUDED_JSONPATH_H

#include "json/value.h"
#include <string>
#include <vector>

namespace RestPose {

    /** A component in a JSONPath.
     */
    struct JSONPathComponent {
	enum Type {
	    JSONPATH_KEY,
	    JSONPATH_INDEX
	};
	Type type;
	std::string key;
	unsigned int index;

	void set_string(const std::string & key_) {
	    key = key_;
	    type = JSONPATH_KEY;
	}

	void set_index(unsigned int index_) {
	    index = index_;
	    type = JSONPATH_INDEX;
	}

	/// Set from a JSON string or integer.
	void set(const Json::Value & value);

	/// Get as a JSON string or integer.
	Json::Value get() const {
	    if (type == JSONPATH_KEY) {
		return Json::Value(key);
	    } else {
		return Json::Value(index);
	    }
	}

	bool is_string() const {
	    return type == JSONPATH_KEY;
	}

	bool is_index() const {
	    return type == JSONPATH_INDEX;
	}

	bool operator==(const JSONPathComponent & other) const {
	    if (type != other.type) {
		return false;
	    }
	    if (type == JSONPATH_KEY) {
		return key == other.key;
	    } else {
		return index == other.index;
	    }
	}

	bool operator<(const JSONPathComponent & other) const {
	    if (type != other.type) {
		return type < other.type;
	    }
	    if (type == JSONPATH_KEY) {
		return key < other.key;
	    } else {
		return index < other.index;
	    }
	}
    };

    /** A path to a value in a json object / array.
     *
     *  This is just a list of keys / indices.
     */
    struct JSONPath {
	std::vector<JSONPathComponent> path;

	void append_string(const std::string & key) {
	    path.resize(path.size() + 1);
	    path.back().set_string(key);
	}

	void append_index(unsigned int index) {
	    path.resize(path.size() + 1);
	    path.back().set_index(index);
	}

	void to_json(Json::Value & value) const;
	void from_json(const Json::Value & value);

	/** Look up this path in a Json value.
	 *
	 *  Returns Json::nullValue if the path is not found.
	 */
	const Json::Value & find(const Json::Value & value) const;
    };

    /** A class which walks over a JSON object or array.
     *
     *  This returns a sequence of events.
     */
    class JSONWalker {
      public:
	struct Event {
	    enum {
		START,
		LEAF,
		END
	    } type;

	    /** The index of this component.
	     */
	    JSONPathComponent component;

	    /** A pointer to the value that this event happened on.
	     */
	    const Json::Value * value;
	};

      private:
	/// The value being walked.
	const Json::Value & value;

	struct Level {
	    const Json::Value * parent_value;
	    JSONPathComponent parent_component;

	    Json::Value::const_iterator pos;
	    Json::Value::const_iterator end;
	    JSONPathComponent::Type type;
	    bool started;

	    Level(const Json::Value & value);
	    const Json::Value & current() const;
	    void set_event(Event & event);
	    void set_event_parent(Event & event);
	    void next(Event & event);
	};

	std::vector<Level> stack;

	JSONPath current;

      public:
	/** Create the walker.
	 *
	 *  The walker is positioned such that the next call to next() will
	 *  return the first item in the walk.
	 */
	JSONWalker(const Json::Value & value_);

	/** Move the the next item in the walk.
	 *
	 *  Returns false if the walk has finished.  Otherwise, sets the
	 *  supplied event to reflect the next item in the walk.
	 */
	bool next(Event & event);
    };
};

#endif /* RESTPOSE_INCLUDED_JSONPATH_H */
