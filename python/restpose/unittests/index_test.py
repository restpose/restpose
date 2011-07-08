# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import Server, ResourceNotFound

class IndexTest(TestCase):

    def wait(self, coll):
        chk = coll.checkpoint().wait()
        self.assertEqual(chk.errors, [])
        self.assertEqual(chk.total_errors, 0)
        self.assertTrue(chk.reached)
        self.assertFalse(chk.expired)

    def test_delete_db(self):
        """Test that deleting a database functions correctly.

        """
        coll = Server().collection("test_coll")
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, type="blurb", id="1")
        self.wait(coll)
        coll.get_doc("blurb", "1")
        self.assertEqual(coll.get_doc("blurb", "1").get('data'),
                         dict(
                              cat = ['greeting'],
                              empty = [''],
                              id = ['1'],
                              tag = ['A tag'],
                              text = ['Hello world'],
                              type = ['blurb'],
                             ))


        return # delete not yet implemented!
        coll.delete()
        self.wait(coll)
        msg = None
        try:
            coll.get_doc("blurb", "1")
        except ResourceNotFound, e:
            msg = e.msg
        self.assertEqual(msg, 'No collection of name "test_coll" exists')

    def test_delete_doc(self):
        """Test that deleting a document functions correctly.

        """
        coll = Server().collection("test_coll")
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, type="blurb", id="1")
        self.wait(coll)
        coll.get_doc("blurb", "1")
        self.assertEqual(coll.get_doc("blurb", "1").get('data'),
                         dict(
                              cat = ['greeting'],
                              empty = [''],
                              id = ['1'],
                              tag = ['A tag'],
                              text = ['Hello world'],
                              type = ['blurb'],
                             ))

        return # delete not yet implemented!
        coll.delete_doc(type="blurb", id="1")
        self.wait(coll)
        msg = None
        try:
            coll.get_doc("blurb", "1")
        except ResourceNotFound, e:
            msg = e.msg
        self.assertEqual(msg, 'No collection of name "test_coll" exists')
