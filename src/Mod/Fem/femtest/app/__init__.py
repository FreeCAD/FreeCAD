import unittest


def load_tests(loader, tests, pattern):
    loader = unittest.defaultTestLoader
    return loader.discover("femtest.app")


def cover_tests():
    return [
        "feminout",
        "femmesh",
        "femobjects",
        "femresult",
        "femsolver",
        "femtools"]
