# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

from unittest import TestCase

class RestPoseTestCase(TestCase):

    def wait(self, coll, errors=None, commit=True):
        """Wait on a collection for a checkpoint to be reached.

        Checks that the errors in the checkpoint are as listed in errors.

        """
        chk = coll.checkpoint(commit=commit).wait()
        if errors is None:
            errors = []
        self.assertEqual(chk.errors, errors)
        self.assertEqual(chk.total_errors, len(errors))
        self.assertTrue(chk.reached)
