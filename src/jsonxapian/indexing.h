/** @file indexing.h
 * @brief Routines used for indexing.
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

#ifndef RESTPOSE_INCLUDED_INDEXING_H
#define RESTPOSE_INCLUDED_INDEXING_H

#include "json/value.h"
#include <map>
#include <string>
#include <xapian.h>

#include "utils/rsperrors.h"
#include "schema.h"

namespace RestPose {
    class DocumentData;

    /** Base class for field indexers: these process a JSON value and store
     *  the result in a field.
     */
    class FieldIndexer {
      public:
	/** Process a field into a document.
	 *
	 *  This method should store terms and values in the document supplied.
	 *
	 *  If a value should be stored for the field, the method should set the docdata
	 *  appropriately.
	 *
	 *  If the field is an ID field, it should set the idterm appropriately.
	 *
	 *  @param doc The document to store terms and values in.
	 *  @param docdata The document data (may be modified by the method).
	 *  @param value The JSON value held in the field.
	 *  @param idterm A string which may be set to the document ID.
	 */
	virtual void index(Xapian::Document & doc,
			   DocumentData & docdata,
			   const Json::Value & value,
			   std::string & idterm) const = 0;
	virtual ~FieldIndexer();
    };

    /** A field indexer which expects a single string as input, and indexes it
     *  with a prefix.
     */
    class ExactStringIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
	unsigned int wdfinc;
	unsigned int max_length;
	MaxLenFieldConfig::TooLongAction too_long_action;
	bool isid;
      public:
	ExactStringIndexer(const std::string & prefix_,
			   const std::string & store_field_,
			   unsigned int wdfinc_,
			   unsigned int max_length_,
			   MaxLenFieldConfig::TooLongAction too_long_action_,
			   bool isid_)
		: prefix(prefix_), store_field(store_field_), wdfinc(wdfinc_),
		  max_length(max_length_), too_long_action(too_long_action_),
		  isid(isid_)
	{}

	virtual ~ExactStringIndexer();

	void index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & value,
		   std::string & idterm) const;
    };

    /** A field indexer which expects a single string as input, and stores it.
     */
    class StoredIndexer : public FieldIndexer {
	std::string store_field;
      public:
	StoredIndexer(const std::string & store_field_)
		: store_field(store_field_)
	{}

	virtual ~StoredIndexer();

	void index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & value,
		   std::string & idterm) const;
    };

    /** A field indexer which expects a number representing seconds since 1970.
     */
    class DateIndexer : public FieldIndexer {
	unsigned int slot;
	std::string store_field;
      public:
	DateIndexer(unsigned int slot_,
		    const std::string & store_field_)
		: slot(slot_), store_field(store_field_)
	{}

	virtual ~DateIndexer();

	void index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & value,
		   std::string & idterm) const;
    };

    /** A field indexer which expects an array of strings as input, and
     *  indexes them using the Xapian TermGenerator.
     */
    class TermGeneratorIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
	std::string stem_lang;
      public:
	TermGeneratorIndexer(const std::string & prefix_,
			     const std::string & store_field_,
			     const std::string & stem_lang_)
		: prefix(prefix_),
		  store_field(store_field_),
		  stem_lang(stem_lang_)
	{}

	virtual ~TermGeneratorIndexer();

	void index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & value,
		   std::string & idterm) const;
    };

    /** A field indexer which expects an array of strings as input, and
     *  indexes them as CJK text.
     */
    class CJKIndexer : public FieldIndexer {
	std::string prefix;
	std::string store_field;
      public:
	CJKIndexer(const std::string & prefix_,
		   const std::string & store_field_)
		: prefix(prefix_), store_field(store_field_)
	{}

	virtual ~CJKIndexer();

	void index(Xapian::Document & doc,
		   DocumentData & docdata,
		   const Json::Value & value,
		   std::string & idterm) const;
    };

};

#endif /* RESTPOSE_INCLUDED_INDEXING_H */