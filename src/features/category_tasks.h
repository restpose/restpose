/** @file category_tasks.h
 * @brief Tasks related to categories.
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
#ifndef RESTPOSE_INCLUDED_CATEGORY_TASKS_H
#define RESTPOSE_INCLUDED_CATEGORY_TASKS_H

#include "server/basetasks.h"
#include <string>

/** Get the category heirarchy names for a collection.
 */
class CollGetTaxonomiesTask : public ReadonlyCollTask {
    TaskManager * taskman;
  public:
    CollGetTaxonomiesTask(const RestPose::ResultHandle & resulthandle_,
			  const std::string & coll_name_,
			  TaskManager * taskman_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

/** Get a category heirarchy in a collection.
 */
class CollGetTaxonomyTask : public ReadonlyCollTask {
    const std::string taxonomy_name;
    TaskManager * taskman;
  public:
    CollGetTaxonomyTask(const RestPose::ResultHandle & resulthandle_,
			const std::string & coll_name_,
			const std::string & taxonomy_name_,
			TaskManager * taskman_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      taxonomy_name(taxonomy_name_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

/** Get a category, and its list of parents, in a collection.
 */
class CollGetCategoryTask : public ReadonlyCollTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    TaskManager * taskman;
  public:
    CollGetCategoryTask(const RestPose::ResultHandle & resulthandle_,
			const std::string & coll_name_,
			const std::string & taxonomy_name_,
			const std::string & cat_id_,
			TaskManager * taskman_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

/** Check if a category has a given parent.
 */
class CollGetCategoryParentTask : public ReadonlyCollTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    const std::string parent_id;
    TaskManager * taskman;
  public:
    CollGetCategoryParentTask(const RestPose::ResultHandle & resulthandle_,
			      const std::string & coll_name_,
			      const std::string & taxonomy_name_,
			      const std::string & cat_id_,
			      const std::string & parent_id_,
			      TaskManager * taskman_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

class ProcessingCollPutCategoryParentTask : public ProcessingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    const std::string parent_id;
  public:
    ProcessingCollPutCategoryParentTask(const std::string & taxonomy_name_,
					const std::string & cat_id_,
					const std::string & parent_id_)
	    : ProcessingTask(false),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

class CollPutCategoryParentTask : public IndexingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    const std::string parent_id;
  public:
    CollPutCategoryParentTask(const std::string & taxonomy_name_,
			      const std::string & cat_id_,
			      const std::string & parent_id_)
	    : taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};


class ProcessingCollDeleteTaxonomyTask : public ProcessingTask {
    const std::string taxonomy_name;
  public:
    ProcessingCollDeleteTaxonomyTask(const std::string & taxonomy_name_)
	    : ProcessingTask(false),
	      taxonomy_name(taxonomy_name_)
    {}

    void perform(const std::string & coll_name, TaskManager * taskman);
};

class CollDeleteTaxonomyTask : public IndexingTask {
    const std::string taxonomy_name;
  public:
    CollDeleteTaxonomyTask(const std::string & taxonomy_name_)
	    : taxonomy_name(taxonomy_name_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};


class ProcessingCollDeleteCategoryTask : public ProcessingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
  public:
    ProcessingCollDeleteCategoryTask(const std::string & taxonomy_name_,
				     const std::string & cat_id_)
	    : ProcessingTask(false),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_)
    {}

    void perform(const std::string & coll_name, TaskManager * taskman);
};

class CollDeleteCategoryTask : public IndexingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
  public:
    CollDeleteCategoryTask(const std::string & taxonomy_name_,
			   const std::string & cat_id_)
	    : taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};


class ProcessingCollDeleteCategoryParentTask : public ProcessingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    const std::string parent_id;
  public:
    ProcessingCollDeleteCategoryParentTask(const std::string & taxonomy_name_,
					   const std::string & cat_id_,
					   const std::string & parent_id_)
	    : ProcessingTask(false),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

class CollDeleteCategoryParentTask : public IndexingTask {
    const std::string taxonomy_name;
    const std::string cat_id;
    const std::string parent_id;
  public:
    CollDeleteCategoryParentTask(const std::string & taxonomy_name_,
				 const std::string & cat_id_,
				 const std::string & parent_id_)
	    : taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};

#endif /* RESTPOSE_INCLUDED_CATEGORY_TASKS_H */
