# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import query, Server, ResourceNotFound

class SearchTest(TestCase):
    def check_results(self, results, offset=0, size=10, check_at_least=0,
                      matches_lower_bound=None,
                      matches_estimated=None,
                      matches_upper_bound=None,
                      items = [],
                      info = {}):
        if matches_lower_bound is None:
            matches_lower_bound = len(items)
        if matches_estimated is None:
            matches_estimated = len(items)
        if matches_upper_bound is None:
            matches_upper_bound = len(items)

        self.assertEqual(results.offset, offset)
        self.assertEqual(results.size, size)
        self.assertEqual(results.check_at_least, check_at_least)
        self.assertEqual(results.matches_lower_bound, matches_lower_bound)
        self.assertEqual(results.matches_estimated, matches_estimated)
        self.assertEqual(results.matches_upper_bound, matches_upper_bound)
        self.assertEqual(len(results.items), len(items))
        for i in xrange(len(items)):
            self.assertEqual(results.items[i].rank, items[i].rank)
            self.assertEqual(results.items[i].fields, items[i].fields)
        self.assertEqual(results.info, info)

    maxDiff = 10000
    def test_basic_search(self):
        server = Server()
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll = server.collection("test_coll")
        #coll.delete()
        coll.checkpoint().wait()
        #self.assertEqual(coll.status.get('doc_count'), 0)

        msg = None
        try:
            coll.get_doc("blurb", "2")
        except ResourceNotFound, e:
            msg = e.msg
        self.assertTrue(e.msg == 'No collection of name "test_coll" exists' or
                        e.msg, 'No document found of type "blurb" and id "2"')
        coll.add_doc(doc, type="blurb", docid="1")
        coll.checkpoint().wait()
        gotdoc = coll.get_doc("blurb", "1")
        self.assertEqual(gotdoc, dict(data={
                                      'cat': ['greeting'],
                                      'empty': [''],
                                      'id': ['1'],
                                      'tag': ['A tag'],
                                      'text': ['Hello world'],
                                      'type': ['blurb'],
                                      }, terms={
                                      '\tblurb\t1': {},
                                      '!\tblurb': {},
                                      '#\tFcat': {},
                                      '#\tFempty': {},
                                      '#\tFtag': {},
                                      '#\tFtext': {},
                                      '#\tM': {},
                                      '#\tMempty': {},
                                      '#\tN': {},
                                      '#\tNcat': {},
                                      '#\tNtag': {},
                                      '#\tNtext': {},
                                      'Zt\thello': {'wdf': 1},
                                      'Zt\tworld': {'wdf': 1},
                                      'c\tCgreeting': {},
                                      'g\tA tag': {},
                                      't\thello': {'positions': [1], 'wdf': 1},
                                      't\tworld': {'positions': [2], 'wdf': 1},
                                      }))
        q = coll.type("blurb").field_is('tag', 'A tag')
        results = q.search().do()
        self.assertEqual(coll.status.get('doc_count'), 1)
        expected_items = [
            query.SearchResult(rank=0, fields={
                    'cat': ['greeting'],
                    'empty': [''],
                    'id': ['1'],
                    'tag': ['A tag'],
                    'text': ['Hello world'],
                    'type': ['blurb'],
                }),
            ]
        self.check_results(results, items=expected_items)


        q = coll.type("blurb").field_exists()
        results = q.search().do()
        self.check_results(results, items=expected_items)

        q = coll.type("blurb").field_exists('tag')
        results = q.search().do()
        self.check_results(results, items=expected_items)

        q = coll.type("blurb").field_exists('id')
        # ID field is not stored, so searching for its existence returns
        # nothing.
        results = q.search().do()
        self.check_results(results, items=[])

        q = coll.type("blurb").field_exists('type')
        # Type field is not stored, so searching for its existence returns
        # nothing.
        results = q.search().do()
        self.check_results(results, items=[])

        q = coll.type("blurb").field_exists('missing')
        results = q.search().do()
        self.check_results(results, items=[])


        q = coll.type("blurb").field_empty()
        self.check_results(q.search().do(), items=expected_items)

        q = coll.type("blurb").field_empty('empty')
        self.check_results(q.search().do(), items=expected_items)

        q = coll.type("blurb").field_empty('text')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_empty('id')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_empty('type')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_empty('missing')
        self.check_results(q.search().do(), items=[])


        q = coll.type("blurb").field_nonempty()
        self.check_results(q.search().do(), items=expected_items)

        q = coll.type("blurb").field_nonempty('empty')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_nonempty('text')
        self.check_results(q.search().do(), items=expected_items)

        q = coll.type("blurb").field_nonempty('id')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_nonempty('type')
        self.check_results(q.search().do(), items=[])

        q = coll.type("blurb").field_nonempty('missing')
        self.check_results(q.search().do(), items=[])


        q = coll.type("blurb").field_has_error()
        self.check_results(q.search().do(), items=[])


        q = coll.type("blurb").query_all()
        s = q.search()
        results = q.search().calc_cooccur('t').do()
        self.assertEqual(coll.status.get('doc_count'), 1)
        self.check_results(results, check_at_least=1,
                           items=expected_items,
                           info=[{
                               'counts': [['hello', 'world', 1]],
                               'docs_seen': 1,
                               'terms_seen': 2,
                               'prefix': 't',
                               'type': 'cooccur'
                           }])

        s = q.search()
        results = q.search().calc_occur('t').do()
        self.assertEqual(coll.status.get('doc_count'), 1)
        self.check_results(results, check_at_least=1,
                           items=expected_items,
                           info=[{
                               'counts': [['hello', 1], ['world', 1]],
                               'docs_seen': 1,
                               'terms_seen': 2,
                               'prefix': 't',
                               'type': 'occur'
                           }])
