from ViewProvider import ViewProvider
from typing import Any, Final


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

    Object: Any = ...
    """Set/Get the associated data object"""

    ForceUpdate: bool = False
    """Reference count to force update visual"""

    Document: Final[Any] = ...
    """Return the document the view provider is part of"""
