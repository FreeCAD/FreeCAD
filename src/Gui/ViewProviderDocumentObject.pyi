# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from ViewProvider import ViewProvider
from typing import TYPE_CHECKING, Final

if TYPE_CHECKING:
    from FreeCAD import DocumentObject as _DocumentObject

class ViewProviderDocumentObject(ViewProvider):
    """
    This is the ViewProvider base class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    def update(self) -> None:
        """
        Update the view representation of the object
        """
        ...
    Object: _DocumentObject = ...
    """Set/Get the associated data object"""

    ForceUpdate: bool = False
    """Reference count to force update visual"""

    Document: Final["Document"] = ...
    """Return the document the view provider is part of"""
