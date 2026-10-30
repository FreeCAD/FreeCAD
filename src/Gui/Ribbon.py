# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Single-row grouped toolbar ("ribbon") for Part Design and sketch editing.

In a supported context the regular toolbars are hidden and replaced by one
row of labelled groups (CREATE, MODIFY, ...). Each group shows a few pinned
tools and its label opens a menu with every tool in the group. Other
workbenches keep FreeCAD's normal toolbars.

Layout entries: a command name, "-" for a separator, or a group command
name, whose items become a sub-menu.
"""

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui, QtWidgets

translate = FreeCAD.Qt.translate

# Toolbars that stay visible next to the ribbon.
KEPT_TOOLBARS = {"Workbench", "Edit"}

PART_DESIGN = [
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Create"),
        "pinned": [
            "PartDesign_NewSketch",
            "PartDesign_Pad",
            "PartDesign_Revolution",
            "PartDesign_AdditivePipe",
            "PartDesign_AdditiveLoft",
            "PartDesign_Hole",
        ],
        "menu": [
            "PartDesign_NewSketch",
            "-",
            "PartDesign_Body",
            "Std_Part",
            "Std_Group",
            "-",
            "PartDesign_Pad",
            "PartDesign_Revolution",
            "PartDesign_AdditivePipe",
            "PartDesign_AdditiveLoft",
            "PartDesign_AdditiveHelix",
            "PartDesign_Hole",
            "-",
            "PartDesign_CompPrimitiveAdditive",
            "-",
            "PartDesign_Mirrored",
            "PartDesign_LinearPattern",
            "PartDesign_PolarPattern",
            "PartDesign_PathPattern",
            "PartDesign_PointPattern",
            "PartDesign_MultiTransform",
            "-",
            "Std_LinkActions",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Modify"),
        "pinned": ["PartDesign_Fillet", "PartDesign_Chamfer", "PartDesign_Thickness"],
        "menu": [
            "PartDesign_Fillet",
            "PartDesign_Chamfer",
            "PartDesign_Draft",
            "PartDesign_Thickness",
            "PartDesign_Defeaturing",
            "-",
            "PartDesign_Boolean",
            "-",
            "Std_VarSet",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Construct"),
        "pinned": ["PartDesign_Plane", "PartDesign_Line", "PartDesign_Point"],
        "menu": [
            "PartDesign_Plane",
            "PartDesign_Line",
            "PartDesign_Point",
            "PartDesign_CoordinateSystem",
            "-",
            "PartDesign_ShapeBinder",
            "PartDesign_SubShapeBinder",
            "PartDesign_Clone",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Inspect"),
        "pinned": ["Std_Measure"],
        "menu": [
            "Std_Measure",
            "Std_MassProperties",
            "-",
            "Part_CheckGeometry",
            "Sketcher_ValidateSketch",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "View"),
        "pinned": ["Std_ViewFitAll"],
        "menu": [
            "Std_ViewFitAll",
            "Std_ViewFitSelection",
            "Std_AlignToSelection",
            "-",
            "Std_ViewGroup",
            "Std_DrawStyle",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Select"),
        "pinned": ["Std_BoxSelection"],
        "menu": [
            "Std_BoxSelection",
            "Std_BoxElementSelection",
            "Std_SelectAll",
            "-",
            "Part_SelectFilter",
            "-",
            "Std_SelBack",
            "Std_SelForward",
        ],
    },
]

SKETCH = [
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Create"),
        "pinned": [
            "Sketcher_CreatePolyline",
            "Sketcher_CreateRectangle",
            "Sketcher_CreateCircle",
            "Sketcher_Create3PointArc",
            "Sketcher_Dimension",
        ],
        "menu": [
            "Sketcher_CreateLine",
            "Sketcher_CreatePolyline",
            "Sketcher_CompCreateRectangles",
            "Sketcher_CompCreateCircle",
            "Sketcher_CompCreateArc",
            "Sketcher_CompCreateRegularPolygon",
            "Sketcher_CompCreateConic",
            "Sketcher_CompSlot",
            "Sketcher_CompCreateBSpline",
            "Sketcher_CreateSpiral",
            "Sketcher_CreatePoint",
            "Sketcher_CreateText",
            "-",
            "Sketcher_Symmetry",
            "Sketcher_Rotate",
            "Sketcher_Translate",
            "-",
            "Sketcher_CompExternal",
            "Sketcher_CarbonCopy",
            "-",
            "Sketcher_Dimension",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Modify"),
        "pinned": ["Sketcher_CreateFillet", "Sketcher_Trimming", "Sketcher_Offset"],
        "menu": [
            "Sketcher_CreateFillet",
            "Sketcher_CreateChamfer",
            "Sketcher_Trimming",
            "Sketcher_Extend",
            "Sketcher_Split",
            "Sketcher_JoinCurves",
            "-",
            "Sketcher_Offset",
            "Sketcher_Translate",
            "Sketcher_Rotate",
            "Sketcher_Scale",
            "Sketcher_Symmetry",
            "-",
            "Sketcher_ToggleConstruction",
            "Sketcher_CompBSplineShowHideGeometryInformation",
            "-",
            "Sketcher_DeleteAllGeometry",
            "Sketcher_DeleteAllConstraints",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Constraints"),
        "pinned": [
            "Sketcher_ConstrainCoincidentUnified",
            "Sketcher_ConstrainHorVer",
            "Sketcher_ConstrainParallel",
            "Sketcher_ConstrainPerpendicular",
            "Sketcher_ConstrainTangent",
            "Sketcher_ConstrainEqual",
            "Sketcher_ConstrainSymmetric",
            "Sketcher_ConstrainLock",
        ],
        "menu": [
            "Sketcher_ConstrainCoincidentUnified",
            "Sketcher_ConstrainHorVer",
            "Sketcher_ConstrainHorizontal",
            "Sketcher_ConstrainVertical",
            "Sketcher_ConstrainParallel",
            "Sketcher_ConstrainPerpendicular",
            "Sketcher_ConstrainTangent",
            "Sketcher_ConstrainEqual",
            "Sketcher_ConstrainSymmetric",
            "Sketcher_ConstrainBlock",
            "Sketcher_ConstrainLock",
            "Sketcher_ConstrainSnellsLaw",
            "-",
            "Sketcher_ConstrainDistance",
            "Sketcher_ConstrainDistanceX",
            "Sketcher_ConstrainDistanceY",
            "Sketcher_ConstrainRadius",
            "Sketcher_ConstrainDiameter",
            "Sketcher_ConstrainAngle",
            "-",
            "Sketcher_ToggleDrivingConstraint",
            "Sketcher_ToggleActiveConstraint",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Inspect"),
        "pinned": ["Sketcher_SelectElementsWithDoFs"],
        "menu": [
            "Sketcher_SelectElementsWithDoFs",
            "Sketcher_SelectConflictingConstraints",
            "Sketcher_SelectRedundantConstraints",
            "Sketcher_SelectMalformedConstraints",
            "Sketcher_SelectPartiallyRedundantConstraints",
            "-",
            "Sketcher_ValidateSketch",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "View"),
        "pinned": ["Sketcher_ViewSketch"],
        "menu": [
            "Sketcher_ViewSketch",
            "Sketcher_ViewSection",
            "Std_ViewFitAll",
            "-",
            "Sketcher_Grid",
            "Sketcher_Snap",
            "Sketcher_RenderingOrder",
            "Sketcher_ArcOverlay",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Select"),
        "pinned": ["Std_BoxElementSelection"],
        "menu": [
            "Std_BoxElementSelection",
            "-",
            "Sketcher_SelectOrigin",
            "Sketcher_SelectHorizontalAxis",
            "Sketcher_SelectVerticalAxis",
            "-",
            "Sketcher_SelectConstraints",
            "Sketcher_SelectElementsAssociatedWithConstraints",
        ],
    },
    {
        "label": QtCore.QT_TRANSLATE_NOOP("Ribbon", "Finish Sketch"),
        "pinned": ["Sketcher_LeaveSketch"],
        "menu": ["Sketcher_LeaveSketch", "Sketcher_CancelSketch"],
        "accent": True,
    },
]


def _context():
    """Return the layout for the current workbench and edit state, or None."""
    wb = FreeCADGui.activeWorkbench()
    name = wb.name() if wb else ""
    if name == "PartDesignWorkbench":
        return PART_DESIGN
    if name == "SketcherWorkbench":
        doc = FreeCADGui.ActiveDocument
        vp = doc.getInEdit() if doc else None
        if vp and vp.Object.isDerivedFrom("Sketcher::SketchObject"):
            return SKETCH
    return None


def _alive(obj):
    try:
        obj.objectName()
        return True
    except RuntimeError:
        return False


class _Actions:
    """Looks up the QActions for commands, making stand-ins where FreeCAD has none.

    FreeCAD only creates a command's action once it is placed in a menu or
    toolbar, so some commands (e.g. the Part Design datums) have none yet.
    """

    def __init__(self, parent):
        self.parent = parent
        self.fallbacks = {}

    def get(self, name):
        """The command's actions; a group command returns its items."""
        cmd = FreeCADGui.Command.get(name)
        if not cmd:
            return []
        actions = cmd.getAction()
        if actions:
            return actions
        if name not in self.fallbacks:
            info = cmd.getInfo()
            action = QtGui.QAction(
                FreeCADGui.getIcon(info["pixmap"]), info["menuText"].replace("&", ""), self.parent
            )
            action.setToolTip(info["toolTip"])
            action.setStatusTip(info["statusTip"])
            action.triggered.connect(lambda checked=False, n=name: FreeCADGui.runCommand(n))
            self.fallbacks[name] = action
        return [self.fallbacks[name]]

    def update(self):
        for name, action in self.fallbacks.items():
            action.setEnabled(FreeCADGui.Command.get(name).isActive())


