# SPDX-License-Identifier: LGPL-2.1-or-later

from App.DocumentObject import DocumentObject
from Gui.Document import Document as DocumentPy
from ViewProvider import ViewProvider
from typing import Final

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

    Object: DocumentObject
    """Set/Get the associated data object"""

    ForceUpdate: bool
    """Reference count to force update visual"""

    Document: Final[DocumentPy]
    """Return the document the view provider is part of"""
