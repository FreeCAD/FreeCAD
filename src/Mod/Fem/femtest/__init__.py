import unittest

import femtest.app
import femtest.gui


def load_tests(loader, tests, pattern):
    suite = unittest.TestSuite()
    suite.addTests(femtest.app.load_tests(loader, tests, pattern))
    suite.addTests(femtest.gui.load_tests(loader, tests, pattern))
    return suite


def cover_tests():
    return (
        femtest.app.cover_tests() +
        femtest.gui.cover_tests())
