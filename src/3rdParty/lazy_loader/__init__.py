"""
LazyLoader will defer import of a module until first usage. Usage:
from lazy_loader.lazy_loader import LazyLoader
numpy = LazyLoader("numpy", globals(), "numpy")

or

whatever = LazyLoader("module", globals(), "module.whatever")

or to replicate import module as something

something = LazyLoader("module", globals(), "module")
"""