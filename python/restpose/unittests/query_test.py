# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import query
import operator

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
	self.assertEqual(results.size_requested, 0)
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

	qm = q * 3.14
	qm.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 3.14,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })

        q2 = query.QueryField("fieldname", "is", "11", target)
        q1 = qm | q2
	q1.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'or': [
                            {'scale': {'factor': 3.14,
			      'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
			 })

        q1 = qm & q2
	q1.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'and': [
                            {'scale': {'factor': 3.14,
			      'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
			 })

        q1 = qm ^ q2
	q1.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'xor': [
                            {'scale': {'factor': 3.14,
			      'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
			 })

        q1 = qm - q2
	q1.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'not': [
                            {'scale': {'factor': 3.14,
			      'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
			 })

	qm = 2 * q
	qm.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 2,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })

	qm = q / 2
	qm.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 0.5,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })

	qm = operator.div(q, 2)
	qm.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 0.5,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })

	qm = operator.truediv(q, 2)
	qm.search().do()
	self.assertEqual(target.last.body,
			 {'from': 0,
			  'size': 10,
			  'check_at_least': 0,
			  'query': {'scale': {'factor': 0.5,
			    'query': {'field': ['fieldname', 'is', '10']}}},
			 })
