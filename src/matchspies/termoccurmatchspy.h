/** @file termoccurmatchspy.h
 * @brief MatchSpy for counting the occurrences of terms.
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

#ifndef RESTPOSE_INCLUDED_TERMOCCURMATCHSPY_H
#define RESTPOSE_INCLUDED_TERMOCCURMATCHSPY_H

#include <json/value.h>
#include <map>
#include <set>
#include <string>
#include <xapian.h>

namespace RestPose {

class BaseTermOccurMatchSpy : public Xapian::MatchSpy {
  protected:
    /** Number of documents seen by the matchspy.
     */
    Xapian::doccount docs_seen;

    /** Limit on the number of documents considered by matchspy.
     */
    Xapian::doccount doc_limit;

    /** Number of terms seen by the matchspy.
     */
    Xapian::termcount terms_seen;

    /** Maximum number of resulting term values to return.
     */
    Xapian::doccount result_limit;

    /** Prefix to look for in terms to count.
     */
    std::string prefix;

    /** Original prefix (prefix without a possible trailing tab).
     */
    std::string orig_prefix;

    /** Term suffixes to ignore.
     */
    std::set<std::string> stopwords;

    /** Count of number of times each term suffix has been seen;
     */
    std::map<std::string, Xapian::doccount> counts;

    /** True iff number of documents each term is contained in should be
     *  returned.
     */
    bool get_termfreqs;

    /** The database being searched.
     */
    const Xapian::Database * db;

  public:
    BaseTermOccurMatchSpy(const std::string & prefix_,
			  Xapian::doccount doc_limit_,
			  Xapian::doccount result_limit_,
			  bool get_termfreqs_,
			  const Xapian::Database * db_);

    void add_stopword(const std::string & word);

    virtual void operator()(const Xapian::Document &doc, Xapian::weight wt) = 0;

    virtual void get_result(Json::Value & result) const = 0;
};

class TermOccurMatchSpy : public BaseTermOccurMatchSpy {
  public:
    TermOccurMatchSpy(const std::string & prefix_,
		      Xapian::doccount doc_limit_,
		      Xapian::doccount result_limit_,
		      bool get_termfreqs_,
		      const Xapian::Database * db_)
	    : BaseTermOccurMatchSpy(prefix_, doc_limit_, result_limit_,
				    get_termfreqs_, db_)
    {}

    void operator()(const Xapian::Document &doc, Xapian::weight wt);

    void get_result(Json::Value & result) const;
};

class TermCoOccurMatchSpy : public BaseTermOccurMatchSpy {
  public:
    TermCoOccurMatchSpy(const std::string & prefix_,
			Xapian::doccount doc_limit_,
			Xapian::doccount result_limit_,
			bool get_termfreqs_,
			const Xapian::Database * db_)
	    : BaseTermOccurMatchSpy(prefix_, doc_limit_, result_limit_,
				    get_termfreqs_, db_)
    {}

    void operator()(const Xapian::Document &doc, Xapian::weight wt);

    void get_result(Json::Value & result) const;
};

}

#endif /* RESTPOSE_INCLUDED_TERMOCCURMATCHSPY_H */
