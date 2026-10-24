# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Final
from Base.PyObjectBase import PyObjectBase
from ExtensionContainer import ExtensionContainer


class Extension(PyObjectBase):
    """
    Base class for all extensions
    Author: Stefan Troeger (stefantroeger@gmx.net)
    Licence: LGPL
    """

    ExtendedObject: Final[ExtensionContainer]
    """Get extended container object"""
