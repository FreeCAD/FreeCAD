# SPDX-License-Identifier: LGPL-2.1-or-later

"""Typed public method signatures for the ``FreeCADGui._View3DInventor`` PyCXX type."""

from __future__ import annotations

from collections.abc import Callable
from typing import Any, Literal, TypeAlias, TypedDict, overload
from Base.Metadata import deprecated

from FreeCAD import DocumentObject, Placement, Rotation, Vector
from FreeCADGui import _MDIView, _View3DInventorViewer

_Point2: TypeAlias = tuple[int, int]
_Point3: TypeAlias = tuple[float, float, float]
_Quaternion: TypeAlias = tuple[float, float, float, float]
_ViewOrientation: TypeAlias = (
    Literal[
        "Top",
        "Bottom",
        "Front",
        "Rear",
        "Left",
        "Right",
        "Isometric",
        "Dimetric",
        "Trimetric",
        "Custom",
    ]
    | str
)
_CameraType: TypeAlias = Literal["Orthographic", "Perspective"] | int | str
_DraggerCallbackType: TypeAlias = Literal[
    "addFinishCallback",
    "addStartCallback",
    "addMotionCallback",
    "addValueChangedCallback",
]

class _View3DObjectInfo(TypedDict, total=False):
    """Hit-test payload returned for one picked object in the 3D view."""

    x: float
    y: float
    z: float
    Document: str
    Object: str
    Component: str
    ParentObject: DocumentObject
    SubName: str

class _View3DRayInfo(TypedDict, total=False):
    """World-space ray-pick payload returned by ``getObjectInfoRay()``."""

    PickedPoint: Vector
    Document: str
    Object: str
    ParentObject: str
    Component: str
    SubName: str

class _View3DEventInfo(TypedDict, total=False):
    """Dictionary payload passed to one classic 3D-view event callback."""

    Type: str
    Time: str
    Position: _Point2
    ShiftDown: bool
    CtrlDown: bool
    AltDown: bool
    State: str
    Key: str
    Button: str
    Delta: int
    Translation: _Point3
    Rotation: _Quaternion

_View3DEventCallback: TypeAlias = Callable[[_View3DEventInfo], object]
_View3DProxyCallback: TypeAlias = Callable[[object], object]

