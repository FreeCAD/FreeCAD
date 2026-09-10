# SPDX-License-Identifier: LGPL-2.1-or-later

"""Shared typing protocols for Draft document objects."""

from __future__ import annotations

from typing import Any, TYPE_CHECKING, Protocol

if TYPE_CHECKING:
    from FreeCAD import (
        _DocumentObjectLink as DocumentObjectLink,
        _DocumentObjectList as DocumentObjectList,
        _DocumentObjectListInput as DocumentObjectListInput,
    )
else:
    DocumentObjectLink = object
    DocumentObjectList = object
    DocumentObjectListInput = object


class DraftAPI(Protocol):
    """Shared typed boundary for the public Draft module used by BIM."""

    def get_type(self, obj: object | None, /) -> str | None: ...

    def getType(self, obj: object | None, /) -> str | None: ...

    def isClone(
        self,
        obj: object,
        objtype: str | list[str] | None = None,
        recursive: bool = False,
        /,
    ) -> bool: ...

    def getObjectsOfType(self, objects: object, object_type: str, /) -> list[Any]: ...

    def get_group_contents(
        self,
        objectslist: object,
        walls: bool = False,
        addgroups: bool = False,
        spaces: bool = False,
        noarchchild: bool = False,
        exclude_names: object | None = None,
    ) -> list[Any]: ...

    def precision(self) -> int: ...

    def loadTexture(
        self, filename: str, size: object | None = None, gui: bool = True, /
    ) -> object: ...

    def svgpatterns(self) -> dict[str, list[str]]: ...

    def get_diffuse_color(self, objs: object, /) -> Any: ...

    def clone(self, obj: object, /) -> Any: ...

    def move(self, *args: Any, **kwargs: Any) -> Any: ...

    def formatObject(self, target: object, origin: object | None = None, /) -> None: ...


class CloneObjectLike(Protocol):
    """Dynamic properties used by Draft clone helpers."""

    @property
    def CloneOf(self) -> DocumentObjectLink: ...

    @CloneOf.setter
    def CloneOf(self, value: DocumentObjectLink) -> None: ...

    @property
    def Objects(self) -> DocumentObjectList: ...

    @Objects.setter
    def Objects(self, value: DocumentObjectListInput) -> None: ...
