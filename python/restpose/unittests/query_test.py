# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import query

class DummyTarget(object):
    """A stub target that just remembers the query structure last passed to it.
    """
    last = None
    def search(self, q):
    	self.last = q
	return query.SearchResults({})

class QueryTest(TestCase):
    def test_ops(self):
        target = DummyTarget()
        q = query.QueryField("fieldname", "is", "10", target)

	results = q.search().do()
	self.assertEqual(results.offset, 0)
	self.assertEqual(results.size, 0)
	self.assertEqual(results.check_at_least, 0)
	self.assertEqual(results.matches_lower_bound, 0)
	self.assertEqual(results.matches_estimated, 0)
	self.assertEqual(results.matches_upper_bound, 0)
	self.assertEqual(results.items, [])
	self.assertEqual(results.info, {})

	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'field': ['fieldname', 'is', '10']},
			 })

	q = q * 3.14;
	q.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 3.14,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })
