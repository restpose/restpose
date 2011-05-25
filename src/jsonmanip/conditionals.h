/** @file conditionals.h
 * @brief Conditional expressions applied to JSON documents.
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

#ifndef RESTPOSE_INCLUDED_CONDITIONALS_H
#define RESTPOSE_INCLUDED_CONDITIONALS_H

#include "json/value.h"

#include "jsonpath.h"

namespace RestPose {

    /** Base class of conditional clauses.
     */
    class ConditionalClause {
      public:
	/** The name of the JSON element that this clause's configuration is
	 *  stored under. */
	std::string name;

	ConditionalClause(const std::string & name_) : name(name_) {}
	virtual ~ConditionalClause();

	/** Dump the clause to a JSON object.
	 *
	 *  @param value A JSON value which will have the representation of the
	 *  clause assigned to it.  The initial value will be overwritten.
	 */
	virtual void to_json(Json::Value & value) const = 0;

	/// Build a new ConditionalClause subclass from some JSON.
	static ConditionalClause * from_json(const Json::Value & value);

	/// Apply this conditional against a value.
	virtual Json::Value apply(const Json::Value & value) const = 0;
    };

    /** A conditional clause that tests if a field exists.
     */
    class ConditionalClauseExists : public ConditionalClause {
	/** The path to the the element, from the base JSON object.
	 */
	JSONPath path;
      public:
	/// Initialise from json.
	ConditionalClauseExists(const Json::Value & value);

	/** Dump to json.  Uses the format: {"exists": [path] }
	 */
	void to_json(Json::Value & value) const;

	/// Apply this conditional against a value.
	Json::Value apply(const Json::Value & value) const;
    };

    /** A conditional clause that gets a field from the document.
     */
    class ConditionalClauseGet : public ConditionalClause {
	/** The path to the the element, from the base JSON object.
	 */
	JSONPath path;
      public:
	/// Initialise from json.
	ConditionalClauseGet(const Json::Value & value);

	/** Dump to json.  Uses the format: {"exists": [path] }
	 */
	void to_json(Json::Value & value) const;

	/// Apply this conditional to a document.
	Json::Value apply(const Json::Value & document) const;
    };

    /** A conditional clause which returns a literal value.
     */
    class ConditionalClauseLiteral : public ConditionalClause {
	/// The value that this clause returns.
	Json::Value value;
      public:
	/// Initialise from json.
	ConditionalClauseLiteral(const Json::Value & value_);

	/** Dump to json.
	 */
	void to_json(Json::Value & value_) const;

	/// apply this conditional.
	Json::Value apply(const Json::Value &) const;
    };

    /** A conditional clause that tests if items are equal.
     */
    class ConditionalClauseEquals : public ConditionalClause {
	std::vector<ConditionalClause *> children;
      public:
	/// Initialise from json.
	ConditionalClauseEquals(const Json::Value & value);

	~ConditionalClauseEquals();

	/** Dump to json.
	 */
	void to_json(Json::Value & value) const;

	/// Apply this conditional.
	Json::Value apply(const Json::Value &document) const;
    };

    /** A conditional expression, to be applied to a JSON document.
     */
    class Conditional {
	/// The conditional clause - owned by the Conditional.
	ConditionalClause * clause;

      public:
	Conditional(const Conditional & other);

	Conditional & operator=(const Conditional & other);

	Conditional()
		: clause(NULL)
	{}
	~Conditional() {
	    delete clause;
	}

	/// Convert the conditional to a JSON object.
	void to_json(Json::Value & value) const;

	/// Initialise the conditional from a JSON object.
	void from_json(const Json::Value & value);

	/** Test the conditional.
	 *
	 *  An uninitialised conditional, or one initialised from null, will
	 *  raise an InvalidValueError if tested.
	 */
	bool test(const Json::Value & value) const;

	/** Check if the conditional is null.
	 *
	 *  An uninitialised conditional, or one initialised from null, will
	 *  raise an InvalidValueError if tested.
	 */
	bool is_null() const { return clause == NULL; }
    };
};

#endif /* RESTPOSE_INCLUDED_CONDITIONALS_H */
