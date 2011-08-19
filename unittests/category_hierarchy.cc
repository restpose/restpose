/** @file category_hierarchy.cc
 * @brief Tests for category hierarchies
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
#include "UnitTest++.h"

#include <cstdio>
#include <cstdlib>
#include "jsonxapian/taxonomy.h"
#include "logger/logger.h"
#include "str.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;
using namespace std;

static string
flatten(const Categories & cats)
{
    string result;
    bool need_comma = false;
    for (Categories::const_iterator i = cats.begin(); i != cats.end(); ++i) {
	if (need_comma) {
	    result += ",";
	}
	result += *i;
	need_comma = true;
    }
    return result;
}

static string
flatten(const Category * cat)
{
    if (cat == NULL) {
	return "NULL";
    }
    return flatten(cat->parents) + ":" +
	   flatten(cat->ancestors) + ":" +
	   flatten(cat->children) + ":" +
	   flatten(cat->descendants);
}

/// Test building a basic hierarchy
TEST(TaxonomyBasic)
{
    Json::Value tmp;
    Taxonomy h;

    CHECK_EQUAL("{}",
		json_serialise(h.to_json(tmp)));
    CHECK_THROW(h.from_json(string("")), InvalidValueError);
    h.from_json(json_unserialise("{}", tmp));

    // Add an initial category.
    Categories modified;
    h.add("cat1", modified);
    CHECK_EQUAL("{\"cat1\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("cat1", flatten(modified));
    CHECK_EQUAL(":::", flatten(h.find("cat1")));

    // Adding the same category again doesn't put it in modified.
    modified.clear();
    h.add("cat1", modified);
    CHECK_EQUAL("{\"cat1\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("", flatten(modified));
    CHECK_EQUAL(":::", flatten(h.find("cat1")));

    // Add a parent.
    modified.clear();
    h.add_parent("cat1", "cat2", modified);
    CHECK_EQUAL("{\"cat1\":[\"cat2\"],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("cat1,cat2", flatten(modified));
    CHECK_EQUAL("cat2:cat2::", flatten(h.find("cat1")));
    CHECK_EQUAL("::cat1:cat1", flatten(h.find("cat2")));

    // Add the child as a parent of something else.
    modified.clear();
    h.add_parent("cat0", "cat1", modified);
    CHECK_EQUAL("{\"cat0\":[\"cat1\"],\"cat1\":[\"cat2\"],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("cat0,cat1,cat2", flatten(modified));
    CHECK_EQUAL("cat1:cat1,cat2::", flatten(h.find("cat0")));
    CHECK_EQUAL("cat2:cat2:cat0:cat0", flatten(h.find("cat1")));
    CHECK_EQUAL("::cat1:cat0,cat1", flatten(h.find("cat2")));

    Json::Value saved_config;
    h.to_json(saved_config);

    // Delete cat1 - should break all links.
    modified.clear();
    h.remove("cat1", modified);
    CHECK_EQUAL("{\"cat0\":[],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("cat0,cat1,cat2", flatten(modified));
    CHECK_EQUAL(":::", flatten(h.find("cat0")));
    CHECK_EQUAL("NULL", flatten(h.find("cat1")));
    CHECK_EQUAL(":::", flatten(h.find("cat2")));

    // Deleting it again should have no effect.
    modified.clear();
    h.remove("cat1", modified);
    CHECK_EQUAL("{\"cat0\":[],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("", flatten(modified));
    CHECK_EQUAL(":::", flatten(h.find("cat0")));
    CHECK_EQUAL("NULL", flatten(h.find("cat1")));
    CHECK_EQUAL(":::", flatten(h.find("cat2")));

    Json::Value saved_config2;
    h.to_json(saved_config2);

    // Check setting the config from json works correctly.
    h.from_json(saved_config);
    CHECK_EQUAL("{\"cat0\":[\"cat1\"],\"cat1\":[\"cat2\"],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL("cat1:cat1,cat2::", flatten(h.find("cat0")));
    CHECK_EQUAL("cat2:cat2:cat0:cat0", flatten(h.find("cat1")));
    CHECK_EQUAL("::cat1:cat0,cat1", flatten(h.find("cat2")));

    h.from_json(saved_config2);
    CHECK_EQUAL("{\"cat0\":[],\"cat2\":[]}",
		json_serialise(h.to_json(tmp)));
    CHECK_EQUAL(":::", flatten(h.find("cat0")));
    CHECK_EQUAL("NULL", flatten(h.find("cat1")));
    CHECK_EQUAL(":::", flatten(h.find("cat2")));
}

static bool
check_for_loop(const Taxonomy & h, const string & child, const string & parent)
{
    if (child == parent) {
	// A trivial loop.
	return true;
    }

    const Category * child_cat = h.find(child);
    const Category * parent_cat = h.find(parent);
    if (child_cat == NULL || parent_cat == NULL) {
	// If either is new, can't be a loop.
	return false;
    }

    // Check all the ancestors for the parent.  If any are descendants of the child, we have a loop.
    for (Categories::const_iterator i = parent_cat->ancestors.begin();
	 i != parent_cat->ancestors.end(); ++i) {
	if (child_cat->descendants.find(*i) != child_cat->descendants.end()) {
	    return true;
	}
    }
    if (child_cat->descendants.find(parent) != child_cat->descendants.end()) {
	return true;
    }

    return false;
}

/// Fuzz test for modifications to a category hierarchy.
TEST(TaxonomyRandomOp)
{
    srand(42);
    int count = 200;

    Taxonomy h;
    map<string, string> flat_cats; // Flattened versions of the categories.
    while (count-- > 0) {
	Categories actual_modified;
	int action = rand() % 100;
	string c1 = "c" + str(rand() % 20);
	string c2 = "c" + str(rand() % 20);
	Categories modified;
	if (action < 30) {
	    // 30% of time, add a parent.
	    const Category * old = h.find(c1);
	    if (check_for_loop(h, c1, c2)) {
		CHECK_THROW(h.add_parent(c1, c2, modified), InvalidValueError);
	    } else if (old == NULL) {
		// Category didn't exist, so both it and the parent should be modified.
		h.add_parent(c1, c2, modified);
		CHECK(modified.find(c1) != modified.end());
		CHECK(modified.find(c2) != modified.end());
	    } else if (old->descendants.find(c2) != old->descendants.end()) {
		// Parent was already a child of this category, so should get an error.
		CHECK_THROW(h.add_parent(c1, c2, modified), InvalidValueError);
	    } else {
		// Check if the parent was already a parent of this category.
		h.add_parent(c1, c2, modified);
	    }
	} else if (action < 60) {
	    // 30% of time, add a category, with no parent.
	    const Category * old = h.find(c1);
	    h.add(c1, modified);
	    if (old == NULL) {
		CHECK(modified.find(c1) != modified.end());
	    } else {
		CHECK_EQUAL(size_t(0), modified.size());
	    }
	} else if (action < 80) {
	    // 20% of time, remove a category.
	    const Category * old = h.find(c1);
	    h.remove(c1, modified);
	    if (old == NULL) {
		CHECK_EQUAL(size_t(0), modified.size());
	    } else {
		CHECK(modified.find(c1) != modified.end());
		actual_modified.insert(c1);
	    }
	    flat_cats.erase(c1);
	} else {
	    // 20% of time, remove a parent.
	    bool changed = false;
	    const Category * old = h.find(c1);
	    if (old != NULL && old->parents.find(c2) != old->parents.end()) {
		changed = true;
	    }
	    h.remove_parent(c1, c2, modified);
	    if (changed) {
		CHECK(modified.find(c1) != modified.end());
		CHECK(modified.find(c2) != modified.end());
	    } else {
		CHECK_EQUAL(size_t(0), modified.size());
	    }
	}

	// Check that "modified" matches the categories which have changed.
	for (map<string, Category>::const_iterator i = h.begin();
	     i != h.end(); ++i) {
	    const Category & cat(i->second);
	    string newflat = flatten(&cat);
	    map<string, string>::iterator j = flat_cats.find(cat.name);
	    if (j == flat_cats.end() || j->second != newflat) {
		actual_modified.insert(cat.name);
	    }
	    flat_cats[cat.name] = newflat;

	    // Check that all parents are also ancestors.
	    for (Categories::const_iterator k = cat.parents.begin();
		 k != cat.parents.end(); ++k) {
		CHECK(cat.ancestors.find(*k) != cat.ancestors.end());
	    }
	    // Check that all children are also descendants.
	    for (Categories::const_iterator k = cat.children.begin();
		 k != cat.children.end(); ++k) {
		CHECK(cat.descendants.find(*k) != cat.descendants.end());
	    }
	    // Check that all descendants are not parents or ancestors.
	    for (Categories::const_iterator k = cat.descendants.begin();
		 k != cat.descendants.end(); ++k) {
		CHECK(cat.parents.find(*k) == cat.parents.end());
		CHECK(cat.ancestors.find(*k) == cat.ancestors.end());
	    }
	    // Check that all ancestors are not children or descendants.
	    for (Categories::const_iterator k = cat.ancestors.begin();
		 k != cat.ancestors.end(); ++k) {
		CHECK(cat.children.find(*k) == cat.children.end());
		CHECK(cat.descendants.find(*k) == cat.descendants.end());
	    }
	}
	CHECK_EQUAL(flatten(actual_modified), flatten(modified));

	// Dump to JSON and load again.
	Json::Value tmp;
	h.to_json(tmp);
	h.from_json(tmp);
	CHECK_EQUAL(flat_cats.size(), h.size());
	for (map<string, Category>::const_iterator i = h.begin();
	     i != h.end(); ++i) {
	    const Category & cat(i->second);
	    string newflat = flatten(&cat);
	    map<string, string>::iterator j = flat_cats.find(cat.name);
	    CHECK(j != flat_cats.end());
	    CHECK_EQUAL(j->second, newflat);
	}
	
    }
}
