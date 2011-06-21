/** @file category_hierarchy.h
 * @brief A hierarchy of categories.
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

#ifndef RESTPOSE_INCLUDED_CATEGORY_HIERARCHY_H
#define RESTPOSE_INCLUDED_CATEGORY_HIERARCHY_H

#include "json/value.h"
#include <map>
#include <set>
#include <string>

namespace RestPose {

typedef std::set<std::string> Categories;

/** A category in the hierarcy.
 */
struct Category {
    /// Name of this category.
    std::string name;

    /// Direct parents.
    Categories parents;

    /// Direct children.
    Categories children;

    /// All ancestors (including direct parents).
    Categories ancestors;

    /// All descendants (including direct children).
    Categories descendants;

    Category(const std::string & name_)
	    : name(name_)
    {}

    void add_parent(const std::string & parent, Categories & modified);
    void add_child(const std::string & child, Categories & modified);
    void add_ancestor(const std::string & ancestor, Categories & modified);
    void add_descendant(const std::string & descendant, Categories & modified);

    void remove_parent(const std::string & parent, Categories & modified);
    void remove_child(const std::string & child, Categories & modified);
    void set_ancestors(const Categories & new_ancestors,
		       Categories & modified);
    void set_descendants(const Categories & new_descendants,
			 Categories & modified);
};

/** The hierarchy of categories.
 */
class CategoryHierarchy {
    /// All the categories, by name.
    std::map<std::string, Category> categories;

    /// Recalculate the descendants of the given category.
    void recalc_descendants(const std::string & cat_name,
			    Categories & modified);

    /// Recalculate the ancestors of the given category.
    void recalc_ancestors(const std::string & cat_name,
			  Categories & modified);

    CategoryHierarchy(const CategoryHierarchy &);
    void operator=(const CategoryHierarchy &);

  public:
    CategoryHierarchy();
    ~CategoryHierarchy();

    const Category * find(const std::string & cat_name) const;

    std::map<std::string, Category>::const_iterator begin() const
    {
	return categories.begin();
    }

    std::map<std::string, Category>::const_iterator end() const
    {
	return categories.end();
    }

    size_t size() const
    {
	return categories.size();
    }

    void add(const std::string & cat_name, Categories & modified);
    void remove(const std::string & cat_name, Categories & modified);
    void add_parent(const std::string & cat_name,
		    const std::string & parent_name,
		    Categories & modified);
    void remove_parent(const std::string & cat_name,
		       const std::string & parent_name,
		       Categories & modified);

    /** Convert the hierarchy information to JSON.
     *
     *  Returns a reference to the value supplied, to allow easier use inline.
     */
    Json::Value & to_json(Json::Value & value) const;

    /** Set the hierarchy infromation from a JSON representation.
     *
     *  Wipes out any previous configuration.
     */
    void from_json(const Json::Value & value);
};

}

#endif /* RESTPOSE_INCLUDED_CATEGORY_HIERARCHY_H */
