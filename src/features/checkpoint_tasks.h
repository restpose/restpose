/** @file checkpoint_tasks.h
 * @brief Tasks related to checkpoints.
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
#ifndef RESTPOSE_INCLUDED_CHECKPOINT_TASKS_H
#define RESTPOSE_INCLUDED_CHECKPOINT_TASKS_H

#include "server/basetasks.h"
#include <string>

/// A checkpoint on the processing queue.
class ProcessorCheckpointTask : public ProcessingTask {
    std::string checkid;

    /** Whether to cause an immediate commit once all the tasks before the
     *  checkpoint have been performed. */
    bool do_commit;

  public:
    ProcessorCheckpointTask(const std::string & checkid_,
			    bool do_commit_=true)
	    : ProcessingTask(false),
	      checkid(checkid_),
	      do_commit(do_commit_)
    {}

    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

/// A checkpoint on the indexing queue.
class IndexingCheckpointTask : public IndexingTask {
    std::string checkid;
    bool do_commit;
  public:
    IndexingCheckpointTask(const std::string & checkid_,
			   bool do_commit_=true)
	    : IndexingTask(),
	      checkid(checkid_),
	      do_commit(do_commit_)
    {}

    void perform(RestPose::Collection & collection,
		 TaskManager * taskman);
    IndexingTask * clone() const;
};

/** Get the list of checkpoints for a collection.
 *
 *  Note - this is a ReadonlyTask instead of a ReadonlyCollTask because it
 *  doesn't access the collection object - it just accesses the checkpoint list
 *  for the collection, which is stored separately.
 */
class CollGetCheckpointsTask : public ReadonlyTask {
    const std::string coll_name;
    TaskManager * taskman;
  public:
    CollGetCheckpointsTask(const RestPose::ResultHandle & resulthandle_,
			   const std::string & coll_name_,
			   TaskManager * taskman_)
	    : ReadonlyTask(resulthandle_),
	      coll_name(coll_name_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

/** Get the status of a checkpoint in a collection.
 *
 *  Note - this is a ReadonlyTask instead of a ReadonlyCollTask because it
 *  doesn't access the collection object - it just accesses the checkpoint list
 *  for the collection, which is stored separately.
 */
class CollGetCheckpointTask : public ReadonlyTask {
    const std::string coll_name;
    TaskManager * taskman;
    std::string checkid;
  public:
    CollGetCheckpointTask(const RestPose::ResultHandle & resulthandle_,
			  const std::string & coll_name_,
			  TaskManager * taskman_,
			  const std::string & checkid_)
	    : ReadonlyTask(resulthandle_),
	      coll_name(coll_name_),
	      taskman(taskman_),
	      checkid(checkid_)
    {}

    void perform(RestPose::Collection * collection);
};

#endif /* RESTPOSE_INCLUDED_CHECKPOINT_TASKS_H */
