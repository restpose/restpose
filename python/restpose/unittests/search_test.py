# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from .helpers import RestPoseTestCase
from .. import query, Server
from ..resource import RestPoseResource
from .. import ResourceNotFound
from restkit import ResourceError

class LogResource(RestPoseResource):
    """A simple resource which logs requests.

    This could be tidied up and made into a useful debugging tool in future,
    but for now it's just used to test supplying a custom resource instance to
    the Server() class.

    """
    def __init__(self, *args, **kwargs):
        super(LogResource, self).__init__(*args, **kwargs)
        self.log = []

    def request(self, method, path=None, *args, **kwargs):
        try:
            res = super(LogResource, self).request(method, path=path,
                                                   *args, **kwargs)
        except ResourceError, e:
            self.log.append("%s: %s -> %d %s" % (method, path, e.status_int, e))
            raise
        self.log.append("%s: %s -> %d" % (method, path, int(res.status[:3])))
        return res

    def clone(self):
        res = super(LogResource, self).clone()
        res.log = self.log
        return res


class SearchTest(RestPoseTestCase):
    maxDiff = 10000

    # Expected items for tests which return a single result
    expected_items_single = [
        query.SearchResult(rank=0, data={
                'cat': ['greeting'],
                'empty': [''],
                'id': ['1'],
                'tag': ['A tag'],
                'text': ['Hello world'],
                'type': ['blurb'],
            }),
        ]

    def check_results(self, results, offset=0, size_requested=10, check_at_least=0,
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
        self.assertEqual(results.size_requested, size_requested)
        self.assertEqual(results.check_at_least, check_at_least)
        self.assertEqual(results.matches_lower_bound, matches_lower_bound)
        self.assertEqual(results.matches_estimated, matches_estimated)
        self.assertEqual(results.matches_upper_bound, matches_upper_bound)
        self.assertEqual(len(results.items), len(items))
        for i in xrange(len(items)):
            self.assertEqual(results.items[i].rank, items[i].rank)
            self.assertEqual(results.items[i].data, items[i].data)
        self.assertEqual(results.info, info)

    @classmethod
    def setup_class(cls):
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll = Server().collection("test_coll")
        #coll.delete()
        coll.add_doc(doc, doc_type="blurb", doc_id="1")
        chk = coll.checkpoint().wait()
        assert chk.total_errors == 0

    def setUp(self):
        self.coll = Server().collection("test_coll")

    def test_indexed_ok(self):
        """Check that setup put the database into the desired state.

        """
        self.assertEqual(self.coll.status.get('doc_count'), 1)
        gotdoc = self.coll.get_doc("blurb", "1")
        self.assertEqual(gotdoc.data, {
                                      'cat': ['greeting'],
                                      'empty': [''],
                                      'id': ['1'],
                                      'tag': ['A tag'],
                                      'text': ['Hello world'],
                                      'type': ['blurb'],
                                      })
        self.assertEqual(gotdoc.terms, {
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
                                      })
        self.assertEqual(gotdoc.values, {})

    def test_field_is(self):
        q = self.coll.doc_type("blurb").field_is('tag', 'A tag')
        results = q.search().do()
        self.check_results(results, items=self.expected_items_single)

    def test_field_exists(self):
        q = self.coll.doc_type("blurb").field_exists()
        results = q.search().do()
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_exists('tag')
        results = q.search().do()
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_exists('id')
        # ID field is not stored, so searching for its existence returns
        # nothing.
        results = q.search().do()
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_exists('type')
        # Type field is not stored, so searching for its existence returns
        # nothing.
        results = q.search().do()
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_exists('missing')
        results = q.search().do()
        self.check_results(results, items=[])

    def test_field_empty(self):
        q = self.coll.doc_type("blurb").field_empty()
        self.check_results(q.search().do(), items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_empty('empty')
        self.check_results(q.search().do(), items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_empty('text')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_empty('id')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_empty('type')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_empty('missing')
        self.check_results(q.search().do(), items=[])

    def test_field_nonempty(self):
        q = self.coll.doc_type("blurb").field_nonempty()
        self.check_results(q.search().do(), items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_nonempty('empty')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_nonempty('text')
        self.check_results(q.search().do(), items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_nonempty('id')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_nonempty('type')
        self.check_results(q.search().do(), items=[])

        q = self.coll.doc_type("blurb").field_nonempty('missing')
        self.check_results(q.search().do(), items=[])

    def test_field_has_error(self):
        q = self.coll.doc_type("blurb").field_has_error()
        self.check_results(q.search().do(), items=[])

    def test_query_all(self):
        q = self.coll.doc_type("blurb").query_all()
        self.check_results(q.search().do(), items=self.expected_items_single)

    def test_query_none(self):
        q = self.coll.doc_type("blurb").query_none()
        self.check_results(q.search().do(), items=[])

    def test_calc_cooccur(self):
        q = self.coll.doc_type("blurb").query_all()
        s = q.search()
        results = q.search().calc_cooccur('t').do()
        self.assertEqual(self.coll.status.get('doc_count'), 1)
        self.check_results(results, check_at_least=1,
                           items=self.expected_items_single,
                           info=[{
                               'counts': [['hello', 'world', 1]],
                               'docs_seen': 1,
                               'terms_seen': 2,
                               'prefix': 't',
                               'type': 'cooccur'
                           }])

    def test_calc_occur(self):
        q = self.coll.doc_type("blurb").query_all()
        s = q.search()
        results = q.search().calc_occur('t').do()
        self.assertEqual(self.coll.status.get('doc_count'), 1)
        self.check_results(results, check_at_least=1,
                           items=self.expected_items_single,
                           info=[{
                               'counts': [['hello', 1], ['world', 1]],
                               'docs_seen': 1,
                               'terms_seen': 2,
                               'prefix': 't',
                               'type': 'occur'
                           }])

    def test_raw_query(self):
        results = self.coll.doc_type("blurb").search(dict(query=dict(matchall=True)))
        self.check_results(results, items=self.expected_items_single)

    def test_query_adjust_offset(self):
        """Test adjusting the configured offset for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        search = q.search()
        self.check_results(search.offset(1).do(), offset=1,
                           matches_lower_bound=1,
                           matches_estimated=1,
                           matches_upper_bound=1,
                           items=[])
        # Check that the adjustment didn't change the original search.
        self.check_results(search.do(), items=self.expected_items_single)

    def test_query_adjust_size(self):
        """Test adjusting the configured size for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        search = q.search()
        self.check_results(search.size(1).do(), size_requested=1,
                           items=self.expected_items_single)
        # Check that the adjustment didn't change the original search.
        self.check_results(search.do(), items=self.expected_items_single)

    def test_query_adjust_check_at_least(self):
        """Test adjusting the configured check_at_least value for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        search = q.search()
        self.check_results(search.check_at_least(1).do(), check_at_least=1,
                           items=self.expected_items_single)
        # Check that the adjustment didn't change the original search.
        self.check_results(search.do(), items=self.expected_items_single)

    def test_resource_log_search(self):
        "Test runnning a search, using a special resource to log requests."
        # The URI is required, but will be replaced by that passed to Server()
        logres = LogResource(uri='http://127.0.0.1:7777/')

        coll = Server(resource_instance=logres).collection("test_coll")
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, doc_type="blurb", doc_id="1")
        self.wait(coll)
        self.assertTrue(len(logres.log) >= 3)
        self.assertEqual(logres.log[0],
                         'PUT: /coll/test_coll/type/blurb/id/1 -> 202')
        self.assertEqual(logres.log[1],
                         'POST: /coll/test_coll/checkpoint -> 201')
        self.assertEqual(logres.log[2][:32],
                         'GET: /coll/test_coll/checkpoint/')
        self.assertEqual(logres.log[2][-6:], '-> 200')

        logres.log[:] = []
        doc = coll.doc_type('blurb').get_doc('2')
        self.assertRaises(ResourceNotFound, getattr, doc, 'data')
        self.assertEqual(logres.log,
                         ['GET: /coll/test_coll/type/blurb/id/2 -> ' +
                          '404 No document found of type "blurb" and id "2"'])

    def test_search_on_unknown_type(self):
        """Test that a search on an unknown document type returns exactly the
        same results as a search on a known document type.

        """
        coll = Server().collection("test_coll")
        coll.add_doc({}, doc_type="empty_type", doc_id="1")
        coll.delete_doc(doc_type="empty_type", doc_id="1")
        self.wait(coll)
        empty_query = coll.doc_type("empty_type").query_all()
        missing_query = coll.doc_type("missing_type").query_all()

        self.assertEqual(empty_query
                         .search(offset=7, size=11, check_at_least=3)
                         .do()._raw,
                         missing_query
                         .search(offset=7, size=11, check_at_least=3)
                         .do()._raw)

        self.assertEqual(empty_query
                         .search(offset=7, size=11, check_at_least=3)
                         .calc_cooccur('t')
                         .do()._raw,
                         missing_query
                         .search(offset=7, size=11, check_at_least=3)
                         .calc_cooccur('t')
                         .do()._raw)
