# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from .helpers import RestPoseTestCase
from .. import Server, ResourceNotFound

class IndexTest(RestPoseTestCase):

    def test_delete_db(self):
        """Test that deleting a database functions correctly.

        """
        coll = Server().collection("test_coll")
        coll.delete()
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, doc_type="blurb", doc_id="1")
        self.wait(coll)
        self.assertEqual(coll.get_doc("blurb", "1").data,
                         dict(
                              cat = ['greeting'],
                              empty = [''],
                              id = ['1'],
                              tag = ['A tag'],
                              text = ['Hello world'],
                              type = ['blurb'],
                             ))

        self.assertTrue('test_coll' in Server().collections)

        coll.delete()

        # Need to set commit=False, or the checkpoint re-creates the
        # collection.
        self.wait(coll, commit=False)
        self.assertTrue('test_coll' not in Server().collections)
        msg = None
        try:
            coll.get_doc("blurb", "1").data
        except ResourceNotFound, e:
            msg = e.msg
        self.assertEqual(msg, 'No collection of name "test_coll" exists')

    def test_delete_doc(self):
        """Test that deleting a document functions correctly.

        """
        coll = Server().collection("test_coll")
        coll.delete()
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, doc_type="blurb", doc_id="1")
        self.wait(coll)
        self.assertEqual(coll.get_doc("blurb", "1").data,
                         dict(
                              cat = ['greeting'],
                              empty = [''],
                              id = ['1'],
                              tag = ['A tag'],
                              text = ['Hello world'],
                              type = ['blurb'],
                             ))

        coll.delete_doc(doc_type="blurb", doc_id="1")
        self.wait(coll)
        msg = None
        try:
            coll.get_doc("blurb", "1").data
        except ResourceNotFound, e:
            msg = e.msg
        self.assertEqual(msg, 'No document found of type "blurb" and id "1"')

    def test_custom_config(self):
        coll = Server().collection("test_coll")
        coll.delete()
        self.wait(coll)
        config = coll.config
        self.assertEqual(config['format'], 3)
        self.assertEqual(config['types'].keys(), [])

        coll.config = {'types': {'foo': {
            'alpha': {'type': 'text'}
        }}}
        self.wait(coll, [{'msg': 'Setting collection config failed with ' +
                  'RestPose::InvalidValueError: Member format was missing'}])
        self.assertEqual(config['types'].keys(), [])
        coll.config = {'format': 3,
            'types': {'foo': {
                'alpha': {'type': 'text'}
            }}
        }
        self.wait(coll)
        config = coll.config
        self.assertEqual(config['format'], 3)
        self.assertEqual(config['types'].keys(), ['foo'])
        coll.delete()
        self.wait(coll)
