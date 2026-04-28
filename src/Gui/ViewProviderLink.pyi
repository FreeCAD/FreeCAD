# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from ViewProviderDocumentObject import ViewProviderDocumentObject
from typing import Any, Final

class ViewProviderLink(ViewProviderDocumentObject):
    """
    This is the ViewProviderLink class

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    DraggingPlacement: Any = ...
    """Get/set dragger placement during dragging"""

    LinkView: Final[Any] = ...
    """Get the associated LinkView object"""