class _View3DInventor:
    """Primary 3D Inventor view wrapper used by the FreeCAD GUI."""

    def fitAll(self, factor: float = 1.0, /) -> None:
        """Fit the full scene into view."""
        ...

    def boxZoom(self, XMin: int, YMin: int, XMax: int, YMax: int) -> None:
        """Zoom to one screen-space rectangle."""
        ...

    def viewBottom(self) -> None:
        """Orient the camera to the bottom view."""
        ...

    def viewFront(self) -> None:
        """Orient the camera to the front view."""
        ...

    def viewLeft(self) -> None:
        """Orient the camera to the left view."""
        ...

    def viewRear(self) -> None:
        """Orient the camera to the rear view."""
        ...

    def viewRight(self) -> None:
        """Orient the camera to the right view."""
        ...

    def viewTop(self) -> None:
        """Orient the camera to the top view."""
        ...

    def viewAxometric(self) -> None:
        """Orient the camera axometrically."""
        ...

    def viewAxonometric(self) -> None:
        """Orient the camera axonometrically."""
        ...

    def viewIsometric(self) -> None:
        """Orient the camera isometrically."""
        ...

    def viewDimetric(self) -> None:
        """Orient the camera dimetrically."""
        ...

    def viewTrimetric(self) -> None:
        """Orient the camera trimetrically."""
        ...

    def viewDefaultOrientation(
        self, view: _ViewOrientation | None = None, scale: float = -1.0, /
    ) -> None:
        """Restore the default camera orientation."""
        ...

    def viewRotateLeft(self) -> None:
        """Rotate the view left."""
        ...

    def viewRotateRight(self) -> None:
        """Rotate the view right."""
        ...

    def zoomIn(self) -> None:
        """Zoom the camera in."""
        ...

    def zoomOut(self) -> None:
        """Zoom the camera out."""
        ...

    @overload
    def viewPosition(self) -> Placement | None:
        """Return the current view placement."""
        ...

    @overload
    def viewPosition(
        self, placement: Placement, steps: int = 0, duration: int = -1, /
    ) -> Placement | None:
        """Return or animate the current view placement."""
        ...

    def startAnimating(self, x: float, y: float, z: float, velocity: float, /) -> None:
        """Start continuous camera animation."""
        ...

    def stopAnimating(self) -> None:
        """Stop continuous camera animation."""
        ...

    def setAnimationEnabled(self, enabled: bool, /) -> None:
        """Enable or disable camera animation support."""
        ...

    def isAnimationEnabled(self) -> bool:
        """Return whether camera animation support is enabled."""
        ...

    def setPopupMenuEnabled(self, enabled: bool, /) -> None:
        """Enable or disable the view popup menu."""
        ...

    def isPopupMenuEnabled(self) -> bool:
        """Return whether the view popup menu is enabled."""
        ...

    def dump(self, filename: str, only_visible: bool = False, /) -> None:
        """Dump the scene graph to one file."""
        ...

    def dumpNode(self, node: object, /) -> str:
        """Return a textual dump of one scene-graph node."""
        ...

    def saveImage(
        self,
        filename: str,
        width: int = -1,
        height: int = -1,
        color: str = "Current",
        comment: str = "$MIBA",
        samples: int = 0,
        /,
    ) -> None:
        """Save the current view as a raster image."""
        ...

    def saveVectorGraphic(
        self, filename: str, page_size: int = 4, background: str = "white", /
    ) -> None:
        """Save the current view as a vector graphic."""
        ...

    def getCamera(self) -> str:
        """Return the current camera description."""
        ...

    def getCameraNode(self) -> Any | None:
        """Return the underlying camera node."""
        ...

    def getViewDirection(self) -> Vector:
        """Return the current camera view direction."""
        ...

    def getUpDirection(self) -> Vector:
        """Return the current camera up direction."""
        ...

    def setViewDirection(self, direction: _Point3, /) -> None:
        """Set the camera view direction."""
        ...

    def setCamera(self, camera: str, /) -> None:
        """Set the camera from one serialized description."""
        ...

    def setCameraOrientation(
        self,
        rotation: Rotation | tuple[float, float, float, float],
        move_to_rotation_center: bool = False,
        /,
    ) -> None:
        """Set the camera orientation."""
        ...

    def getCameraOrientation(self) -> Rotation:
        """Return the current camera orientation."""
        ...

    def getCameraType(self) -> str:
        """Return the current camera type name."""
        ...

    def setCameraType(self, camera_type: _CameraType, /) -> None:
        """Set the camera type."""
        ...

    def listCameraTypes(self) -> list[str]:
        """Return the supported camera type names."""
        ...

    def getCursorPos(self) -> tuple[int, int]:
        """Return the current cursor position in view coordinates."""
        ...

    def getObjectInfo(self, point: _Point2, radius: float = 0.0, /) -> _View3DObjectInfo | None:
        """Return hit-test information for one point."""
        ...

    def getObjectsInfo(
        self, point: _Point2, radius: float = 0.0, /
    ) -> list[_View3DObjectInfo] | None:
        """Return hit-test information for all objects near one point."""
        ...

    def getSize(self) -> tuple[int, int]:
        """Return the current view size in pixels."""
        ...

    @overload
    def getObjectInfoRay(self, start: Vector, direction: Vector, /) -> _View3DRayInfo | None:
        """Return hit-test information for one world-space ray."""
        ...

    @overload
    def getObjectInfoRay(
        self,
        start_x: float,
        start_y: float,
        start_z: float,
        direction_x: float,
        direction_y: float,
        direction_z: float,
        /,
    ) -> _View3DRayInfo | None: ...
    @overload
    def getPoint(self, x: int, y: int, /) -> Vector:
        """Project one screen-space point into world coordinates."""
        ...

    @overload
    def getPoint(self, point: _Point2, /) -> Vector: ...
    @overload
    def getPointOnFocalPlane(self, x: int, y: int, /) -> Vector:
        """Project one screen-space point onto the focal plane."""
        ...

    @overload
    def getPointOnFocalPlane(self, point: _Point2, /) -> Vector: ...
    @overload
    def getPointOnScreen(self, point: Vector, /) -> _Point2:
        """Project one world-space point onto the screen."""
        ...

    @overload
    def getPointOnScreen(self, x: float, y: float, z: float, /) -> _Point2: ...
    @overload
    def getPointOnViewport(self, point: Vector, /) -> _Point2:
        """Project one world-space point into viewport coordinates."""
        ...

    @overload
    def getPointOnViewport(self, x: float, y: float, z: float, /) -> _Point2: ...
    @overload
    def projectPointToLine(self, x: int, y: int, /) -> tuple[Vector, Vector]:
        """Project one screen-space point to a world-space line."""
        ...

    @overload
    def projectPointToLine(self, point: _Point2, /) -> tuple[Vector, Vector]: ...
    def addEventCallback(
        self, event_type: str, callback: _View3DEventCallback, /
    ) -> _View3DEventCallback:
        """Register one event callback."""
        ...

    def removeEventCallback(self, event_type: str, callback: _View3DEventCallback, /) -> None:
        """Remove one event callback."""
        ...

    def setAnnotation(self, name: str, buffer: str, /) -> None:
        """Set one named annotation buffer."""
        ...

    def removeAnnotation(self, name: str, /) -> None:
        """Remove one named annotation buffer."""
        ...

    def getSceneGraph(self) -> Any:
        """Return the root scene graph."""
        ...

    def getViewer(self) -> _View3DInventorViewer:
        """Return the low-level viewer wrapper."""
        ...

    def addEventCallbackPivy(
        self, event_type: object, callback: _View3DProxyCallback, extended: bool | int = 1, /
    ) -> _View3DProxyCallback:
        """Register one Pivy event callback."""
        ...

    def removeEventCallbackPivy(
        self, event_type: object, callback: _View3DProxyCallback, extended: bool | int = 1, /
    ) -> _View3DProxyCallback:
        """Remove one Pivy event callback."""
        ...

    @deprecated(
        deprecated_in="26.3",
        removed_in="27.2",
        replacement="addEventCallbackPivy",
    )
    def addEventCallbackSWIG(
        self, event_type: object, callback: _View3DProxyCallback, extended: bool | int = 1, /
    ) -> _View3DProxyCallback:
        """Register one SWIG event callback."""
        ...

    @deprecated(
        deprecated_in="26.3",
        removed_in="27.2",
        replacement="removeEventCallbackPivy",
    )
    def removeEventCallbackSWIG(
        self, event_type: object, callback: _View3DProxyCallback, extended: bool | int = 1, /
    ) -> _View3DProxyCallback:
        """Remove one SWIG event callback."""
        ...

    def listNavigationTypes(self) -> list[str]:
        """Return the supported navigation-style names."""
        ...

    def getNavigationType(self) -> str:
        """Return the current navigation-style name."""
        ...

    def setNavigationType(self, navigation_type: str, /) -> None:
        """Set the navigation style by name."""
        ...

    def setAxisCross(self, enabled: bool, /) -> None:
        """Enable or disable the axis cross."""
        ...

    def hasAxisCross(self) -> bool:
        """Return whether the axis cross is enabled."""
        ...

    def addDraggerCallback(
        self,
        dragger: object,
        callback_type: _DraggerCallbackType,
        callback: _View3DProxyCallback,
        /,
    ) -> _View3DProxyCallback:
        """Register one dragger callback."""
        ...

    def removeDraggerCallback(
        self,
        dragger: object,
        callback_type: _DraggerCallbackType,
        callback: _View3DProxyCallback,
        /,
    ) -> _View3DProxyCallback:
        """Remove one dragger callback."""
        ...

    def getViewProvidersOfType(self, type_name: str, /) -> list[object]:
        """Return the view providers of one type name."""
        ...

    def redraw(self) -> None:
        """Request a redraw of the view."""
        ...

    def setName(self, name: str, /) -> None:
        """Set the internal view name."""
        ...

    def toggleClippingPlane(
        self,
        toggle: int = -1,
        beforeEditing: bool = False,
        noManip: bool = True,
        pla: Placement = ...,
    ) -> None:
        """Toggle or configure the clipping plane."""
        ...

    def hasClippingPlane(self) -> bool:
        """Return whether a clipping plane is active."""
        ...

    def graphicsView(self) -> Any:
        """Return the underlying graphics-view object."""
        ...

    def setCornerCrossVisible(self, visible: bool, /) -> None:
        """Show or hide the corner cross."""
        ...

    def isCornerCrossVisible(self) -> bool:
        """Return whether the corner cross is visible."""
        ...

    def setCornerCrossSize(self, size: int, /) -> None:
        """Set the corner-cross size."""
        ...

    def getCornerCrossSize(self) -> int:
        """Return the corner-cross size."""
        ...

    def cast_to_base(self) -> _MDIView:
        """Return this view as the base MDI view wrapper."""
        ...
