# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import query, Server, ResourceNotFound

class SearchTest(TestCase):
    def check_results(self, results, offset=0, size=10, checkatleast=0,
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
        self.assertEqual(results.checkatleast, checkatleast)
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
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting" }
        coll = server.collection("test_coll")
        #coll.delete()
        coll.checkpoint().wait()
        #self.assertEqual(coll.status.get('doc_count'), 0)

        try:
            coll.get_doc("blurb", "2")
            self.fail()
        except ResourceNotFound, e:
            if e.msg != 'No collection of name "test_coll" exists':
                self.assertEqual(e.msg, 'No document found of type "blurb" and id "2"')
        coll.add_doc(doc, type="blurb", docid="1")
        coll.checkpoint().wait()
        gotdoc = coll.get_doc("blurb", "1")
        self.assertEqual(gotdoc, dict(data={
                                      'cat': ['greeting'],
                                      'id': ['1'],
                                      'tag': ['A tag'],
                                      'text': ['Hello world'],
                                      'type': ['blurb'],
                                      }, terms={
                                      '\tblurb\t1': {},
                                      '!\tblurb': {},
                                      'Zt\thello': {'wdf': 1},
                                      'Zt\tworld': {'wdf': 1},
                                      'c\tCgreeting': {},
                                      'g\tA tag': {},
                                      't\thello': {'positions': [1], 'wdf': 1},
                                      't\tworld': {'positions': [2], 'wdf': 1},
                                      }))
        q = coll.type("blurb").field_is('tag', 'A tag')
        q = coll.type("blurb").query_all()
        results = q.search().do()
        self.assertEqual(coll.status.get('doc_count'), 1)
        self.check_results(results, items=[
            query.SearchResult(rank=0, fields={
                    'cat': ['greeting'],
                    'id': ['1'],
                    'tag': ['A tag'],
                    'text': ['Hello world'],
                    'type': ['blurb'],
                }),
            ]
        )

        s = q.search()
        results = q.search().calc_cooccur('t').do()
        self.assertEqual(coll.status.get('doc_count'), 1)
        self.check_results(results, checkatleast=1,
                           items=[
                           query.SearchResult(rank=0, fields={
                                              'cat': ['greeting'],
                                              'id': ['1'],
                                              'tag': ['A tag'],
                                              'text': ['Hello world'],
                                              'type': ['blurb'],
                                              }),
                           ],
                           info=[{
                               'counts': [['hello', 'world', 1]],
                               'docs_seen': 1,
                               'terms_seen': 2,
                               'prefix': 't',
                               'type': 'cooccur'
                           }])
