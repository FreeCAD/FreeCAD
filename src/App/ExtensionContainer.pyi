from Base.Metadata import export, constmethod
from PropertyContainer import PropertyContainer


@export(
    Initialization=True,
    Constructor=True,
)
class ExtensionContainer(PropertyContainer):
    """
    Base class for all objects which can be extended
    Author: Stefan Troeger (stefantroeger@gmx.net)
    Licence: LGPL
    """

    def addExtension(self, identifier: str) -> None:
        """
        Adds an extension to the object. Requires the string identifier for the python extension as argument
        """
        ...

    @constmethod
    def hasExtension(self, identifier: str) -> bool:
        """
        Returns if this object has the specified extension
        """
        ...
