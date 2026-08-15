# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from . import TestBRep, TestTopology


def suite():
    result = unittest.TestSuite()
    result.addTests(TestTopology.suite())
    result.addTests(TestBRep.suite())
    return result
