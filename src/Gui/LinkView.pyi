# SPDX-License-Identifier: LGPL-2.1-or-later

from App.DocumentObject import DocumentObject
from App.Material import Material
from Base.Matrix import Matrix
from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from Base.BoundBox import BoundBox
from ViewProviderDocumentObject import ViewProviderDocumentObject
from typing import TYPE_CHECKING, Any, Final, TypeAlias
from collections.abc import Sequence

@export(
    Include="Gui/ViewProviderLink.h",
    Constructor=True,
    Delete=True,
)
class LinkView(BaseClass):
    """
    Helper class to link to a view object

    Author: Zheng, Lei (realthunder.dev@gmail.com)
    Licence: LGPL
    """

    if TYPE_CHECKING:  # because generate deletes top-level declaration
        _CoinSoPickedPoint: TypeAlias = Any
        _CoinSoPath: TypeAlias = Any
        _CoinSoDetail: TypeAlias = Any
        _CoinSoSeparator: TypeAlias = Any

    def reset(self) -> None:
        """
        Reset the link view and clear the links
        """

    def setMaterial(self, material: Material | dict[int, Material] | Sequence[Material] | None, /) -> None:
        """
        setMaterial(Material): set the override material of the entire linked object

        setMaterial([Material,...]): set the materials for the elements of the link
                                     array/group.

        setMaterial({Int:Material,...}): set the material for the elements of the
                                         link array/group by index.

        If material is None, then the material is unset. If the material of an element
        is unset, it defaults to the override material of the linked object, if there
        is one
        """

    def setType(self, type: int, sublink: bool = True, /) -> None:
        """
        set the link type.

        type=0:  override transformation and visibility
        type=1:  override visibility
        type=2:  no override
        type=-1: sub-object link with override visibility
        type=-2: sub-object link with override transformation and visibility

        sublink: auto delegate to the sub-object references in the link, if there is
                 one and only one.
        """

    def setTransform(self, matrix: Matrix | dict[int, Matrix] | Sequence[Matrix], /) -> None:
        """
        set transformation of the linked object

        set transformation for the elements of the link
                                    array/group

        set transformation for elements of the link
                                          array/group by index
        """

    def setChildren(self, children: list[DocumentObject], vis: list[bool | int] = [], type: int = 0, /) -> None:
        """
        Group a list of children objects. Note, this mode of operation is incompatible
        with link array. Calling this function will deactivate link array. And calling
        setSize() will reset all linked children.

        vis: initial visibility status of the children

        type: children linking type,
           0: override transformation and visibility,
           1: override visibility,
           2: override none.
        """

    def setLink(
        self,
        obj: DocumentObject | ViewProviderDocumentObject | None,
        subname: str | Sequence[str] | None = None,
        /,
    ) -> None:
        """
        Set the link

        Set the link with a sub-object reference

        Set the link with a list of sub object references

        object: The linked document object or its view object

        subname: a string or tuple/list of strings sub-name references to sub object
                 or sub elements (e.g. Face1, Edge2) belonging to the linked object.
                 The sub-name must end with a '.' if it is referencing an sub-object,
                 or else it is considered a sub-element reference.
        """

    def getDetailPath(self, sub: str, path: _CoinSoPath, /) -> _CoinSoDetail | None:
        """
        get the 3d path an detail of an element.

        Return a tuple(path,detail) for the coin3D SoPath and SoDetail of the element
        """

    def getElementPicked(self, pickPoint: _CoinSoPickedPoint, /) -> str | None:
        """
        get the element under a 3d pick point.
        """

    def getBoundBox(self, vobj: ViewProviderDocumentObject, /) -> BoundBox:
        """
        get the bounding box.
        """

    @constmethod
    def getChildren(self) -> tuple[ViewProviderDocumentObject, ...] | None:
        """
        Get children view objects
        """

    LinkedView: Final[ViewProviderDocumentObject | None]
    """The linked view object"""

    SubNames: Final[tuple[str, ...] | None]
    """The sub-object reference of the link"""

    RootNode: Final[_CoinSoSeparator]
    """A pivy node holding the cloned representation of the linked view object"""

    Owner: ViewProviderDocumentObject | None
    """The owner view object of this link handle"""

    Visibilities: tuple[bool, ...] | None
    """Get/set the child element visibility"""

    Count: int
    """Set the element size to create an array of linked object"""
