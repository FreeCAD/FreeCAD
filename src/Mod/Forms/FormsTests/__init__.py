# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from . import TestBlendExport, TestBlendImport, TestBRep, TestTopology, TestReview


def suite():
    result = unittest.TestSuite()
    result.addTests(TestBlendExport.suite())
    result.addTests(TestBlendImport.suite())
    result.addTests(TestTopology.suite())
    result.addTests(TestBRep.suite())
    result.addTests(TestReview.suite())
    return result
