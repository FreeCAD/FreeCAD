import unittest


def runTestsFromClass(test_case_class, verbosity=2):
    suite = unittest.TestLoader().loadTestsFromTestCase(test_case_class)
    unittest.TextTestRunner(verbosity=verbosity).run(suite)


def runTestsFromModule(test_module, verbosity=2):
    suite = unittest.TestLoader().loadTestsFromModule(test_module)
    unittest.TextTestRunner(verbosity=verbosity).run(suite)
