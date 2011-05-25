/** @file dbgroup.h
 * @brief A group of databases, managed as a single unit.
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

#ifndef RESTPOSE_INCLUDED_DBGROUP_H
#define RESTPOSE_INCLUDED_DBGROUP_H

#include <string>
#include <xapian.h>

namespace RestPose {

/** A handle on an individual database.
 */
class DbFragment {
    enum {
	CLOSED,
	OPEN_FOR_WRITING,
	OPEN_FOR_READING
    } state;

    Xapian::WritableDatabase wrdb;
    Xapian::Database rodb;

    std::string name;
    std::string path;

    void invalidate_cache() const;

    DbFragment(const DbFragment & other);
    void operator=(const DbFragment & other);
  public:
    DbFragment(const std::string & name_, const std::string & path_);

    const std::string & get_name() const {
	return name;
    }

    const std::string & get_path() const {
	return path;
    }

    /** Close the databases in this handle.
     */
    void close();

    /** Open the collection for writing.
     *
     *  If already open for writing, does nothing.
     */
    void open_writable();

    /** Check if the fragment is open for writing.
     *
     *  @returns true if the fragment is open for writing, false otherwise.
     */
    bool is_writable() const {
	return state == OPEN_FOR_WRITING;
    }

    /** Check if the fragment is open.
     *
     *  @returns true if the fragment is open for reading or writing, false
     *  otherwise.
     */
    bool is_open() const {
	return state != CLOSED;
    }

    /** Open the collection for reading.
     *
     *  Will reopen the collection, to point at the latest data, if it's
     *  already open.
     *
     *  If currently open for writing, closes it for writing, and opens for
     *  reading instead.
     */
    void open_readonly();

    /** Get a database object for this handle.
     *
     *  Will return a reference to whichever of wrdb or rodb is open,
     *  or open the database readonly and return rodb if it's closed.
     */
    Xapian::Database & get_db();

    /** Get a count of the number of documents in this DbFragment.
     *
     *  Database must be open already (for reading or writing).
     */
    Xapian::doccount get_doccount() const;

    /** Add a document to the database.
     *
     *  Database must be open for writing.
     *
     *  If idterm is not empty, replaces any existing document containing the
     *  same idterm.
     */
    void add_doc(const Xapian::Document & doc, const std::string & idterm);

    /** Delete a document from the database.
     *
     *  Database must be open for writing.
     */
    void delete_doc(const std::string & idterm);

    /** Set a piece of metadata in the database.
     *
     *  Database must be open for writing.
     */
    void set_metadata(const std::string & key, const std::string & value);

    /** Commit any pending changes to the database.
     */
    void commit();
};

/** A group of dbs, arranged to allow writing new documents to the end of small
 *  databases, and later merging them in.
 */
class DbGroup {
    /** The maximum number of documents to put into a new db, before starting
     *  to use a new one.
     */
    unsigned int max_newdb_docs;

    std::string groupdir;

    /** A database holding control information about the group. */
    DbFragment control;

    /** The database fragments making up the group. */
    std::vector<DbFragment *> frags;

    /** The next fragment number to use. */
    unsigned int next_fragnum;

    /** The last stored fraglist value read.
     *
     *  This is used to avoid reopening when not needed.
     */
    std::string last_fraglist_str;

    /** A database holding all the fragments.
     */
    mutable Xapian::Database group_db;

    /** True iff the group_db is valid.
     */
    mutable bool group_db_valid;

    /** Initialise the fragments from the stored config. */
    void init_frags();

    /** Store the list of fragments in the config. */
    void store_fraglist();

    /** Initialise group_db.
     */
    void init_group_db() const;

    /** Mark the group db as invalid, and free its resources.
     */
    void invalidate_group_db() const;

    /** Add a fragment to the group.
     */
    void add_frag();

    DbGroup(const DbGroup & other);
    void operator=(const DbGroup & other);
  public:
    DbGroup(const std::string & groupdir_);
    ~DbGroup();

    /** Close the databases in this group.
     */
    void close();

    /** Open the group for writing.
     *
     *  If already open for writing, does nothing.
     */
    void open_writable();

    /** Check if the group is open for writing.
     *
     *  @returns true if the group is open for writing, false otherwise.
     */
    bool is_writable() const {
	return control.is_writable();
    }

    /** Check if the group is open.
     *
     *  @returns true if the group is open for reading or writing, false
     *  otherwise.
     */
    bool is_open() const {
	return control.is_open();
    }

    /** Open the group for reading.
     *
     *  Will reopen the group, to point at the latest data, if it's
     *  already open.
     *
     *  If currently open for writing, closes it for writing, and opens for
     *  reading instead.
     */
    void open_readonly();

    /** Get a database object for this group.
     *
     *  Not valid after any modifications are made to the database, or the
     *  database is reopened.
     */
    const Xapian::Database & get_db() const;

    /** Get a document, given its idterm string.
     *
     *  @param idterm The idterm to look for.
     *  @param found Set to true if the document was found, false otherwise.
     *  @returns The document found, or an empty document if not found.
     */
    Xapian::Document get_document(const std::string & idterm,
				  bool & found) const;

    /** Check if a document exists, given its idterm string.
     *
     *  @param idterm The document ID to look for.
     *  @returns true if the document exists, false otherwise.
     */
    bool doc_exists(const std::string & idterm) const;

    /** Get the number of documents in the group.
     */
    Xapian::doccount get_doccount() const;

    /** Add a document to the database.
     */
    void add_doc(const Xapian::Document & doc, const std::string & idterm);

    /** Delete a document from the database.
     */
    void delete_doc(const std::string & idterm);

    void set_metadata(const std::string & key, const std::string & value);
    std::string get_metadata(const std::string & key);

    /** Block until all modifications are written to persistent store. */
    void sync();

#if 0
    /** Block until all modifications are available for searching. */
    void refresh();

    /** Block until the database has been compacted, and fragments merged. */
    void compact();
#endif
};

}

#endif /* RESTPOSE_INCLUDED_DBGROUP_H */
