# SPDX-License-Identifier: LGPL-2.1-or-later

from __future__ import annotations

from typing import Any, Callable, Optional, Tuple, overload

from Base.Metadata import class_declarations, export
from Base.Placement import Placement
from Base.PyObjectBase import PyObjectBase
from Base.Vector import Vector

@export(
    Include="Gui/EditableDatumLabel.h",
    Constructor=True,
    Delete=True,
    Initialization=True,
)
@class_declarations("""private:
    struct CallbackState;
    CallbackState* callbackState {nullptr};
    """)
class EditableDatumLabel(PyObjectBase):
    """
    Python wrapper for editable 3D datum labels.

    This helper exposes the Gui-side editable datum label used by interactive
    dimension and positioning workflows.
    """

    @overload
    def __init__(
        self,
        viewer: Any,
        placement: Placement,
        color: Optional[Tuple[float, float, float]] = None,
        autoDistance: bool = False,
        avoidMouseCursor: bool = False,
    ) -> None:
        """
        Create an editable datum label attached to a 3D view.

        viewer : Any
            A `Gui::View3DInventor` or `Gui::View3DInventorViewer` Python object.
        placement : Placement
            Working placement used to position the label geometry.
        color : tuple[float, float, float] | None
            Label RGB color. If `None`, the default datum label color is used.
        autoDistance : bool
            If `True`, keep the label distance automatically adjusted relative to
            the current camera.
        avoidMouseCursor : bool
            If `True`, bias automatic placement away from the mouse cursor.
        """
        ...

    def activate(self) -> None:
        """
        Insert the label into the active 3D scene and start tracking camera changes.
        """
        ...

    def deactivate(self) -> None:
        """
        Remove the label from the active 3D scene and stop any active editing session.
        """
        ...

    def startEdit(
        self,
        value: float,
        eventFilter: Optional[Any] = None,
        visibleToMouse: bool = False,
    ) -> None:
        """
        Start in-place numeric editing for the current label.

        value : float
            Initial numeric value shown by the editor.
        eventFilter : Any | None
            Optional `QObject` used as an extra event filter for the embedded editor.
        visibleToMouse : bool
            If `False`, the editor ignores mouse events and behaves as a keyboard-only
            overlay.
        """
        ...

    def stopEdit(self) -> None:
        """
        Stop the current edit session and keep the current displayed value.
        """
        ...

    def isActive(self) -> bool:
        """
        Return whether the label is currently attached to a viewer.
        """
        ...

    def isInEdit(self) -> bool:
        """
        Return whether the embedded spinbox editor is currently active.
        """
        ...

    def getValue(self) -> float:
        """
        Return the current numeric value tracked by the label.
        """
        ...

    def setSpinboxValue(self, value: float, /) -> None:
        """
        Update the current numeric value and refresh the label text/editor state.
        """
        ...

    def setPlacement(self, placement: Placement, /) -> None:
        """
        Update the label placement used for 3D positioning.
        """
        ...

    def setColor(self, color: Optional[Tuple[float, float, float]], /) -> None:
        """
        Update the label color.
        """
        ...

    def setPoints(self, p1: Vector, p2: Vector, /) -> None:
        """
        Set the label endpoints in the current placement coordinate system.
        """
        ...

    def setFocus(self) -> None:
        """
        Request focus for the embedded editor widget when editing is active.
        """
        ...

    def setFocusToSpinbox(self) -> None:
        """
        Explicitly move keyboard focus to the spinbox editor.
        """
        ...

    def clearSelection(self) -> None:
        """
        Clear any text selection inside the embedded spinbox editor.
        """
        ...

    def setLabelType(self, label_type: str, function: str = "positioning", /) -> None:
        """
        Set the datum label type and its placement behavior.

        label_type : str
            One of `angle`, `distance`, `distancex`, `distancey`, `radius`,
            `diameter`, `symmetric`, or `arclength`.
        function : str
            One of `positioning`, `dimensioning`, or `forced`.
        """
        ...

    def setLabelDistance(self, distance: float, /) -> None:
        """
        Set the label offset distance from its measured geometry.
        """
        ...

    def setLabelStartAngle(self, angle: float, /) -> None:
        """
        Set the datum label start angle parameter.
        """
        ...

    def setLabelRange(self, range: float, /) -> None:
        """
        Set the datum label angular/range parameter.
        """
        ...

    def setLabelRecommendedDistance(self) -> None:
        """
        Recompute a camera-aware recommended label distance.
        """
        ...

    def setLabelAutoDistanceReverse(self, enabled: bool, /) -> None:
        """
        Reverse the automatic label-distance direction when auto-distance is used.
        """
        ...

    def setSpinboxVisibleToMouse(self, enabled: bool, /) -> None:
        """
        Control whether the embedded editor accepts mouse interaction.
        """
        ...

    def setLockedAppearance(self, locked: bool, /) -> None:
        """
        Toggle the visual "accepted/locked" appearance of the label.
        """
        ...

    def resetLockedState(self) -> None:
        """
        Clear any accepted/locked editing state and restore normal appearance.
        """
        ...

    def updateGeometry(self) -> None:
        """
        Recompute the label geometry and editor placement.
        """
        ...

    def getFunction(self) -> str:
        """
        Return the current label function as `positioning`, `dimensioning`, or `forced`.
        """
        ...

    def setValueChangedCallback(self, callback: Optional[Callable[[float], object]], /) -> None:
        """
        Set a callback invoked whenever the current numeric value changes.
        """
        ...

    def setEditingFinishedCallback(self, callback: Optional[Callable[[float], object]], /) -> None:
        """
        Set a callback invoked when editing is accepted with Enter.
        """
        ...

    def setEditingCanceledCallback(self, callback: Optional[Callable[[float], object]], /) -> None:
        """
        Set a callback invoked when editing is canceled with Escape.
        """
        ...

    def setParameterUnsetCallback(self, callback: Optional[Callable[[], object]], /) -> None:
        """
        Set a callback invoked when the current editor input becomes unset/invalid.
        """
        ...

    def setFinishEditingCallback(self, callback: Optional[Callable[[], object]], /) -> None:
        """
        Set a callback invoked for the "finish editing on all visible overlays" action.
        """
        ...
