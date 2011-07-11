# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase
from .. import Server, ResourceNotFound

class IndexTest(TestCase):

    def wait(self, coll, errors=None):
        chk = coll.checkpoint().wait()
        if errors is None:
            errors = []
        self.assertEqual(chk.errors, errors)
        self.assertEqual(chk.total_errors, len(errors))
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
        self.assertEqual(coll.get_doc("blurb", "1").data,
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
        #coll.delete()
        doc = { 'text': 'Hello world', 'tag': 'A tag', 'cat': "greeting",
                'empty': "" }
        coll.add_doc(doc, type="blurb", id="1")
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

        coll.delete_doc(type="blurb", id="1")
        self.wait(coll)
        msg = None
        try:
            coll.get_doc("blurb", "1").data
        except ResourceNotFound, e:
            msg = e.msg
        self.assertEqual(msg, 'No document found of type "blurb" and id "1"')

    def test_custom_config(self):
        coll = Server().collection("test_coll")
        #coll.delete()
        coll.config = {'types': {'foo': {
            'alpha': {'type': 'text'}
        }}}
        self.wait(coll, [{'msg': 'Setting collection config failed with ' +
                  'RestPose::InvalidValueError: Member format was missing'}])
        coll.config = {'format': 3,
            'types': {'foo': {
                'alpha': {'type': 'text'}
            }}
        }
        self.wait(coll, [])
        config = coll.config
        self.assertEqual(config['format'], 3)
        #self.assertEqual(config['types'].keys(), ['foo'])
