# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import constmethod
from App.Extension import Extension

class ViewProviderExtension(Extension):
    """
    Base class for all view provider extensions

    Author: Werner Mayer (wmayer[at]users.sourceforge.net)
    Licence: LGPL
    """

    def setIgnoreOverlayIcon(self, ignore: bool, name: str, /) -> None:
        """
        Ignore the overlay icon of the named extension
        """
        ...

    @constmethod
    def ignoreOverlayIcon(self, name: str, /) -> bool:
        """
        Return whether the overlay icon of the named extension is ignored
        """
        ...
