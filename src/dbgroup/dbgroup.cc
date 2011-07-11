/** @file dbgroup.cc
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

#include <config.h>
#include "dbgroup.h"

#include <cstdio>
#include "utils.h"
#include <xapian.h>
#include "utils/io_wrappers.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "jsonxapian/doctojson.h"
#include "safeerrno.h"
#include "str.h"

#include <set>

using namespace std;
using namespace RestPose;

void
DbFragment::invalidate_cache() const
{
}

DbFragment::DbFragment(const std::string & name_,
		       const std::string & path_)
	: state(CLOSED),
	  name(name_),
	  path(path_)
{}

void
DbFragment::close()
{
    switch (state) {
	case OPEN_FOR_WRITING:
	    wrdb.close();
	    state = CLOSED;
	    invalidate_cache();
	    break;
	case OPEN_FOR_READING:
	    rodb.close();
	    state = CLOSED;
	    invalidate_cache();
	    break;
	case CLOSED:
	    break;
    }
}

void
DbFragment::open_writable()
{
    if (state == OPEN_FOR_WRITING) {
	return;
    }
    state = CLOSED;
    wrdb.close();
    wrdb = Xapian::WritableDatabase(path, Xapian::DB_CREATE_OR_OPEN);
    state = OPEN_FOR_WRITING;
}

void
DbFragment::open_readonly()
{
    invalidate_cache();
    if (state == OPEN_FOR_READING) {
	rodb.reopen();
    } else {
	state = CLOSED;
	wrdb.close();
	rodb.close();
	rodb = Xapian::Database(path);
	state = OPEN_FOR_READING;
    }
}

Xapian::Database &
DbFragment::get_db()
{
    switch (state) {
	case OPEN_FOR_WRITING:
	    return wrdb;
	case OPEN_FOR_READING:
	    return rodb;
	default:
	    open_readonly();
	    return rodb;
    }
}

void
DbFragment::add_doc(const Xapian::Document & doc, const std::string & idterm)
{
    //printf("add_doc '%s' to fragment %s\n", idterm.c_str(), name.c_str());
    if (state != OPEN_FOR_WRITING) {
	throw InvalidStateError("Database must be open for writing to add document");
    }

    invalidate_cache();
    if (idterm.empty()) {
	wrdb.add_document(doc);
    } else {
	wrdb.replace_document(idterm, doc);
    }
}

void
DbFragment::delete_doc(const std::string & idterm)
{
    //printf("delete_doc '%s' from fragment %s\n", idterm.c_str(), name.c_str());
    if (state != OPEN_FOR_WRITING) {
	throw InvalidStateError("Database must be open for writing to delete document");
    }

    invalidate_cache();
    wrdb.delete_document(idterm);
}

void
DbFragment::set_metadata(const std::string & key, const std::string & value)
{
    if (state != OPEN_FOR_WRITING) {
	throw InvalidStateError("Database must be open for writing to add document");
    }

    return wrdb.set_metadata(key, value);
}

void
DbFragment::commit()
{
    if (state == OPEN_FOR_WRITING) {
	wrdb.commit();
    }
}


void
DbGroup::init_frags()
{
    Xapian::Database & db = control.get_db();
    std::string fraglist_str(db.get_metadata("_frags"));
    if (fraglist_str == last_fraglist_str) {
	return;
    } else if (fraglist_str.empty()) {
	frags.clear();
	last_fraglist_str.resize(0);
	next_fragnum = 0;
	return;
    }
    Json::Value fraglist;
    json_unserialise(fraglist_str, fraglist);
    json_check_array(fraglist, "stored list of fragments");
    frags.clear();
    for (Json::Value::iterator i = fraglist.begin();
	 i != fraglist.end(); ++i) {
	const Json::Value & fraginfo = *i;
	json_check_object(fraginfo, "stored fragment information");
	std::string fragname = json_get_string_member(fraginfo, "name",
						      std::string());
	frags.push_back(NULL);
	frags.back() = new DbFragment(fragname, groupdir + "/" + fragname);
    }
    last_fraglist_str = fraglist_str;

    std::string next_fragnum_str = db.get_metadata("_next_fragnum");
    if (next_fragnum_str.empty()) {
	next_fragnum = 0;
    } else {
	Json::Value tmp;
	json_unserialise(next_fragnum_str, tmp);
	next_fragnum = json_get_uint64(tmp);
    }
}

void
DbGroup::store_fraglist()
{
    Json::Value fraglist(Json::arrayValue);
    std::string xapiandb_contents;
    for (std::vector<DbFragment *>::iterator i = frags.begin();
	 i != frags.end(); ++i) {
	Json::Value & fraginfo = fraglist.append(Json::objectValue);
	fraginfo[Json::StaticString("name")] = (*i)->get_name();
	xapiandb_contents += "auto " + (*i)->get_name() + "\n";
    }
    control.set_metadata("_frags", json_serialise(fraglist));
    control.set_metadata("_next_fragnum", json_serialise(next_fragnum));

    std::string xapiandb_file(groupdir + "/XAPIANDB");
    std::string xapiandb_filetmp(xapiandb_file + ".tmp");
    int fd = io_open_append_create(xapiandb_filetmp.c_str(), true);
    if (fd == -1) {
	throw SysError("Couldn't create file '" + xapiandb_filetmp + "'",
		       errno);
    }
    if (!io_write(fd, xapiandb_contents)) {
	throw SysError("Couldn't write to file '" + xapiandb_filetmp + "'",
		       errno);
    }

    if (!io_close(fd)) {
	throw SysError("Couldn't close file '" + xapiandb_filetmp + "'",
		       errno);
    }
    if (rename(xapiandb_filetmp.c_str(), xapiandb_file.c_str()) == -1) {
	throw SysError("Couldn't rename temporary file to '" +
		       xapiandb_file + "'", errno);
    }
}

void
DbGroup::init_group_db() const
{
    if (!control.is_open()) {
	throw InvalidStateError("Database must be open to access groupdb");
    }
    if (group_db_valid) {
	return;
    }
    group_db = Xapian::Database();
    for (std::vector<DbFragment *>::const_iterator i = frags.begin();
	 i != frags.end(); ++i) {
	group_db.add_database((*i)->get_db());
    }
    group_db_valid = true;
}

void
DbGroup::invalidate_group_db() const
{
    if (group_db_valid) {
	group_db_valid = false;
	group_db = Xapian::Database();
    }
}

void
DbGroup::add_frag()
{
    invalidate_group_db();
    std::string fragname = "frag" + str(next_fragnum);
    next_fragnum += 1;
    frags.push_back(NULL);
    frags.back() = new DbFragment(fragname, groupdir + "/" + fragname);
    frags.back()->open_writable();

    store_fraglist();
    control.commit();
}

DbGroup::DbGroup(const std::string & groupdir_)
	: max_newdb_docs(10000000),
	  groupdir(groupdir_),
	  control("control", groupdir_ + "/control"),
	  next_fragnum(0),
	  group_db_valid(false)
{
}

DbGroup::~DbGroup()
{
    for (std::vector<DbFragment *>::iterator i = frags.begin();
	 i != frags.end(); ++i) {
	delete *i;
    }
}

void
DbGroup::close()
{
    invalidate_group_db();
    last_fraglist_str.resize(0);
    control.close();
    for (std::vector<DbFragment *>::iterator i = frags.begin();
	 i != frags.end(); ++i) {
	(*i)->close();
    }
}

void
DbGroup::open_writable()
{
    if (control.is_writable()) {
	// We have the write lock, so nothing should have been able to
	// change under us, so no need to reopen.
	return;
    }

    // Make the top directory, if needed.
    if (!dir_exists(groupdir)) {
	if (mkdir(groupdir, 0770) != 0) {
	    throw SysError("Couldn't create directory '" + groupdir + "'",
			   errno);
	}
    }

    invalidate_group_db();
    control.open_writable();
    try {
	init_frags();
    } catch(...) {
	control.close();
	throw;
    }
}

void
DbGroup::open_readonly()
{
    // We don't always need to rebuild the group db after this, if
    // all of the fragments were open, and the fragment list hasn't changed.
    // FIXME - check if this is the case, and don't invalidate the group db
    // if so.
    invalidate_group_db();

    control.open_readonly();
    try {
	init_frags();

	/* Force a reopen of all the subdbs here. */
	for (std::vector<DbFragment *>::iterator i = frags.begin();
	     i != frags.end(); ++i) {
	    (*i)->open_readonly();
	}
    } catch(...) {
	control.close();
	throw;
    }
}

