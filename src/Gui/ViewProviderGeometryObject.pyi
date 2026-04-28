# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import no_args
from ViewProviderDocumentObject import ViewProviderDocumentObject

class ViewProviderGeometryObject(ViewProviderDocumentObject):
    """
    This is the ViewProvider geometry class

    Author: Werner Mayer (wmayer@users.sourceforge.net)
    Licence: LGPL
    """

    @staticmethod
    @no_args
    def getUserDefinedMaterial() -> object:
        """
        Get a material object with the user-defined colors.
        """
        ...
