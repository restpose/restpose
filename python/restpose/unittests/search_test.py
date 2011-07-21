# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from .helpers import RestPoseTestCase
from .. import query, Server
from ..resource import RestPoseResource
from ..query import Query
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

    expected_item_data =  {
        'cat': ['greeting'],
        'empty': [''],
        'id': ['1'],
        'tag': ['A tag'],
        'text': ['Hello world'],
        'type': ['blurb'],
    }

    # Expected items for tests which return a single result
    expected_items_single = [
        query.SearchResult(rank=0, data=expected_item_data),
    ]

    def check_results(self, results, offset=0, size_requested=None, check_at_least=0,
                      matches_lower_bound=None,
                      matches_estimated=None,
                      matches_upper_bound=None,
                      items = [],
                      info = []):
        if size_requested is None:
            size_requested = 10 # server default
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
        coll.delete()
        coll.checkpoint().wait()
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
                                      '\\tblurb\\t1': {},
                                      '!\\tblurb': {},
                                      '#\\tFcat': {},
                                      '#\\tFempty': {},
                                      '#\\tFtag': {},
                                      '#\\tFtext': {},
                                      '#\\tM': {},
                                      '#\\tMempty': {},
                                      '#\\tN': {},
                                      '#\\tNcat': {},
                                      '#\\tNtag': {},
                                      '#\\tNtext': {},
                                      'Zt\\thello': {'wdf': 1},
                                      'Zt\\tworld': {'wdf': 1},
                                      'c\\tCgreeting': {},
                                      'g\\tA tag': {},
                                      't\\thello': {'positions': [1], 'wdf': 1},
                                      't\\tworld': {'positions': [2], 'wdf': 1},
                                      })
        self.assertEqual(gotdoc.values, {})

    def test_field_is(self):
        q = self.coll.doc_type("blurb").field_is('tag', 'A tag')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

    def test_field_exists(self):
        q = self.coll.doc_type("blurb").field_exists()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_exists('tag')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_exists('id')
        # ID field is not stored, so searching for its existence returns
        # nothing.
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_exists('type')
        # Type field is not stored, so searching for its existence returns
        # nothing.
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_exists('missing')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

    def test_field_empty(self):
        q = self.coll.doc_type("blurb").field_empty()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_empty('empty')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_empty('text')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_empty('id')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_empty('type')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_empty('missing')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

    def test_field_nonempty(self):
        q = self.coll.doc_type("blurb").field_nonempty()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_nonempty('empty')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_nonempty('text')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

        q = self.coll.doc_type("blurb").field_nonempty('id')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_nonempty('type')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

        q = self.coll.doc_type("blurb").field_nonempty('missing')
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

    def test_field_has_error(self):
        q = self.coll.doc_type("blurb").field_has_error()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

    def test_query_all(self):
        q = self.coll.doc_type("blurb").query_all()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

    def test_query_none(self):
        q = self.coll.doc_type("blurb").query_none()
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=[])

    def test_calc_cooccur(self):
        q = self.coll.doc_type("blurb").query_all()
        results = self.coll.doc_type("blurb").search(q.calc_cooccur('t'))
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
        results = self.coll.doc_type("blurb").search(q.calc_occur('t'))
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

        # if size isn't specified in the query, uses the server's page size,
        # which is 10.
        self.check_results(results, items=self.expected_items_single,
                           size_requested=10)

    def test_query_adjust_offset(self):
        """Test adjusting the configured offset for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        results = self.coll.doc_type("blurb").search(q[1:])
        self.check_results(results, offset=1,
                           matches_lower_bound=1,
                           matches_estimated=1,
                           matches_upper_bound=1,
                           items=[])
        # Check that the adjustment didn't change the original search.
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

    def test_query_adjust_size(self):
        """Test adjusting the configured size for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        results = self.coll.doc_type("blurb").search(q[:1])
        self.check_results(results, size_requested=1,
                           items=self.expected_items_single)
        # Check that the adjustment didn't change the original search.
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

    def test_query_adjust_check_at_least(self):
        """Test adjusting the configured check_at_least value for a search.

        """
        q = self.coll.doc_type("blurb").query_all()
        results = self.coll.doc_type("blurb").search(q.check_at_least(1))
        self.check_results(results, check_at_least=1,
                           items=self.expected_items_single)
        # Check that the adjustment didn't change the original search.
        results = self.coll.doc_type("blurb").search(q)
        self.check_results(results, items=self.expected_items_single)

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
        empty_type = coll.doc_type("empty_type")
        missing_type = coll.doc_type("missing_type")
        empty_query = empty_type.query_all()
        missing_query = missing_type.query_all()

        self.assertEqual(empty_type.search(empty_query[7:18]
                                           .check_at_least(3))._raw,
                         missing_type.search(missing_query[7:18]
                                             .check_at_least(3))._raw)

        self.assertEqual(empty_type.search(empty_query[7:18]
                                           .check_at_least(3)
                                           .calc_cooccur('t'))._raw,
                         missing_type.search(missing_query[7:18]
                                             .check_at_least(3)
                                             .calc_cooccur('t'))._raw)

    def test_query_subscript(self):
        """Test subscript on a query.

        """
        q = self.coll.doc_type("blurb").query_all()
        self.assertEqual(q[0].data, self.expected_item_data)


