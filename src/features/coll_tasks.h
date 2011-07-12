/** @file coll_tasks.h
 * @brief Tasks related to collections.
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
#ifndef RESTPOSE_INCLUDED_COLL_TASKS_H
#define RESTPOSE_INCLUDED_COLL_TASKS_H

#include "server/basetasks.h"
#include <string>

class CollectionPool;

class CollListTask : public ReadonlyTask {
    CollectionPool & collections;
  public:
    CollListTask(const RestPose::ResultHandle & resulthandle_,
		 CollectionPool & collections_)
	    : ReadonlyTask(resulthandle_),
	      collections(collections_)
    {}

    void perform(RestPose::Collection * collection);
};

class CollInfoTask : public ReadonlyCollTask {
  public:
    CollInfoTask(const RestPose::ResultHandle & resulthandle_,
		 const std::string & coll_name_)
	    : ReadonlyCollTask(resulthandle_, coll_name_)
    {}

    void perform(RestPose::Collection * collection);
};

class CollGetConfigTask : public ReadonlyCollTask {
  public:
    CollGetConfigTask(const RestPose::ResultHandle & resulthandle_,
		      const std::string & coll_name_)
	    : ReadonlyCollTask(resulthandle_, coll_name_)
    {}

    void perform(RestPose::Collection * collection);
};


class ProcessingCollSetConfigTask : public ProcessingTask {
    Json::Value config;
  public:
    ProcessingCollSetConfigTask(const Json::Value & config_)
	    : ProcessingTask(false),
	      config(config_)
    {}
    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

class CollSetConfigTask : public IndexingTask {
    Json::Value config;
  public:
    CollSetConfigTask(const Json::Value & config_)
	    : IndexingTask(),
	      config(config_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};

#endif /* RESTPOSE_INCLUDED_COLL_TASKS_H */
