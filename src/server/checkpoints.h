/** @file checkpoints.h
 * @brief Checkpoints, for monitoring indexing progress.
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

#ifndef RESTPOSE_INCLUDED_CHECKPOINTS_H
#define RESTPOSE_INCLUDED_CHECKPOINTS_H

#include "json/value.h"
#include <string>
#include "utils/threading.h"
#include <vector>

/** An error when indexing.
 */
class IndexingError {
    /** A human readable message about the error.
     */
    std::string msg;

    /** The type of the document being processed when the error occurred.
     *
     *  Set to empty if no document was being processed.
     */
    std::string doc_type;

    /** The id of the document being processed when the error occurred.
     *
     *  Set to empty if no document was being processed.
     */
    std::string doc_id;

  public:
    IndexingError(const std::string & msg_,
		  const std::string & doc_type_,
		  const std::string & doc_id_)
	    : msg(msg_),
	      doc_type(doc_type_),
	      doc_id(doc_id_)
    {}

    /** Set a Json value to be a dict describing the error.
     */
    Json::Value & to_json(Json::Value & result) const;
};

/** A log of indexing errors since a checkpoint.
 *
 *  This is kept in memory, and used to provide an error report
 *  when a checkpoint has been reached.
 */
class IndexingErrorLog {
    /** A list of the first few errors which occurred since the previous checkpoint.
     *
     *  This list will be of size no larger than max_errors.
     */
    std::vector<IndexingError> errors;

    /** A count of the total number of errors since the last checkpoint.
     */
    unsigned int total_errors;

    /** Maximum number of errors to preserve.
     */
    unsigned int max_errors;

  public:
    /** Make a new IndexingErrorLog.
     *
     * @param max_errors_ The maximum number of errors to record in
     * detail in the log.  Further errors will increase the error
     * count, but not have their details recorded.
     */
    IndexingErrorLog(unsigned int max_errors_)
	    : total_errors(0),
	      max_errors(max_errors_)
    {}

    /** Append an error.
     */
    void append_error(const std::string & msg,
		      const std::string & doc_type,
		      const std::string & doc_id);

    /** Set entries in a Json object describing the error log.
     *
     * @param result The json object to set entries in.  Must be a Json object
     * value.
     */
    Json::Value & to_json(Json::Value & result) const;
};

/** A checkpoint for tasks on a collection.
 */
class CheckPoint {
    /** The errors associated with the checkpoint.
     */
    IndexingErrorLog * errors;

    /** Time at which the checkpoint was last touched.
     */
    mutable double last_touched;

    /** Whether the checkpoint has been reached yet.
     */
    bool reached;

  public:
    /** Create a checkpoint.
     *
     *  Initially, the checkpoint has no errors associated, and has not been
     *  reached.
     */
    CheckPoint();

    ~CheckPoint();

    /** Mark the checkpoint as having been reached.
     *
     *  Takes ownership of the supplied errors object.
     *
     *  @param errors_ The errors to associate with the checkpoint.  May be NULL.
     */
    void set_reached(IndexingErrorLog * errors_);

    /** Get the status of the checkpoint.
     *
     *  Sets the provided Json value to describe the checkpoint status, and
     *  also returns the provided value.
     */
    Json::Value & get_state(Json::Value & result) const;

    /** Get the time in seconds since the CheckPoint was modified or checked
     *  (with get_state()).
     *
     *  Used for removing old checkpoints.
     */
    double seconds_since_touched() const;
};

/** Known checkpoints for a collection.
 */
class CheckPoints {
    /** Map from the checkpoint ID to the checkpoint data.
     */
    std::map<std::string, CheckPoint> points;

    CheckPoints(const CheckPoints &);
    void operator=(const CheckPoints &);
  public:
    CheckPoints() {}
    ~CheckPoints() {}

    /** Expire any checkpoints which haven't been touched recently.
     *
     *  @param max_age The number of seconds after which checkpoints should be
     *  removed if they haven't been touched.
     */
    void expire(double max_age);

    /** Allocate a new checkpoint, and return its id.
     */
    std::string alloc_checkpoint();

    /** Get the list of checkpoint IDs as a JSON value.
     */
    Json::Value & ids_to_json(Json::Value & result) const;

    /** Mark a checkpoint as having been reached.
     *
     *  If the checkpoint doesn't exist (or has expired), this creates it.
     *
     *  Takes ownership of the supplied errors object.
     *
     *  @param checkid The checkpoint to mark.
     *  @param errors The errors to associate with the checkpoint.  May be NULL.
     */
    void set_reached(const std::string & checkid,
		     IndexingErrorLog * errors);

    /** Get the status of the checkpoint.
     *
     *  Sets the provided Json value to describe the checkpoint status, and
     *  also returns the provided value.
     *
     *  Returns a Json null value if the checkpoint isn't found (whether
     *  because it never existed, or because it has expired).
     *
     *  @param checkid The checkpoint to get the status from..
     */
    Json::Value & get_state(const std::string & checkid,
			    Json::Value & result) const;
};

#endif /* RESTPOSE_INCLUDED_CHECKPOINTS_H */
