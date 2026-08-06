"""
Headless smoke test runner for Part boolean feature tests.
Run via: pixi run smoke-test-boolean
"""

import sys
import unittest

sys.path.insert(0, "src/Mod/Part")

from parttests.BooleanFeatureTest import TestBooleanFeatures

suite = unittest.TestLoader().loadTestsFromTestCase(TestBooleanFeatures)
runner = unittest.TextTestRunner(verbosity=2)
result = runner.run(suite)
sys.exit(0 if result.wasSuccessful() else 1)
