/** @file collconfigs.h
 * @brief A holder for collection configurations.
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
#ifndef RESTPOSE_INCLUDED_COLLCONFIGS_H
#define RESTPOSE_INCLUDED_COLLCONFIGS_H

#include "jsonxapian/collconfig.h"
#include <map>
#include <string>
#include "utils/threading.h"

class CollectionPool;

namespace RestPose {

/** Holds CollectionConfig objects for each collection.
 *
 *  Used to allow processing threads to get the appropriate configuration, even
 *  if it hasn't been comitted to the collection yet.
 *
 *  This is threadsafe - accesses are serialised by an internal mutex, and the
 *  returned results are new objects sharing no state with the internal
 *  objects.
 */
class CollectionConfigs {
    Mutex mutex;
    std::map<std::string, CollectionConfig *> configs;
    CollectionPool & pool;

    CollectionConfigs(const CollectionConfigs &);
    void operator=(const CollectionConfigs &);
  public:
    CollectionConfigs(CollectionPool & pool_) : pool(pool_) {}
    ~CollectionConfigs();

    /** Get a (newly allocated) configuration for a given collection.
     *
     *  If the configuration isn't already known, attempts to get a
     *  corresponding collection from the collection pool and reads the
     *  configuration from that.  If no such collection exists, returns
     *  a default configuration.
     */
    CollectionConfig * get(const std::string & coll_name);

    /** Set the configuration for a collection.
     *
     *  Takes ownership of the supplied config.
     */
    void set(const std::string & coll_name,
	     CollectionConfig * config);

    /** Reset the stored configuration for a given collection.
     *
     *  This sets the stored configuration to the default configuration.  This
     *  is used when the collection is deleted, to ensure that if a new
     *  processing task enters the queue before the collection has been
     *  deleted from disk, the old configuration isn't read from the old
     *  collection.
     */
    void reset(const std::string & coll_name);
};

}

#endif /* RESTPOSE_INCLUDED_COLLCONFIGS_H */
