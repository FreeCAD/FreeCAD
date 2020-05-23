import unittest

import FreeCAD


def load_tests(loader, tests, pattern):
    if FreeCAD.GuiUp:
        loader = unittest.defaultTestLoader
        return loader.discover("femtest.gui")
    return unittest.TestSuite()


def cover_tests():
    return [
        "femcommands",
        "femguiobjects"]