class LargeSearchTest(RestPoseTestCase):
    """Tests of handling results of searches which return lots of results.

    """
    maxDiff = 10000

    @staticmethod
    def make_doc(num):
        return { 'num': [num], 'id': [str(num)], 'type': ['num'] }

    @classmethod
    def setup_class(cls):
        coll = Server().collection("test_coll")
        coll.delete()
        coll.checkpoint().wait()
        for num in xrange(193):
            doc = cls.make_doc(num)
            coll.add_doc(doc, doc_type="num", doc_id=str(num))
        chk = coll.checkpoint().wait()
        assert chk.total_errors == 0

    def setUp(self):
        self.coll = Server().collection("test_coll")

    def test_indexed_ok(self):
        """Check that setup put the database into the desired state.

        """
        self.assertEqual(self.coll.status.get('doc_count'), 193)
        gotdoc = self.coll.get_doc("num", "77")
        self.assertEqual(gotdoc.data, {
                                      'num': [77],
                                      'id': ['77'],
                                      'type': ['num'],
                                      })
        self.assertEqual(gotdoc.terms, {
                                      '\\tnum\\t77': {},
                                      '!\\tnum': {},
                                      '#\\tFnum': {},
                                      '#\\tN': {},
                                      '#\\tNnum': {},
                                      })
        self.assertEqual(gotdoc.values, { '268435599': '\\xb8\\xd0' })

    def test_query_empty(self):
        q = self.coll.doc_type("num").query_none()
        self.assertRaises(IndexError, q.__getitem__, 0)

    def test_query_order_by_field(self):
        """Test setting the result order to be by a field.

        """
        q = self.coll.doc_type("num").query_all()
        q1 = q.order_by('num') # default order is ascending
        self.assertEqual(q1[0].data, self.make_doc(0))
        self.assertEqual(q1[192].data, self.make_doc(192))
        q1 = q.order_by('num', True) 
        self.assertEqual(q1[0].data, self.make_doc(0))
        self.assertEqual(q1[192].data, self.make_doc(192))
        q1 = q.order_by('num', False) 
        self.assertEqual(q1[0].data, self.make_doc(192))
        self.assertEqual(q1[192].data, self.make_doc(0))

    def test_query_all(self):
        q = self.coll.doc_type("num").query_all().order_by('num')

        self.assertEqual(q[0].data, self.make_doc(0))
        self.assertEqual(q[1].data, self.make_doc(1))
        self.assertEqual(q[50].data, self.make_doc(50))
        self.assertEqual(q[192].data, self.make_doc(192))
        self.assertRaises(IndexError, q.__getitem__, 193)
        self.assertEqual(len(q), 193)

        qs = q[10:20]
        self.assertEqual(qs[0].data, self.make_doc(10))
        self.assertEqual(qs[1].data, self.make_doc(11))
        self.assertEqual(qs[9].data, self.make_doc(19))
        self.assertRaises(IndexError, qs.__getitem__, 10)
        self.assertRaises(IndexError, qs.__getitem__, -1)
        self.assertEqual(len(qs), 10)

        qs = q[190:200]
        self.assertEqual(qs[0].data, self.make_doc(190))
        self.assertEqual(qs[1].data, self.make_doc(191))
        self.assertEqual(qs[2].data, self.make_doc(192))
        self.assertRaises(IndexError, qs.__getitem__, 3)
        self.assertRaises(IndexError, qs.__getitem__, -1)
        self.assertEqual(len(qs), 3)

        qs = q[15:]
        self.assertEqual(qs[0].data, self.make_doc(15))
        self.assertEqual(qs[50 - 15].data, self.make_doc(50))
        self.assertEqual(qs[192 - 15].data, self.make_doc(192))
        self.assertRaises(IndexError, qs.__getitem__, 193 - 15)
        self.assertEqual(len(qs), 193 - 15)
