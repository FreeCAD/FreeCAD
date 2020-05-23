import unittest
import FreeCAD as App


class BaseTest(unittest.TestCase):

    def assertSameElements(self, a, b):
        first = set(a)
        second = set(a)
        if (first != second):
            msg = "{} != {}".format(first, second)
            raise AssertionError(msg)


class DocumentTest(BaseTest):

    def setUp(self):
        self.doc = App.newDocument()

    def tearDown(self):
        App.closeDocument(self.doc.Name)
