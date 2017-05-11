import unittest

def runTest(test_case, verbosity=2):
    suite = unittest.TestLoader().loadTestsFromTestCase(test_case)
    unittest.TextTestRunner(verbosity=verbosity).run(suite)
