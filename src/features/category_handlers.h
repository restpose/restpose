/** @file category_handlers.h
 * @brief Handlers related to categories.
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
#ifndef RESTPOSE_INCLUDED_CATEGORY_HANDLERS_H
#define RESTPOSE_INCLUDED_CATEGORY_HANDLERS_H

#include "rest/handler.h"

/** Get information about categories.
 *
 *  Expects 1, 2, 3 or 4 path parameters
 *
 *   - the collection name
 *   - the taxonomy name
 *   - the category id
 *   - the parent category id
 */
class CollGetCategoryHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};
class CollGetCategoryHandler : public QueuedHandler {
    std::string coll_name;
    std::string taxonomy_name;
    std::string cat_id;
    std::string parent_id;
  public:
    CollGetCategoryHandler(const std::string & coll_name_,
			   const std::string taxonomy_name_,
			   const std::string cat_id_,
			   const std::string parent_id_)
	    : coll_name(coll_name_),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    Queue::QueueState enqueue(ConnectionInfo & conn,
			      const Json::Value & body);
};

/** Set category information.
 *
 *  Expects 4 path parameters
 *
 *   - the collection name
 *   - the taxonomy name
 *   - the category id
 *   - the parent category id
 */
class CollPutCategoryHandlerFactory : public HandlerFactory {
  public:
    Handler * create(const std::vector<std::string> & path_params) const;
};
class CollPutCategoryHandler : public NoWaitQueuedHandler {
    std::string coll_name;
    std::string taxonomy_name;
    std::string cat_id;
    std::string parent_id;
  public:
    CollPutCategoryHandler(const std::string & coll_name_,
			   const std::string taxonomy_name_,
			   const std::string cat_id_,
			   const std::string parent_id_)
	    : coll_name(coll_name_),
	      taxonomy_name(taxonomy_name_),
	      cat_id(cat_id_),
	      parent_id(parent_id_)
    {}

    Queue::QueueState enqueue(ConnectionInfo & conn,
			      const Json::Value & body);
};

#endif /* RESTPOSE_INCLUDED_CATEGORY_HANDLERS_H */
