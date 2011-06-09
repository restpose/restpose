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
    mutable Mutex mutex;
    std::map<std::string, CollectionConfig *> configs;

    CollectionConfigs(const CollectionConfigs &);
    void operator=(const CollectionConfigs &);
  public:
    CollectionConfigs() {}
    ~CollectionConfigs();

    /** Get a (newly allocated) configuration for a given collection.
     *
     *  Returns NULL if no configuration is known.
     */
    CollectionConfig * get(const std::string & coll_name) const;

    /** Set the configuration for a collection.
     *
     *  Takes ownership of the supplied config.
     */
    void set(const std::string & coll_name,
	     CollectionConfig * config);
};

}

#endif /* RESTPOSE_INCLUDED_COLLCONFIGS_H */
