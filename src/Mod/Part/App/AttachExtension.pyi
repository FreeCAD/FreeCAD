from Base.Metadata import export
from App.DocumentObjectExtension import DocumentObjectExtension
from typing import Any, Final


@export(
    Twin="AttachExtension",
    TwinPointer="AttachExtension",
    Include="Mod/Part/App/AttachExtension.h",
    FatherInclude="App/DocumentObjectExtensionPy.h",
)
class AttachExtension(DocumentObjectExtension):
    """
    This object represents an attachable object with OCC shape.

    Author: DeepSOIC (vv.titov@gmail.com)
    Licence: LGPL
    """

    Attacher: Final[Any] = ...
    """AttachEngine object driving this AttachableObject. Returns a copy."""

    def positionBySupport(self) -> bool:
        """
        positionBySupport() -> bool

        Reposition object based on AttachmentSupport, MapMode and MapPathParameter properties.
        Returns True if attachment calculation was successful, false if object is not attached and Placement wasn't updated,
        and raises an exception if attachment calculation fails.
        """
        ...

    def changeAttacherType(self, typename: str) -> None:
        """
        changeAttacherType(typename) -> None

        Changes Attacher class of this object.
        typename: string. The following are accepted so far:
        'Attacher::AttachEngine3D'
        'Attacher::AttachEnginePlane'
        'Attacher::AttachEngineLine'
        'Attacher::AttachEnginePoint'
        """
        ...