const Xapian::Database &
DbGroup::get_db() const
{
    init_group_db();
    return group_db;
}

Xapian::Document
DbGroup::get_document(const std::string & idterm, bool & found) const
{
    init_group_db();
    Xapian::PostingIterator pl(group_db.postlist_begin(idterm));
    if (pl == group_db.postlist_end(idterm)) {
	found = false;
	return Xapian::Document();
    }
    found = true;
    return group_db.get_document(*pl);
}

bool
DbGroup::doc_exists(const std::string & idterm) const
{
    init_group_db();
    return group_db.term_exists(idterm);
}

Xapian::doccount
DbGroup::get_doccount() const
{
    init_group_db();
    return group_db.get_doccount();
}

void
DbGroup::add_doc(const Xapian::Document & doc, const std::string & idterm)
{
    if (!control.is_writable()) {
	throw InvalidStateError("Database group must be open to add document ");
    }

    // Ensure there is at least one fragment.
    if (frags.empty()) {
	add_frag();
    }

    if (!idterm.empty()) {
	// Check existing fragments for the document ID.  If found, add to
	// that fragment.
	for (size_t i = frags.size(); i > 0; --i) {
	    DbFragment * ptr = frags[i - 1];
	    if (ptr->get_db().term_exists(idterm)) {
		ptr->open_writable();
		ptr->add_doc(doc, idterm);
		return;
	    }
	}
    }

    // Document doesn't already exist, or no idterm - just add it to the last
    // fragment.
    Xapian::doccount docs = frags.back()->get_db().get_doccount();
    if (docs >= max_newdb_docs) {
	add_frag();
    }
    frags.back()->open_writable();
    frags.back()->add_doc(doc, idterm);
}

void
DbGroup::delete_doc(const std::string & idterm)
{
    if (!control.is_writable()) {
	throw InvalidStateError("Database group must be open to add document ");
    }
    if (idterm.empty()) {
	throw InvalidValueError("Empty term id must not be passed to delete document");
    }

    // Check existing fragments for the document ID.  If found, delete from
    // that fragment, and assume it's nowhere else.
    for (size_t i = frags.size(); i > 0; --i) {
	DbFragment * ptr = frags[i - 1];
	if (ptr->get_db().term_exists(idterm)) {
	    ptr->open_writable();
	    ptr->delete_doc(idterm);
	    return;
	}
    }
}

void
DbGroup::set_metadata(const std::string & key, const std::string & value)
{
    control.set_metadata(key, value);
}

std::string
DbGroup::get_metadata(const std::string & key)
{
    return control.get_db().get_metadata(key);
}

void
DbGroup::sync()
{
    // Commit all fragments.
    for (std::vector<DbFragment *>::iterator i = frags.begin();
	 i != frags.end(); ++i) {
	(*i)->commit();
    }
    control.commit();
}
