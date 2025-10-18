# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Final, Any
from Base.PyObjectBase import PyObjectBase


class Extension(PyObjectBase):
    """
    Base class for all extensions
    Author: Stefan Troeger (stefantroeger@gmx.net)
    Licence: LGPL
    """

    ExtendedObject: Final[Any] = None
    """Get extended container object"""
