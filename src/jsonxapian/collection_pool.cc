/** @file collection_pool.cc
 * @brief A pool of collections, for sharing between threads.
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
#include "collection_pool.h"
#include "diritor.h"
#include <memory>
#include "omassert.h"
#include "safeerrno.h"
#include "utils/rmdir.h"
#include "utils/rsperrors.h"
#include "utils/stringutils.h"
#include "utils.h"

#define DIR_SEPARATOR "/"

using namespace std;
using namespace RestPose;

CollectionPool::CollectionPool(const string & datadir_)
	: datadir(datadir_),
	  max_cached_readers_per_collection(0) // FIXME - currently, we disable caching of readers because when a collection is deleted, we need to know to invalidate all cached readers, including those which are currently assigned to tasks.  To do this, we need to start tracking the assigned readers, and add some way of checking whether the collection has been deleted since these readers were assigned, and skip adding the readers to the cache when they are returned in this situation.  For a quick fix, until this can be implemented, I've just disabled caching of readers.
{
    if (!string_endswith(datadir, DIR_SEPARATOR)) {
	datadir += DIR_SEPARATOR;
    }
    string stripped_datadir = datadir.substr(0, datadir.size() - 1);
    if (!dir_exists(stripped_datadir)) {
	if (mkdir(stripped_datadir, 0777)) {
	    throw SysError("Unable to create datadir \"" +
			   stripped_datadir + "\"", errno);
	}
    }
}

CollectionPool::~CollectionPool()
{
    for (map<string, vector<Collection *> >::const_iterator
	 i = readonly.begin(); i != readonly.end(); ++i) {
	for (vector<Collection *>::const_iterator
	     j = i->second.begin(); j != i->second.end(); ++j) {
	    delete *j;
	}
    }
    for (map<string, Collection *>::const_iterator
	 i = writable.begin(); i != writable.end(); ++i) {
	delete i->second;
    }
}

bool
CollectionPool::exists(const string & collection)
{
    ContextLocker lock(mutex);
    if (readonly.find(collection) != readonly.end()) {
	return true;
    }
    if (writable.find(collection) != writable.end()) {
	return true;
    }
    if (dir_exists(datadir + collection)) {
	return true;
    }
    return false;
}

void
CollectionPool::del(const std::string & coll_name)
{
    ContextLocker lock(mutex);

    map<string, vector<Collection *> >::iterator
	    i = readonly.find(coll_name);
    if (i != readonly.end()) {
	for (vector<Collection *>::iterator k = i->second.begin();
	     k != i->second.end(); ++k) {
	    delete *k;
	}
	readonly.erase(i);
    }

    map<string, Collection *>::iterator j = writable.find(coll_name);
    if (j != writable.end()) {
	delete j->second;
	writable.erase(j);
    }

    string topdir = datadir + coll_name;
    if (dir_exists(topdir)) {
	rmdir_recursive(topdir);
    }
}

Collection *
CollectionPool::get_readonly(const string & collection)
{
    auto_ptr<Collection> result;
    map<string, vector<Collection *> >::iterator i;
    ContextLocker lock(mutex);
    i = readonly.find(collection);
    if (i == readonly.end() || i->second.empty()) {
	result = auto_ptr<Collection>(
		new Collection(collection, datadir + collection));
    } else {
	result = auto_ptr<Collection>(i->second.back());
	i->second.pop_back();
    }
    result->open_readonly();
    return result.release();
}

Collection *
CollectionPool::get_writable(const string & collection)
{
    auto_ptr<Collection> result;
    map<string, Collection *>::iterator i;
    ContextLocker lock(mutex);
    i = writable.find(collection);
    if (i == writable.end() || i->second == NULL) {
	result = auto_ptr<Collection>(
		new Collection(collection, datadir + collection));
    } else {
	result = auto_ptr<Collection>(i->second);
	i->second = NULL;
    }
    result->open_writable();
    return result.release();
}

void
CollectionPool::release(Collection * collection)
{
    if (collection == NULL) {
	return;
    }
    auto_ptr<Collection> collptr(collection);

    ContextLocker lock(mutex);
    if (collection->is_writable()) {
	pair<map<string, Collection *>::iterator, bool> ret;
	pair<string, Collection *> item(collection->get_name(), NULL);
	ret = writable.insert(item);
	if (ret.second) {
	    // New element was inserted
	    ret.first->second = collptr.release();
	} else {
	    // ret.first points to an existing element.
	    if (ret.first->second == NULL) {
		ret.first->second = collptr.release();
	    }
	}
    } else {
	map<string, vector<Collection *> >::iterator i;
	i = readonly.find(collection->get_name());
	if (i == readonly.end()) {
	    // Insert a new element.
	    pair<map<string, vector<Collection *> >::iterator, bool> ret;
	    pair<string, vector<Collection *> > item;
	    item.first = collection->get_name();
	    ret = readonly.insert(item);
	    Assert(ret.second);
	    i = ret.first;
	}
	if (i->second.size() < max_cached_readers_per_collection) {
	    i->second.push_back(NULL);
	    i->second.back() = collptr.release();
	}
    }
}

void
CollectionPool::get_names(std::vector<std::string> & result)
{
    result.clear();
    DirectoryIterator diriter(false);
    diriter.start(datadir);
    while (diriter.next()) {
	if (diriter.get_type() == DirectoryIterator::DIRECTORY) {
	    result.push_back(diriter.leafname());
	}
    }
}
