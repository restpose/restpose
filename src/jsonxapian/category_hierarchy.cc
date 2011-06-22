/** @file category_hierarchy.cc
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

#include <config.h>
#include "category_hierarchy.h"

#include "logger/logger.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include "utils/stringutils.h"

using namespace RestPose;
using namespace std;

void
Category::add_parent(const std::string & parent, Categories & modified)
{
    if (parents.insert(parent).second + ancestors.insert(parent).second) {
	modified.insert(name);
    }
}

void
Category::add_child(const std::string & child, Categories & modified)
{
    if (children.insert(child).second + descendants.insert(child).second) {
	modified.insert(name);
    }
}

void
Category::add_ancestor(const std::string & ancestor, Categories & modified)
{
    if (ancestors.insert(ancestor).second) {
	modified.insert(name);
    }
}

void
Category::add_descendant(const std::string & descendant, Categories & modified)
{
    if (descendants.insert(descendant).second) {
	modified.insert(name);
    }
}

void
Category::remove_parent(const std::string & parent, Categories & modified)
{
    if (parents.erase(parent)) {
	modified.insert(name);
    }
}

void
Category::remove_child(const std::string & child, Categories & modified)
{
    if (children.erase(child)) {
	modified.insert(name);
    }
}

void
Category::set_ancestors(const Categories & new_ancestors,
			Categories & modified)
{
    if (ancestors != new_ancestors) {
	modified.insert(name);
    }
    ancestors = new_ancestors;
}

void
Category::set_descendants(const Categories & new_descendants,
			  Categories & modified)
{
    LOG_DEBUG("Setting descendants of '" + name + "' to '" +
	      string_join(",", new_descendants.begin(),
			  new_descendants.end()) +
	      "' from '" +
	      string_join(",", descendants.begin(),
			  descendants.end()) + "'");
    if (descendants != new_descendants) {
	modified.insert(name);
    }
    descendants = new_descendants;
}


void
CategoryHierarchy::recalc_ancestors(const string & cat_name,
				    Categories & modified)
{
    LOG_DEBUG("Recalculating ancestors of '" + cat_name + "'");
    Categories ancestors;
    Categories seen;
    Categories stack;
    stack = categories.find(cat_name)->second.parents;
    while (!stack.empty()) {
	Category & cat = categories.find(*(stack.begin()))->second;
	stack.erase(stack.begin());
	seen.insert(cat.name);
	ancestors.insert(cat.name);
	for (Categories::const_iterator i = cat.parents.begin();
	     i != cat.parents.end(); ++i) {
	    stack.insert(*i);
	}
    }
    categories.find(cat_name)->second.set_ancestors(ancestors, modified);
}

void
CategoryHierarchy::recalc_descendants(const string & cat_name,
				      Categories & modified)
{
    LOG_DEBUG("Recalculating descendants of '" + cat_name + "'");
    Categories descendants;
    Categories seen;
    Categories stack;
    stack = categories.find(cat_name)->second.children;
    while (!stack.empty()) {
	Category & cat = categories.find(*(stack.begin()))->second;
	stack.erase(stack.begin());
	seen.insert(cat.name);
	descendants.insert(cat.name);
	for (Categories::const_iterator i = cat.children.begin();
	     i != cat.children.end(); ++i) {
	    stack.insert(*i);
	}
    }
    categories.find(cat_name)->second.set_descendants(descendants, modified);
}

const Category *
CategoryHierarchy::find(const std::string & cat_name) const
{
    map<string, Category>::const_iterator i = categories.find(cat_name);
    if (i == categories.end()) {
	return NULL;
    }
    return &(i->second);
}

void
CategoryHierarchy::add(const string & cat_name, Categories & modified)
{
    if (categories.find(cat_name) == categories.end()) {
	categories.insert(make_pair(cat_name, Category(cat_name)));
	modified.insert(cat_name);
    }
}

void
CategoryHierarchy::remove(const string & cat_name, Categories & modified)
{
    map<string, Category>::iterator i = categories.find(cat_name);
    if (i == categories.end()) return;
    Category & category = i->second;
    LOG_DEBUG("Removing category: parents='" +
	      string_join(",",
			  category.parents.begin(),
			  category.parents.end()) +
	      "' children='" +
	      string_join(",",
			  category.children.begin(),
			  category.children.end()) +
	      "' ancestors='" +
	      string_join(",",
			  category.ancestors.begin(),
			  category.ancestors.end()) +
	      "' descendants='" +
	      string_join(",",
			  category.descendants.begin(),
			  category.descendants.end()) +
	      "'"
	     );

    for (Categories::const_iterator j = category.parents.begin();
	 j != category.parents.end(); ++j) {
	categories.find(*j)->second.remove_child(cat_name, modified);
    }
    for (Categories::const_iterator j = category.children.begin();
	 j != category.children.end(); ++j) {
	categories.find(*j)->second.remove_parent(cat_name, modified);
    }

    for (Categories::const_iterator j = category.ancestors.begin();
	 j != category.ancestors.end(); ++j) {
	recalc_descendants(*j, modified);
    }
    for (Categories::const_iterator j = category.descendants.begin();
	 j != category.descendants.end(); ++j) {
	recalc_ancestors(*j, modified);
    }
    categories.erase(cat_name);
    modified.insert(cat_name);
}

void
CategoryHierarchy::add_parent(const string & cat_name,
			      const string & parent_name,
			      Categories & modified)
{
    if (cat_name == parent_name) {
	throw InvalidValueError("Cannot set category as parent of itself");
    }
    add(cat_name, modified);
    add(parent_name, modified);
    Category & cat = categories.find(cat_name)->second;
    Category & parent = categories.find(parent_name)->second;

    // Ensure no loop happens by checking that none of the ancestors of the
    // new parent are descendants of the category.
    for (Categories::iterator i = parent.ancestors.begin();
	 i != parent.ancestors.end(); ++i) {
	if (cat.descendants.find(*i) != cat.descendants.end()) {
	    throw InvalidValueError("Attempt to create loop in category "
				    "hierarchy: '" + parent_name + "' is a "
				    "descendant of '" + cat_name +
				    "' - can't add it as a parent");
	}
    }
    if (cat.descendants.find(parent_name) != cat.descendants.end()) {
	throw InvalidValueError("Attempt to create loop in category "
				"hierarchy: '" + parent_name + "' is a "
				"descendant of '" + cat_name +
				"' - can't add it as a parent");
    }

    cat.add_parent(parent_name, modified);
    cat.add_ancestor(parent_name, modified);
    parent.add_child(cat_name, modified);
    parent.add_descendant(cat_name, modified);

    // Add all ancestors of cat as ancestors of all descendants of cat.
    for (Categories::const_iterator i = cat.descendants.begin();
	 i != cat.descendants.end(); ++i) {
	for (Categories::const_iterator j = cat.ancestors.begin();
	     j != cat.ancestors.end(); ++j) {
	    categories.find(*i)->second.add_ancestor(*j, modified);
	    categories.find(*j)->second.add_descendant(*i, modified);
	}
    }

    // Add all descendants of parent as descendants of all ancestors of
    // parent.
    for (Categories::const_iterator i = parent.ancestors.begin();
	 i != parent.ancestors.end(); ++i) {
	for (Categories::const_iterator j = parent.descendants.begin();
	     j != parent.descendants.end(); ++j) {
	    categories.find(*i)->second.add_descendant(*j, modified);
	    categories.find(*j)->second.add_ancestor(*i, modified);
	}
    }
}

void
CategoryHierarchy::remove_parent(const string & cat_name,
				 const string & parent_name,
				 Categories & modified)
{
    if (cat_name == parent_name) {
	return;
    }
    map<string, Category>::iterator i = categories.find(cat_name);
    if (i == categories.end()) return;
    Category & child = i->second;

    i = categories.find(parent_name);
    if (i == categories.end()) return;
    Category & parent = i->second;

    parent.remove_child(cat_name, modified);
    child.remove_parent(parent_name, modified);

    recalc_ancestors(cat_name, modified);
    recalc_descendants(parent_name, modified);

    for (Categories::const_iterator j = child.descendants.begin();
	 j != child.descendants.end(); ++j) {
	recalc_ancestors(*j, modified);
    }
    for (Categories::const_iterator j = parent.ancestors.begin();
	 j != parent.ancestors.end(); ++j) {
	recalc_descendants(*j, modified);
    }
}

Json::Value &
CategoryHierarchy::to_json(Json::Value & value) const
{
    value = Json::objectValue;
    for (map<string, Category>::const_iterator i = categories.begin();
	 i != categories.end(); ++i) {
	Json::Value & parents = value[i->first] = Json::arrayValue;
	for (set<string>::const_iterator j = i->second.parents.begin();
	     j != i->second.parents.end(); ++j) {
	    parents.append(*j);
	}
    }
    return value;
}

void
CategoryHierarchy::from_json(const Json::Value & value)
{
    categories.clear();
    json_check_object(value, "category hierarchy");
    Categories modified;
    for (Json::ValueIterator i = value.begin(); i != value.end(); ++i) {
	string name = i.memberName();
	add(name, modified);
	Json::Value & item(*i);
	if (!item.isNull()) {
	    json_check_array(item, "list of category parents");
	    for (Json::ValueIterator j = item.begin(); j != item.end(); ++j) {
		json_check_string(*j, "category parent");
		add_parent(name, (*j).asString(), modified);
	    }
	}
    }
}
