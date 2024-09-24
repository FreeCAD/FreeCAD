import os
from unittest import TestCase


def assertFilePathsEqual(self: TestCase, path1: os.PathLike, path2: os.PathLike):
    self.assertEqual(os.path.realpath(path1), os.path.realpath(path2))
