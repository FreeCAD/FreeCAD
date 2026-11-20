# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from . import TestBlendExport, TestBlendImport, TestBRep, TestTopology, TestReview, TestCurvature


def suite():
    result = unittest.TestSuite()
    result.addTests(TestBlendExport.suite())
    result.addTests(TestBlendImport.suite())
    result.addTests(TestTopology.suite())
    result.addTests(TestBRep.suite())
    result.addTests(TestReview.suite())
    result.addTests(TestCurvature.suite())
    return result


def load_tests(loader, tests, pattern):
    """Expose the package suite to ``unittest`` name loading.

    FreeCAD's test runner calls ``loadTestsFromName("FormsTests")``, which
    only honours a ``load_tests`` hook, not a ``suite()`` function.
    """
    return suite()
