# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from Base.Metadata import export, constmethod
from Base.BaseClass import BaseClass
from typing import Any, Final, List, Dict, Tuple, overload

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

    def reset(self) -> None:
        """
        Reset the link view and clear the links
        """
        ...

    @overload
    def setMaterial(self, material: None, /) -> None: ...
    @overload
    def setMaterial(self, material: Any, /) -> None: ...
    @overload
    def setMaterial(self, material: List[Any], /) -> None: ...
    @overload
    def setMaterial(self, material: Dict[int, Any], /) -> None: ...
    def setMaterial(self, material: Any, /) -> None:
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
        ...

    @overload
    def setType(self, type: int, /) -> None: ...
    @overload
    def setType(self, type: int, sublink: bool, /) -> None: ...
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
        ...

    @overload
    def setTransform(self, matrix: Any, /) -> None: ...
    @overload
    def setTransform(self, matrix: List[Any], /) -> None: ...
    @overload
    def setTransform(self, matrix: Dict[int, Any], /) -> None: ...
    def setTransform(self, matrix: Any, /) -> None:
        """
        set transformation of the linked object

        set transformation for the elements of the link
                                    array/group

        set transformation for elements of the link
                                          array/group by index
        """
        ...

    def setChildren(self, children: List[Any], vis: List[Any] = [], type: int = 0, /) -> None:
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
        ...

    @overload
    def setLink(self, obj: Any, /) -> None: ...
    @overload
    def setLink(self, obj: Any, subname: str, /) -> None: ...
    @overload
    def setLink(self, obj: Any, subname: List[str], /) -> None: ...
    def setLink(self, obj: Any, subname: Any = None, /) -> None:
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
        ...

    def getDetailPath(self, element: Any, /) -> Tuple[Any, Any]:
        """
        get the 3d path an detail of an element.

        Return a tuple(path,detail) for the coin3D SoPath and SoDetail of the element
        """
        ...

    def getElementPicked(self, pickPoint: Any, /) -> Any:
        """
        get the element under a 3d pick point.
        """
        ...

    def getBoundBox(self, vobj: Any = None, /) -> Any:
        """
        get the bounding box.
        """
        ...

    @constmethod
    def getChildren(self) -> Any:
        """
        Get children view objects
        """
        ...
    LinkedView: Final[Any] = ...
    """The linked view object"""

    SubNames: Final[Any] = ...
    """The sub-object reference of the link"""

    RootNode: Final[Any] = ...
    """A pivy node holding the cloned representation of the linked view object"""

    Owner: Any = ...
    """The owner view object of this link handle"""

    Visibilities: Any = ...
    """Get/set the child element visibility"""

    Count: int = 0
    """Set the element size to create an array of linked object"""