def _fill_menu(menu, entries, actions):
    for name in entries:
        if name == "-":
            menu.addSeparator()
            continue
        found = actions.get(name)
        if len(found) > 1:
            # Group commands hand out their items; show them as a sub-menu.
            info = FreeCADGui.Command.get(name).getInfo()
            sub = menu.addMenu(found[0].icon(), info["menuText"].replace("&", ""))
            sub.addActions(found)
        elif found:
            menu.addAction(found[0])
    # Drop separators left dangling by commands that are not available.
    previous = None
    for action in menu.actions():
        if action.isSeparator() and (previous is None or previous.isSeparator()):
            menu.removeAction(action)
        else:
            previous = action
    if menu.actions() and menu.actions()[-1].isSeparator():
        menu.removeAction(menu.actions()[-1])


class _Group(QtWidgets.QWidget):
    """Pinned tool buttons above a label that opens the group's full menu."""

    ACCENT = "#2e9d4e"

    def __init__(self, spec, actions, icon_size, parent=None):
        super().__init__(parent)
        self.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Preferred)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(6, 2, 6, 0)
        layout.setSpacing(0)

        icons = QtWidgets.QWidget(self)
        row = QtWidgets.QHBoxLayout(icons)
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(2)
        for name in spec["pinned"]:
            found = actions.get(name)
            if not found:
                continue
            button = QtWidgets.QToolButton(icons)
            button.setDefaultAction(found[0])
            button.setAutoRaise(True)
            button.setIconSize(QtCore.QSize(icon_size, icon_size))
            row.addWidget(button)
        layout.addWidget(icons, 0, QtCore.Qt.AlignHCenter)

        self.menu = QtWidgets.QMenu(self)
        _fill_menu(self.menu, spec["menu"], actions)

        text = translate("Ribbon", spec["label"]).upper()
        self.label = QtWidgets.QToolButton(self)
        self.label.setText(text + " \u25be")
        self.label.setAutoRaise(True)
        self.label.setPopupMode(QtWidgets.QToolButton.InstantPopup)
        self.label.setMenu(self.menu)
        style = "QToolButton::menu-indicator { image: none; width: 0; }"
        if spec.get("accent"):
            style += f" QToolButton {{ color: {self.ACCENT}; font-weight: bold; }}"
        self.label.setStyleSheet(style)
        font = self.label.font()
        font.setPointSizeF(font.pointSizeF() * 0.85)
        self.label.setFont(font)
        layout.addWidget(self.label, 0, QtCore.Qt.AlignHCenter)


