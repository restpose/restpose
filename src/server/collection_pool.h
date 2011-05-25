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
#ifndef RESTPOSE_INCLUDED_COLLECTION_POOL_H
#define RESTPOSE_INCLUDED_COLLECTION_POOL_H

#include "jsonxapian/collection.h"
#include <string>
#include "utils/threading.h"

/** A pool of Collection objects.
 *
 *  Collection objects can be accessed only by a single thread at a time.  The
 *  pool maintains a set of objects ready to be used by a thread, and returned
 *  to the pool after use.
 */
class CollectionPool {
    /** Mutex obtained by all public methods.
     */
    Mutex mutex;

    /** The top directory, which collections are stored under.
     */
    std::string datadir;

    /** Maximum number of instances of each collection to retain.
     *
     *  More instances may be returned, but will be deleted as soon as they're
     *  released by callers.
     */
    size_t max_cached_readers_per_collection;

    /** The readonly collections owned by this pool, keyed by collection name.
     */
    std::map<std::string, std::vector<RestPose::Collection *> > readonly;

    /** The writable collections owned by this pool, keyed by collection name.
     */
    std::map<std::string, RestPose::Collection *> writable;

    /** No copying */
    CollectionPool(const CollectionPool &);
    /** No assignment */
    void operator=(const CollectionPool &);

  public:
    CollectionPool(const std::string & datadir_);
    ~CollectionPool();

    /** Get a pointer to a collection, opened for reading, by collection name.
     *
     *  The returned pointer will never be NULL, and ownership of the pointer
     *  passes to the caller.
     */
    RestPose::Collection * get_readonly(const std::string & collection);

    /** Get a pointer to a collection, opened for writing, by collection name.
     *
     *  May return NULL, if the collection is already owned for writing by a
     *  separate caller.  Ownership of the pointer passes to the caller.
     */
    RestPose::Collection * get_writable(const std::string & collection);

    /** Release a collection back to the pool.
     */
    void release(RestPose::Collection * collection);
};

#endif /* RESTPOSE_INCLUDED_COLLECTION_POOL_H */
