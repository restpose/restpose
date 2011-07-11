/** @file collconfigs.cc
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

#include <config.h>
#include "collconfigs.h"
#include "collection_pool.h"
#include <memory>

using namespace std;
using namespace RestPose;

CollectionConfigs::~CollectionConfigs()
{
    for (map<string, CollectionConfig *>::iterator i = configs.begin();
	 i != configs.end(); ++i) {
	delete i->second;
    }
}

CollectionConfig *
CollectionConfigs::get(const std::string & coll_name)
{
    ContextLocker lock(mutex);
    map<string, CollectionConfig *>::const_iterator i = configs.find(coll_name);
    if (i == configs.end()) {
	if (pool.exists(coll_name)) {
	    auto_ptr<Collection> coll(pool.get_readonly(coll_name));
	    auto_ptr<CollectionConfig> config(coll->get_config().clone());
	    pool.release(coll.release());

	    pair<map<string, CollectionConfig *>::iterator, bool> ret;
	    ret = configs.insert(pair<string, CollectionConfig *>(coll_name, NULL));
	    ret.first->second = config.release();
	    i = ret.first;
	} else {
	    auto_ptr<CollectionConfig> config(new CollectionConfig(coll_name));
	    config->set_default();

	    pair<map<string, CollectionConfig *>::iterator, bool> ret;
	    ret = configs.insert(pair<string, CollectionConfig *>(coll_name, NULL));
	    ret.first->second = config.release();
	    i = ret.first;
	}
    }
    return i->second->clone();
}

void
CollectionConfigs::set(const std::string & coll_name,
		       CollectionConfig * config)
{
    auto_ptr<CollectionConfig> config_ptr(config);
    ContextLocker lock(mutex);
    map<string, CollectionConfig *>::iterator i = configs.find(coll_name);
    if (i == configs.end()) {
	pair<map<string, CollectionConfig *>::iterator, bool> ret;
	ret = configs.insert(pair<string, CollectionConfig *>(coll_name, NULL));
	ret.first->second = config_ptr.release();
    } else {
	delete i->second;
	i->second = config_ptr.release();
    }
}

void
CollectionConfigs::reset(const std::string & coll_name)
{
    ContextLocker lock(mutex);
    map<string, CollectionConfig *>::iterator i = configs.find(coll_name);
    if (i == configs.end()) {
	pair<map<string, CollectionConfig *>::iterator, bool> ret;
	ret = configs.insert(pair<string, CollectionConfig *>(coll_name, NULL));
	ret.first->second = new CollectionConfig(coll_name);
	ret.first->second->set_default();
    } else {
	delete i->second;
	i->second = new CollectionConfig(coll_name);
	i->second->set_default();
    }
}