class Ribbon(QtCore.QObject):
    ICON_SIZE = 28

    def __init__(self, mw):
        super().__init__(mw)
        self.mw = mw
        self.layout = None
        self.hidden = {}  # toolbar -> whether its View > Toolbars entry was visible
        self.actions = _Actions(self)

        self.bar = QtWidgets.QToolBar(translate("Ribbon", "Ribbon"), mw)
        self.bar.setObjectName("Ribbon")
        self.bar.setMovable(False)
        self.bar.toggleViewAction().setVisible(False)
        before = mw.findChild(QtWidgets.QToolBar, "View")
        if before:
            mw.insertToolBar(before, self.bar)
        else:
            mw.addToolBar(self.bar)
        self.bar.hide()

        self.timer = QtCore.QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.timeout.connect(self.apply)
        self.state_timer = QtCore.QTimer(self)
        self.state_timer.setInterval(500)
        self.state_timer.timeout.connect(self.actions.update)
        # Gui::MainWindow's own signal; PySide only knows the window as a QMainWindow.
        QtCore.QObject.connect(mw, QtCore.SIGNAL("workbenchActivated(QString)"), self.schedule)
        FreeCADGui.addDocumentObserver(self)

    # Gui document observer callbacks
    def slotInEdit(self, vp):
        self.schedule()

    def slotResetEdit(self, vp):
        self.schedule()

    def slotActivateDocument(self, doc):
        self.schedule()

    def schedule(self, *args):
        self.timer.start(0)

    def _foreign_toolbars(self):
        return [
            tb
            for tb in self.mw.findChildren(QtWidgets.QToolBar)
            if tb is not self.bar
            and tb.parentWidget() is self.mw
            and tb.objectName() not in KEPT_TOOLBARS
        ]

    def eventFilter(self, obj, event):
        # Workbench setup and edit modes show toolbars again; keep them hidden.
        if event.type() == QtCore.QEvent.Show and self.layout is not None:
            self.schedule()
        return False

    def apply(self):
        layout = _context()
        if layout is None:
            self._restore()
            return

        if layout is not self.layout:
            self._populate(layout)
        self.hidden = {tb: v for tb, v in self.hidden.items() if _alive(tb)}
        for tb in self._foreign_toolbars():
            if tb not in self.hidden:
                self.hidden[tb] = tb.toggleViewAction().isVisible()
                tb.installEventFilter(self)
            # Hidden entries are skipped when FreeCAD saves toolbar visibility,
            # so the user's normal toolbar setup survives.
            tb.toggleViewAction().setVisible(False)
            tb.hide()
        self.bar.show()
        self.actions.update()
        self.state_timer.start()

    def _populate(self, layout):
        self.layout = layout
        self.bar.clear()
        first = True
        for spec in layout:
            group = _Group(spec, self.actions, self.ICON_SIZE, self.bar)
            if not group.menu.actions():
                group.deleteLater()
                continue
            if not first:
                self.bar.addSeparator()
            self.bar.addWidget(group)
            first = False

    def _restore(self):
        if self.layout is None:
            return
        self.layout = None
        self.state_timer.stop()
        self.bar.hide()
        self.bar.clear()
        for tb, entry_visible in self.hidden.items():
            try:
                tb.removeEventFilter(self)
                tb.toggleViewAction().setVisible(entry_visible)
            except RuntimeError:
                pass  # FreeCAD deleted this toolbar meanwhile
        self.hidden.clear()
        # Let FreeCAD show the active workbench's toolbars as the user set them up.
        FreeCADGui.activeWorkbench().reloadActive()


# Survives importlib.reload() so a reload can tear down the running instance.
_ribbon = globals().get("_ribbon")


def install():
    global _ribbon
    if _ribbon is None:
        _ribbon = Ribbon(FreeCADGui.getMainWindow())
        _ribbon.schedule()
    return _ribbon


def uninstall():
    """Remove the ribbon and bring back the regular toolbars (used for live reloads)."""
    global _ribbon
    if _ribbon is not None:
        _ribbon._restore()
        FreeCADGui.removeDocumentObserver(_ribbon)
        QtCore.QObject.disconnect(
            _ribbon.mw, QtCore.SIGNAL("workbenchActivated(QString)"), _ribbon.schedule
        )
        _ribbon.mw.removeToolBar(_ribbon.bar)
        _ribbon.bar.deleteLater()
        _ribbon.deleteLater()
        _ribbon = None
