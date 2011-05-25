/** @file mapping.h
 * @brief Mappings applied to JSON documents.
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

#ifndef RESTPOSE_INCLUDED_MAPPING_H
#define RESTPOSE_INCLUDED_MAPPING_H

#include "json/value.h"
#include "jsonmanip/conditionals.h"
#include <string>
#include <vector>

namespace RestPose {

    struct MappingActions;

    class Collection;

    /** A map from a key or index to actions for the associated value
     *  and its children.
     */
    typedef std::map<JSONPathComponent, MappingActions> ActionMap;

    struct MappingTarget {
	/** The field to store the results in.
	 */
	std::string field;

	/** The categoriser to use.
	 */
	std::string categoriser;

	MappingTarget(const std::string & field_)
		: field(field_)
	{}
    };

    /** The mapping to apply at or below a particular element.
     */
    struct MappingActions {

	/** Mappings to apply to children.
	 */
	ActionMap children;

	/** The target fields to map this element to.
	 */
	std::vector<MappingTarget> target_fields;

	/** Find the actions for a given path.
	 *
	 *  Creates an empty action if no current actions for the path.
	 */
	MappingActions & find(const Json::Value & path,
			      size_t path_offset = 0);
    };

    /** A mapping, to be applied to a JSON document.
     */
    class Mapping {
	/// A conditional specifying when the mapping should be applied.
	Conditional when;

	/// Mappings from input paths to field names.
	MappingActions mappings;

	/// Default action, for fields not listed in the mappings.
	enum {
	    PRESERVE_TOP, // Preserve top-level unmapped fields
	    DISCARD // Discard unmapped fields
	} default_action;


	/** Perform custom handling for an event.
	 */
	bool handle(const Collection & collection,
		    const std::vector<const MappingActions *> & stack,
		    const JSONWalker::Event & event,
		    Json::Value & output) const;

	/** Perform the default handling for an event.
	 */
	void handle_default(const std::vector<const MappingActions *> & stack,
			    const JSONWalker::Event & event,
			    Json::Value & output) const;

      public:
	Mapping() : default_action(PRESERVE_TOP) {}

	/// Convert the mapping to a JSON object.
	void to_json(Json::Value & value) const;

	/// Initialise the mapping from a JSON object.
	void from_json(const Json::Value & value);

	/** Apply the mapping.
	 *
	 *  @param input The input to the mapping.
	 *  @param output A value which will be set to the output of the
	 *         mapping.  Any initial value will be overwritten.  If the
	 *         mapping isn't applied, this will be set to null.
	 *  @returns true if the mapping was applied, false if it wasn't (due
	 *         to an associated conditional not matching).
	 */
	bool apply(const Collection & collection,
		   const Json::Value & input,
		   Json::Value & output) const;
    };
};

#endif /* RESTPOSE_INCLUDED_MAPPING_H */
