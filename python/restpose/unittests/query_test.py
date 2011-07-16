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
    count = 0
    def search(self, search):
        self.last = search._build_search()
        self.count = self.count + 1
        return query.SearchResults({})


class QueryTest(TestCase):
    def check_target(self, target, expected_last):
        self.assertEqual(target.count, 1)
        self.assertEqual(target.last, expected_last)
        target.count = 0
        
    def test_ops(self):
        target = DummyTarget()
        q = query.QueryField("fieldname", "is", "10", target)

        self.assertEqual(q.results.offset, 0)
        self.assertEqual(q.results.size_requested, 0)
        self.assertEqual(q.results.check_at_least, 0)
        self.assertEqual(q.results.matches_lower_bound, 0)
        self.assertEqual(q.results.matches_estimated, 0)
        self.assertEqual(q.results.matches_upper_bound, 0)
        self.assertEqual(q.results.items, [])
        self.assertEqual(q.results.info, {})
        self.check_target(target,
                          {
                           'query': {'field': ['fieldname', 'is', '10']},
                          })

        qm = q * 3.14
        qm.results
        self.check_target(target,
                          {
                           'query': {'scale': {'factor': 3.14,
                             'query': {'field': ['fieldname', 'is', '10']}}},
                          })

        q2 = query.QueryField("fieldname", "is", "11", target)
        q1 = qm | q2
        q1.results
        self.check_target(target,
                         {
                          'query': {'or': [
                            {'scale': {'factor': 3.14,
                              'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
                         })

        q1 = qm & q2
        q1.results
        self.check_target(target,
                         {
                          'query': {'and': [
                            {'scale': {'factor': 3.14,
                              'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
                         })

        q1 = qm ^ q2
        q1.results
        self.check_target(target,
                         {
                          'query': {'xor': [
                            {'scale': {'factor': 3.14,
                              'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
                         })

        q1 = qm - q2
        q1.results
        self.check_target(target,
                         {
                          'query': {'not': [
                            {'scale': {'factor': 3.14,
                              'query': {'field': ['fieldname', 'is', '10']}
                            }},
                            {'field': ['fieldname', 'is', '11']}
                          ]}
                         })

        qm = 2 * q
        qm.results
        self.check_target(target,
                         {
                          'query': {'scale': {'factor': 2,
                            'query': {'field': ['fieldname', 'is', '10']}}},
                         })

        qm = q / 2
        qm.results
        self.check_target(target,
                         {
                          'query': {'scale': {'factor': 0.5,
                            'query': {'field': ['fieldname', 'is', '10']}}},
                         })

        qm = operator.div(q, 2)
        qm.results
        self.check_target(target,
                         {
                          'query': {'scale': {'factor': 0.5,
                            'query': {'field': ['fieldname', 'is', '10']}}},
                         })

        qm = operator.truediv(q, 2)
        qm.results
        self.check_target(target,
                         {
                          'query': {'scale': {'factor': 0.5,
                            'query': {'field': ['fieldname', 'is', '10']}}},
                         })

    def test_slices(self):
        target = DummyTarget()
        q = query.QueryField("fieldname", "is", "10", target)

        def chk(target, from_=None, size=None, check_at_least=None):
            expected = { 'query': {'field': ['fieldname', 'is', '10']} }
            if from_ is not None:
                expected['from'] = from_
            if size is not None:
                expected['size'] = size
            if check_at_least is not None:
                expected['check_at_least'] = check_at_least
            self.check_target(target, expected)

        q.check_at_least(7).results
        chk(target, check_at_least=7)


        # Check slices with each end set or not set.
        q[10:20].results
        chk(target, from_=10, size=10)

        q[10:].results
        chk(target, from_=10)

        q[:10].results
        chk(target, from_=0, size=10)

        q[:].results
        chk(target, from_=0)


        # Check all types of subslice for a slice with stat and end set.
        q[10:20][3:5].results
        chk(target, from_=13, size=2)

        q[10:20][3:10].results
        chk(target, from_=13, size=7)

        q[10:20][3:15].results
        chk(target, from_=13, size=7)

        q[10:20][3:].results
        chk(target, from_=13, size=7)

        q[10:20][:5].results
        chk(target, from_=10, size=5)

        q[10:20][:10].results
        chk(target, from_=10, size=10)

        q[10:20][:20].results
        chk(target, from_=10, size=10)

        q[10:20][:].results
        chk(target, from_=10, size=10)


        # Check all types of subslice for a slice with only the start set.
        q[10:][3:5].results
        chk(target, from_=13, size=2)

        q[10:][3:10].results
        chk(target, from_=13, size=7)

        q[10:][3:15].results
        chk(target, from_=13, size=12)

        q[10:][3:].results
        chk(target, from_=13)

        q[10:][:5].results
        chk(target, from_=10, size=5)

        q[10:][:10].results
        chk(target, from_=10, size=10)

        q[10:][:15].results
        chk(target, from_=10, size=15)

        q[10:][:20].results
        chk(target, from_=10, size=20)

        q[10:][:].results
        chk(target, from_=10)


        # Check all types of subslice for a slice with only the end set.
        q[:10][3:5].results
        chk(target, from_=3, size=2)

        q[:10][3:10].results
        chk(target, from_=3, size=7)

        q[:10][3:15].results
        chk(target, from_=3, size=7)

        q[:10][3:].results
        chk(target, from_=3, size=7)

        q[:10][:5].results
        chk(target, from_=0, size=5)

        q[:10][:10].results
        chk(target, from_=0, size=10)

        q[:10][:15].results
        chk(target, from_=0, size=10)

        q[:10][:20].results
        chk(target, from_=0, size=10)

        q[:10][:].results
        chk(target, from_=0, size=10)


        # Check all types of subslice for a slice with neither start or end
        # set.
        q[:][10:20].results
        chk(target, from_=10, size=10)

        q[:][10:].results
        chk(target, from_=10)

        q[:][:10].results
        chk(target, from_=0, size=10)

        q[:][:].results
        chk(target, from_=0)
