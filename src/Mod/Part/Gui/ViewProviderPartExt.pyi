# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export
from Gui.ViewProviderGeometryObject import ViewProviderGeometryObject

@export(
    Include="Mod/Part/Gui/ViewProviderExt.h",
    Namespace="PartGui",
)
class ViewProviderPartExt(ViewProviderGeometryObject):
    """
    This is the ViewProvider geometry class

    Author: David Carter (dcarter@davidcarter.ca)
    Licence: LGPL
    """

    ...
