from __future__ import print_function
import unittest
from pivy.qt import QtCore
import shiboken2 as wrapper


class ShibokenTests(unittest.TestCase):
    """shiboken_64bit_test.py"""

    def testAdresses(self):
        q = QtCore.QObject()
        ptr = wrapper.getCppPointer(q)
        print("CppPointer to an instance of PySide.QtCore.QObject = 0x%016X" % ptr[0])

        # None of the following is expected to raise an
        # OverflowError on 64-bit systems

        # largest 32-bit address
        wrapper.wrapInstance(0xFFFFFFFF, QtCore.QObject)

        # a regular, slightly smaller 32-bit address
        wrapper.wrapInstance(0xFFFFFFF, QtCore.QObject)

        # an actual 64-bit address (> 4 GB, the first non 32-bit address)
        wrapper.wrapInstance(0x100000000, QtCore.QObject)

        # largest 64-bit address
        wrapper.wrapInstance(0xFFFFFFFFFFFFFFFF, QtCore.QObject)


if __name__ == "__main__":
    unittest.main(verbosity=4)
