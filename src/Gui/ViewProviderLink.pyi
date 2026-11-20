# SPDX-License-Identifier: LGPL-2.1-or-later

from ViewProviderDocumentObject import ViewProviderDocumentObject
from typing import Final
from Base.Placement import Placement
from Gui.LinkView import LinkView as LinkViewPy

class ViewProviderLink(ViewProviderDocumentObject):
    """
    This is the ViewProviderLink class

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    DraggingPlacement: Placement
    """Get/set dragger placement during dragging"""

    LinkView: Final[LinkViewPy]
    """Get the associated LinkView object"""
