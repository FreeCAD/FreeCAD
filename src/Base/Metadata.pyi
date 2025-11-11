# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

"""
This file keeps auxiliary metadata to be used by the Python API stubs.
"""

def export(**kwargs):
    """
    A decorator to attach metadata to a class.
    """
    ...

def constmethod(method): ...
def no_args(method): ...
def forward_declarations(source_code):
    """
    A decorator to attach forward declarations to a class.
    """
    ...

def class_declarations(source_code):
    """
    A decorator to attach forward declarations to a class.
    """
    ...

def sequence_protocol(**kwargs):
    """
    A decorator to attach sequence protocol metadata to a class.
    """
    ...
