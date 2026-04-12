# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Modified 2016-01-03 JAndersM
# Modified with fixes for slab slope, align, offset, and hatches

__title__ = "FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchStructure
#  \ingroup ARCH
#  \brief The Structure object and tools
#
#  This module provides tools to build Structure objects.
#  Structure elements are beams, columns, slabs, and other
#  elements that have a structural function, that is, that
#  support other parts of the building.

import enum
import FreeCAD
import ArchComponent
import ArchCommands
import ArchProfile
import Draft
import DraftVecUtils

from FreeCAD import Vector
from draftutils import params
from draftutils import gui_utils


class StructureMode(enum.Enum):
    """Placement mode for the interactive Structure command.

    COLUMN: single-point placement, extrusion along the working plane normal.
    BEAM:   two-point placement, extrusion along the edge between the two points.
    """

    COLUMN = "Column"
    BEAM = "Beam"


if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    import ArchPrecast
    import draftguitools.gui_trackers as DraftTrackers
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt

    # \endcond


# Reads preset profiles and categorizes them
Categories = []
Presets = ArchProfile.readPresets()
for pre in Presets:
    if pre[1] not in Categories:
        Categories.append(pre[1])


class CommandStructuresFromSelection:
    """The Arch Structures from selection command definition."""

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Arch_MultipleStructures",
            "MenuText": QT_TRANSLATE_NOOP("Arch_StructuresFromSelection", "Multiple Structures"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_StructuresFromSelection",
                "Creates multiple BIM Structures from a selected base, using each selected edge as an extrusion path",
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        selex = FreeCADGui.Selection.getSelectionEx()
        if len(selex) >= 2:
            FreeCAD.ActiveDocument.openTransaction(
                translate("Arch", "Create Structures From Selection")
            )
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            base = selex[
                0
            ].Object  # The first selected object is the base for the Structure objects
            for selexi in selex[
                1:
            ]:  # All the edges from the other objects are used as a Tool (extrusion paths)
                if len(selexi.SubElementNames) == 0:
                    subelement_names = [
                        "Edge" + str(i) for i in range(1, len(selexi.Object.Shape.Edges) + 1)
                    ]
                else:
                    subelement_names = [
                        sub for sub in selexi.SubElementNames if sub.startswith("Edge")
                    ]
                for sub in subelement_names:
                    FreeCADGui.doCommand(
                        "structure = Arch.makeStructure(FreeCAD.ActiveDocument." + base.Name + ")"
                    )
                    FreeCADGui.doCommand(
                        "structure.Tool = (FreeCAD.ActiveDocument."
                        + selexi.Object.Name
                        + ", '"
                        + sub
                        + "')"
                    )
                    FreeCADGui.doCommand("structure.BasePerpendicularToTool = True")
                    FreeCADGui.doCommand("Draft.autogroup(structure)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            FreeCAD.Console.PrintError(
                translate(
                    "Arch",
                    "Select the base object first and then the edges to use as extrusion paths",
                )
                + "\n"
            )


class CommandStructuralSystem:
    """The Arch Structural System command definition."""

    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Arch_StructuralSystem",
            "MenuText": QT_TRANSLATE_NOOP("Arch_StructuralSystem", "Structural System"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_StructuralSystem",
                "Create a structural system from a selected structure and axis",
            ),
        }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel, "Structure")
            ax = Draft.getObjectsOfType(sel, "Axis")
            if ax:
                FreeCAD.ActiveDocument.openTransaction(
                    translate("Arch", "Create Structural System")
                )
                FreeCADGui.addModule("Arch")
                if st:
                    FreeCADGui.doCommand(
                        "obj = Arch.makeStructuralSystem("
                        + ArchCommands.getStringList(st)
                        + ", "
                        + ArchCommands.getStringList(ax)
                        + ")"
                    )
                else:
                    FreeCADGui.doCommand(
                        "obj = Arch.makeStructuralSystem(axes = "
                        + ArchCommands.getStringList(ax)
                        + ")"
                    )
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
            else:
                FreeCAD.Console.PrintError(
                    translate("Arch", "Select at least an axis object") + "\n"
                )


class _CommandStructure:
    "the Arch Structure command definition"

    def __init__(self):

        self.mode = StructureMode.COLUMN

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def _loadDimensions(self):
        """Load Width/Height/Length from mode-specific params.

        Each mode persists its own set of dimensions so that switching between beam and column
        doesn't pollute one mode's values with the other's.
        """
        prefix = "Beam" if self.mode == StructureMode.BEAM else "Column"
        self.Width = params.get_param_arch(prefix + "Width")
        self.Height = params.get_param_arch(prefix + "Height")
        self.Length = params.get_param_arch(prefix + "Length")

    def Activated(self):

        self.doc = FreeCAD.ActiveDocument
        self._loadDimensions()
        self.Profile = None
        self.bpoint = None
        self.precastvalues = None
        self.wp = None
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel, "Structure")
            ax = Draft.getObjectsOfType(sel, "Axis")
            if ax:
                FreeCADGui.runCommand("Arch_StructuralSystem")
                return
            elif not (ax) and not (st):
                self.doc.openTransaction(translate("Arch", "Create Structure"))
                FreeCADGui.addModule("Arch")
                for obj in sel:
                    FreeCADGui.doCommand(
                        "obj = Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")"
                    )
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                self.doc.commitTransaction()
                self.doc.recompute()
                return

        # interactive mode
        import WorkingPlane

        FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
        self.wp = WorkingPlane.get_working_plane()
        self.points = []
        self.tracker = DraftTrackers.boxTracker()
        self.tracker.width(self.Width)
        self.tracker.height(self.Height)
        self.tracker.length(self.Length)
        self.tracker.setRotation(self.wp.get_placement().Rotation)
        self.tracker.on()
        self.precast = ArchPrecast._PrecastTaskPanel()
        self.dents = ArchPrecast._DentsTaskPanel()
        self.precast.Dents = self.dents
        if self.mode == StructureMode.BEAM:
            title = translate("Arch", "First Point of Beam")
        else:
            title = translate("Arch", "Base Point of Column")
        FreeCADGui.Snapper.getPoint(
            callback=self.getPoint,
            movecallback=self.update,
            extradlg=[self.taskbox(), self.precast.form, self.dents.form],
            title=title,
        )
        FreeCADGui.draftToolBar.continueCmd.show()

    def getPoint(self, point=None, obj=None):
        "this function is called by the snapper when it has a 3D point"

        self.mode = StructureMode.BEAM if self.modeb.isChecked() else StructureMode.COLUMN
        if point is None:
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.tracker.finalize()
            return
        if self.mode == StructureMode.BEAM and (self.bpoint is None):
            self.bpoint = point
            # Recreate precast/dents task boxes. The getPoint() call below replaces the task panel,
            # destroying the task boxes that were embedded via extradlg by the first call.
            self.precast = ArchPrecast._PrecastTaskPanel()
            self.dents = ArchPrecast._DentsTaskPanel()
            self.precast.Dents = self.dents
            FreeCADGui.Snapper.getPoint(
                last=point,
                callback=self.getPoint,
                movecallback=self.update,
                extradlg=[self.taskbox(), self.precast.form, self.dents.form],
                title=translate("Arch", "Next Point") + ":",
                mode="line",
            )
            return
        FreeCAD.activeDraftCommand = None
        FreeCADGui.Snapper.off()
        self.tracker.off()
        horiz = True  # determines the type of rotation to apply to the final object
        self.doc.openTransaction(translate("Arch", "Create Structure"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("WorkingPlane")
        if self.mode == StructureMode.BEAM:
            self.Length = point.sub(self.bpoint).Length
            params.set_param_arch("BeamLength", self.Length)
        if self.Profile is not None:
            try:  # try to update latest precast values - fails if dialog has been destroyed already
                self.precastvalues = self.precast.getValues()
            except Exception:
                pass
            if ("Precast" in self.Profile) and self.precastvalues:
                # precast concrete
                self.precastvalues["PrecastType"] = self.Profile.split("_")[1]
                self.precastvalues["Length"] = self.Length
                self.precastvalues["Width"] = self.Width
                self.precastvalues["Height"] = self.Height
                argstring = ""
                # fix for precast placement, since their (0,0) point is the lower left corner
                if self.mode == StructureMode.BEAM:
                    delta = FreeCAD.Vector(0, 0 - self.Width / 2, 0)
                else:
                    delta = FreeCAD.Vector(-self.Length / 2, -self.Width / 2, 0)
                delta = self.wp.get_global_coords(delta, as_vector=True)
                point = point.add(delta)
                if self.bpoint:
                    self.bpoint = self.bpoint.add(delta)
                # build the string definition
                for pair in self.precastvalues.items():
                    argstring += pair[0].lower() + "="
                    if isinstance(pair[1], str):
                        argstring += '"' + pair[1] + '",'
                    else:
                        argstring += str(pair[1]) + ","
                FreeCADGui.addModule("ArchPrecast")
                FreeCADGui.doCommand("s = ArchPrecast.makePrecast(" + argstring + ")")
            else:
                # metal profile
                FreeCADGui.doCommand("p = Arch.makeProfile(" + str(self.Profile) + ")")
                if self.mode == StructureMode.BEAM:
                    # horizontal
                    FreeCADGui.doCommand(
                        "s = Arch.makeStructure(p,length=" + str(self.Length) + ")"
                    )
                    horiz = False
                else:
                    # vertical
                    FreeCADGui.doCommand(
                        "s = Arch.makeStructure(p,height=" + str(self.Height) + ")"
                    )
                FreeCADGui.doCommand("s.Profile = " + repr(self.Profile[2]))
        else:
            FreeCADGui.doCommand(
                "s = Arch.makeStructure(length="
                + str(self.Length)
                + ",width="
                + str(self.Width)
                + ",height="
                + str(self.Height)
                + ")"
            )

        # calculate rotation
        if self.mode == StructureMode.BEAM and self.bpoint:
            FreeCADGui.doCommand(
                "s.Placement = Arch.placeAlongEdge("
                + DraftVecUtils.toString(self.bpoint)
                + ","
                + DraftVecUtils.toString(point)
                + ","
                + str(horiz)
                + ")"
            )
        else:
            FreeCADGui.doCommand("s.Placement.Base = " + DraftVecUtils.toString(point))
            FreeCADGui.doCommand("wp = WorkingPlane.get_working_plane()")
            FreeCADGui.doCommand(
                "s.Placement.Rotation = s.Placement.Rotation.multiply(wp.get_placement().Rotation)"
            )

        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(s)")
        self.doc.commitTransaction()
        self.doc.recompute()
        # gui_utils.end_all_events()  # Causes a crash on Linux.
        self.tracker.finalize()
        if FreeCADGui.draftToolBar.continueMode:
            self.Activated()

    def _createItemlist(self, baselist):
        "create nice labels for presets in the task panel"

        ilist = []
        for p in baselist:
            f = FreeCAD.Units.Quantity(p[4], FreeCAD.Units.Length).getUserPreferred()
            d = params.get_param("Decimals", path="Units")
            s1 = str(round(p[4] / f[1], d))
            s2 = str(round(p[5] / f[1], d))
            s3 = str(f[2])
            ilist.append(p[2] + " (" + s1 + "x" + s2 + s3 + ")")
        return ilist

    def taskbox(self):
        "sets up a taskbox widget"

        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch", "Structure Options"))
        grid = QtGui.QGridLayout(w)

        # mode box
        labelmode = QtGui.QLabel(translate("Arch", "Parameters of the structure") + ":")
        self.modeb = QtGui.QRadioButton(translate("Arch", "Beam"))
        self.modec = QtGui.QRadioButton(translate("Arch", "Column"))
        if self.bpoint or self.mode == StructureMode.BEAM:
            self.modeb.setChecked(True)
        else:
            self.modec.setChecked(True)
        grid.addWidget(labelmode, 0, 0, 1, 2)
        grid.addWidget(self.modeb, 1, 0, 1, 1)
        grid.addWidget(self.modec, 1, 1, 1, 1)

        # categories box
        labelc = QtGui.QLabel(translate("Arch", "Category"))
        self.valuec = QtGui.QComboBox()
        self.valuec.addItems([" ", "Precast concrete"] + Categories)
        grid.addWidget(labelc, 2, 0, 1, 1)
        grid.addWidget(self.valuec, 2, 1, 1, 1)

        # presets box
        labelp = QtGui.QLabel(translate("Arch", "Preset"))
        self.vPresets = QtGui.QComboBox()
        self.pSelect = [None]
        fpresets = [" "]
        self.vPresets.addItems(fpresets)
        grid.addWidget(labelp, 3, 0, 1, 1)
        grid.addWidget(self.vPresets, 3, 1, 1, 1)

        # length
        label1 = QtGui.QLabel(translate("Arch", "Length"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(FreeCAD.Units.Quantity(self.Length, FreeCAD.Units.Length).UserString)
        grid.addWidget(label1, 4, 0, 1, 1)
        grid.addWidget(self.vLength, 4, 1, 1, 1)

        # width
        label2 = QtGui.QLabel(translate("Arch", "Width"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width, FreeCAD.Units.Length).UserString)
        grid.addWidget(label2, 5, 0, 1, 1)
        grid.addWidget(self.vWidth, 5, 1, 1, 1)

        # height
        label3 = QtGui.QLabel(translate("Arch", "Height"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(FreeCAD.Units.Quantity(self.Height, FreeCAD.Units.Length).UserString)
        grid.addWidget(label3, 6, 0, 1, 1)
        grid.addWidget(self.vHeight, 6, 1, 1, 1)

        # horizontal button
        value4 = QtGui.QPushButton(translate("Arch", "Switch Length/Height"))
        grid.addWidget(value4, 7, 0, 1, 1)
        value5 = QtGui.QPushButton(translate("Arch", "Switch Length/Width"))
        grid.addWidget(value5, 7, 1, 1, 1)

        # connect slots
        QtCore.QObject.connect(
            self.valuec, QtCore.SIGNAL("currentIndexChanged(int)"), self.setCategory
        )
        QtCore.QObject.connect(
            self.vPresets, QtCore.SIGNAL("currentIndexChanged(int)"), self.setPreset
        )
        QtCore.QObject.connect(self.vLength, QtCore.SIGNAL("valueChanged(double)"), self.setLength)
        QtCore.QObject.connect(self.vWidth, QtCore.SIGNAL("valueChanged(double)"), self.setWidth)
        QtCore.QObject.connect(self.vHeight, QtCore.SIGNAL("valueChanged(double)"), self.setHeight)
        QtCore.QObject.connect(value4, QtCore.SIGNAL("pressed()"), self.rotateLH)
        QtCore.QObject.connect(value5, QtCore.SIGNAL("pressed()"), self.rotateLW)
        QtCore.QObject.connect(self.modeb, QtCore.SIGNAL("toggled(bool)"), self.switchLH)

        # restore preset
        stored = params.get_param_arch("StructurePreset")
        if stored:
            if stored.lower().startswith("precast_"):
                self.valuec.setCurrentIndex(1)
                tp = stored.split("_")[1]
                if tp and (tp in self.precast.PrecastTypes):
                    self.vPresets.setCurrentIndex(self.precast.PrecastTypes.index(tp))
            elif ";" in stored:
                stored = stored.split(";")
                if len(stored) >= 3:
                    if stored[1] in Categories:
                        self.valuec.setCurrentIndex(2 + Categories.index(stored[1]))
                        ps = [p[2] for p in self.pSelect]
                        if stored[2] in ps:
                            self.vPresets.setCurrentIndex(ps.index(stored[2]))
        return w

    def update(self, point, info):
        "this function is called by the Snapper when the mouse is moved"

        if FreeCADGui.Control.activeDialog():
            try:  # try to update latest precast values - fails if dialog has been destroyed already
                self.precastvalues = self.precast.getValues()
            except Exception:
                pass
            if self.Height >= self.Length:
                delta = Vector(0, 0, self.Height / 2)
            else:
                delta = Vector(self.Length / 2, 0, 0)
            delta = self.wp.get_global_coords(delta, as_vector=True)
            if self.mode == StructureMode.COLUMN:
                self.tracker.pos(point.add(delta))
                self.tracker.on()
            else:
                if self.bpoint:
                    delta = Vector(0, 0, -self.Height / 2)
                    delta = self.wp.get_global_coords(delta, as_vector=True)
                    self.tracker.update([self.bpoint.add(delta), point.add(delta)])
                    self.tracker.on()
                    l = (point.sub(self.bpoint)).Length
                    self.vLength.setText(FreeCAD.Units.Quantity(l, FreeCAD.Units.Length).UserString)
                else:
                    self.tracker.off()

    def _paramPrefix(self):
        return "Beam" if self.mode == StructureMode.BEAM else "Column"

    def setWidth(self, d):

        self.Width = d
        self.tracker.width(d)
        params.set_param_arch(self._paramPrefix() + "Width", d)

    def setHeight(self, d):

        self.Height = d
        self.tracker.height(d)
        params.set_param_arch(self._paramPrefix() + "Height", d)

    def setLength(self, d):

        self.Length = d
        self.tracker.length(d)
        params.set_param_arch(self._paramPrefix() + "Length", d)

    def setCategory(self, i):

        self.vPresets.clear()
        if i > 1:
            self.precast.form.hide()
            self.pSelect = [p for p in Presets if p[1] == Categories[i - 2]]
            fpresets = self._createItemlist(self.pSelect)
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        elif i == 1:
            self.precast.form.show()
            self.pSelect = self.precast.PrecastTypes
            fpresets = self.precast.PrecastTypes
            self.vPresets.addItems(fpresets)
            self.setPreset(0)
        else:
            self.precast.form.hide()
            self.pSelect = [None]
            fpresets = [" "]
            self.vPresets.addItems(fpresets)
            params.set_param_arch("StructurePreset", "")

    def setPreset(self, i):

        self.Profile = None
        elt = self.pSelect[i]
        if elt:
            if elt in self.precast.PrecastTypes:
                self.precast.setPreset(elt)
                self.Profile = "Precast_" + elt
                if elt in ["Pillar", "Beam"]:
                    self.dents.form.show()
                else:
                    self.dents.form.hide()
                params.set_param_arch("StructurePreset", self.Profile)
            else:
                # elt[4] is the cross-section depth, elt[5] is the cross-section width.
                # For beams the cross-section depth is Height; for columns it is Length.
                depth = float(elt[4])
                width = float(elt[5])
                if self.mode == StructureMode.BEAM:
                    self.vHeight.setText(
                        FreeCAD.Units.Quantity(depth, FreeCAD.Units.Length).UserString
                    )
                    self.setHeight(depth)
                else:
                    self.vLength.setText(
                        FreeCAD.Units.Quantity(depth, FreeCAD.Units.Length).UserString
                    )
                    self.setLength(depth)
                self.vWidth.setText(FreeCAD.Units.Quantity(width, FreeCAD.Units.Length).UserString)
                self.setWidth(width)
                self.Profile = elt
                params.set_param_arch("StructurePreset", ";".join([str(i) for i in self.Profile]))

    def switchLH(self, beam_toggled):

        self.mode = StructureMode.BEAM if beam_toggled else StructureMode.COLUMN
        self._loadDimensions()
        self.vWidth.setText(FreeCAD.Units.Quantity(self.Width, FreeCAD.Units.Length).UserString)
        self.vHeight.setText(FreeCAD.Units.Quantity(self.Height, FreeCAD.Units.Length).UserString)
        self.vLength.setText(FreeCAD.Units.Quantity(self.Length, FreeCAD.Units.Length).UserString)
        self.tracker.width(self.Width)
        self.tracker.height(self.Height)
        self.tracker.length(self.Length)
        if self.mode == StructureMode.COLUMN:
            self.tracker.setRotation(FreeCAD.Rotation())

    def rotateLH(self):

        l = self.vLength.text()
        h = self.vHeight.text()
        self.vLength.setText(h)
        self.vHeight.setText(l)

    def rotateLW(self):

        l = self.vLength.text()
        w = self.vWidth.text()
        self.vLength.setText(w)
        self.vWidth.setText(l)


class _Structure(ArchComponent.Component):
    "The Structure object"

    def __init__(self, obj):

        ArchComponent.Component.__init__(self, obj)
        self.Type = "Structure"
        self.setProperties(obj)
        obj.IfcType = "Beam"

    def setProperties(self, obj):

        pl = obj.PropertiesList
        if not "Tool" in pl:
            obj.addProperty(
                "App::PropertyLinkSubList",
                "Tool",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP("App::Property", "An optional extrusion path for this element"),
                locked=True,
            )
        if not "ComputedLength" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "ComputedLength",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP("App::Property", "The computed length of the extrusion path"),
                1,
                locked=True,
            )
        if not "ToolOffsetFirst" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "ToolOffsetFirst",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Start offset distance along the extrusion path (positive: extend, negative: trim)",
                ),
                locked=True,
            )
        if not "ToolOffsetLast" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "ToolOffsetLast",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "End offset distance along the extrusion path (positive: extend, negative: trim)",
                ),
                locked=True,
            )
        if not "BasePerpendicularToTool" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "BasePerpendicularToTool",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Automatically align the Base of the Structure perpendicular to the Tool axis",
                ),
                locked=True,
            )
        if not "BaseOffsetX" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "BaseOffsetX",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "X offset between the Base origin and the Tool axis (only used if BasePerpendicularToTool is True)",
                ),
                locked=True,
            )
        if not "BaseOffsetY" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "BaseOffsetY",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Y offset between the Base origin and the Tool axis (only used if BasePerpendicularToTool is True)",
                ),
                locked=True,
            )
        if not "BaseMirror" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "BaseMirror",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Mirror the Base along its Y axis (only used if BasePerpendicularToTool is True)",
                ),
                locked=True,
            )
        if not "BaseRotation" in pl:
            obj.addProperty(
                "App::PropertyAngle",
                "BaseRotation",
                "ExtrusionPath",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Base rotation around the Tool axis (only used if BasePerpendicularToTool is True)",
                ),
                locked=True,
            )
        if not "Length" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Length",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The length of this element, if not based on a profile"
                ),
                locked=True,
            )
        if not "Width" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Width",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The width of this element, if not based on a profile"
                ),
                locked=True,
            )
        if not "Height" in pl:
            obj.addProperty(
                "App::PropertyLength",
                "Height",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The height or extrusion depth of this element. Keep 0 for automatic",
                ),
                locked=True,
            )
        if not "Normal" in pl:
            obj.addProperty(
                "App::PropertyVector",
                "Normal",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The normal extrusion direction of this object (keep (0,0,0) for automatic normal)",
                ),
                locked=True,
            )
        if not "Nodes" in pl:
            obj.addProperty(
                "App::PropertyVectorList",
                "Nodes",
                "Structure",
                QT_TRANSLATE_NOOP("App::Property", "The structural nodes of this element"),
                locked=True,
            )
        if not "Profile" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Profile",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "A description of the standard profile this element is based upon",
                ),
                locked=True,
            )
        if not "NodesOffset" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "NodesOffset",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Offset distance between the centerline and the nodes line"
                ),
                locked=True,
            )
        if not "FaceMaker" in pl:
            obj.addProperty(
                "App::PropertyEnumeration",
                "FaceMaker",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The facemaker type to use to build the profile of this object"
                ),
                locked=True,
            )
            obj.FaceMaker = ["None", "Simple", "Cheese", "Bullseye"]
        if not "ArchSketchData" in pl:
            obj.addProperty(
                "App::PropertyBool",
                "ArchSketchData",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Use Base ArchSketch (if used) data (e.g. widths, aligns, offsets) instead of Wall's properties",
                ),
                locked=True,
            )
            obj.ArchSketchData = True
        if not "ArchSketchEdges" in pl:  # PropertyStringList
            obj.addProperty(
                "App::PropertyStringList",
                "ArchSketchEdges",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Selected edges (or group of edges) of the base ArchSketch, to use in creating the shape of this BIM Structure (instead of using all the Base shape's edges by default).  Input are index numbers of edges or groups.",
                ),
                locked=True,
            )
        else:
            # test if the property was added but as IntegerList, then update;
            type = obj.getTypeIdOfProperty("ArchSketchEdges")
            if type == "App::PropertyIntegerList":
                oldIntValue = obj.ArchSketchEdges
                newStrValue = [str(x) for x in oldIntValue]
                obj.removeProperty("ArchSketchEdges")
                obj.addProperty(
                    "App::PropertyStringList",
                    "ArchSketchEdges",
                    "Structure",
                    QT_TRANSLATE_NOOP(
                        "App::Property",
                        "Selected edges (or group of edges) of the base ArchSketch, to use in creating the shape of this BIM Structure (instead of using all the Base shape's edges by default).  Input are index numbers of edges or groups.",
                    ),
                    locked=True,
                )
                obj.ArchSketchEdges = newStrValue
        if not hasattr(obj, "ArchSketchPropertySet"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "ArchSketchPropertySet",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Select User Defined PropertySet to use in creating variant shape, with same ArchSketch ",
                ),
                locked=True,
            )
            obj.ArchSketchPropertySet = ["Default"]

        # Slab multi-material properties
        if not "Align" in pl:
            obj.addProperty(
                "App::PropertyEnumeration",
                "Align",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Global vertical alignment of the slab stack relative to its base "
                    "face. Bottom/Center/Top. Used when Align Layer is None. "
                    "Only active when IfcType is Slab.",
                ),
                locked=True,
            )
            obj.Align = ["Bottom", "Center", "Top"]

        if not "AlignLayer" in pl:
            obj.addProperty(
                "App::PropertyEnumeration",
                "AlignLayer",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Reference layer: select a specific material layer to pin the slab "
                    "to. When set to a layer name, overrides the global Align. "
                    "Only active when IfcType is Slab and a multi-material is assigned.",
                ),
                locked=True,
            )
            obj.AlignLayer = ["None (use Align)"]

        if not "AlignLayerMode" in pl:
            obj.addProperty(
                "App::PropertyEnumeration",
                "AlignLayerMode",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Which face of the Reference Layer to pin to the base sketch: "
                    "Bottom, Center, or Top of that specific layer. "
                    "Only active when Align Layer is set to a layer name.",
                ),
                locked=True,
            )
            obj.AlignLayerMode = ["Bottom", "Center", "Top"]

        if not "Slope" in pl:
            obj.addProperty(
                "App::PropertyAngle",
                "Slope",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Drainage slope angle applied to the slab along its local X axis. "
                    "Only used when IfcType is Slab. Keep 0 for flat.",
                ),
                locked=True,
            )

        if not "Offset" in pl:
            obj.addProperty(
                "App::PropertyDistance",
                "Offset",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Vertical offset of the slab from its base face along the extrusion "
                    "normal. Applied after Align / AlignLayer. Only used when IfcType is Slab.",
                ),
                locked=True,
            )

        if not "SlopeEdge" in pl:
            obj.addProperty(
                "App::PropertyLinkSub",
                "SlopeEdge",
                "Structure",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Edge to use as the slope direction pivot. If not set, the first "
                    "edge of the base face is used. Only active when Slope != 0 and "
                    "IfcType is Slab.",
                ),
                locked=True,
            )

        if not hasattr(self, "ArchSkPropSetPickedUuid"):
            self.ArchSkPropSetPickedUuid = ""
        if not hasattr(self, "ArchSkPropSetListPrev"):
            self.ArchSkPropSetListPrev = []

    def dumps(self):  # Supercede Arch.Component.dumps()
        dump = super().dumps()
        if not isinstance(dump, tuple):
            dump = (dump,)  # Python Tuple With One Item
        dump = dump + (self.ArchSkPropSetPickedUuid, self.ArchSkPropSetListPrev)
        return dump

    def loads(self, state):
        self.Type = "Structure"
        if state == None:
            return
        elif state[0] == "S":  # state[1] == 't', behaviour before 2024.11.28
            return
        elif state[0] == "Structure":
            self.ArchSkPropSetPickedUuid = state[1]
            self.ArchSkPropSetListPrev = state[2]
        elif state[0] != "Structure":  # model before merging super.dumps/loads()
            self.ArchSkPropSetPickedUuid = state[0]
            self.ArchSkPropSetListPrev = state[1]

    def onDocumentRestored(self, obj):

        ArchComponent.Component.onDocumentRestored(self, obj)
        self.setProperties(obj)

        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
        else:
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])

        # Restore slab-only property visibility and rebuild AlignLayer enum
        self._update_align_layer_enum(obj)
        is_slab = getattr(obj, "IfcType", "") == "Slab"

        for p in ["Align", "AlignLayer", "AlignLayerMode", "Offset", "Slope", "SlopeEdge"]:
            if hasattr(obj, p):
                obj.setEditorMode(p, 0 if is_slab else 2)

        if is_slab:
            align_layer = getattr(obj, "AlignLayer", "None (use Align)")
            layer_active = bool(align_layer and align_layer != "None (use Align)")

            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", ["ReadOnly"] if layer_active else 0)
            if hasattr(obj, "AlignLayerMode"):
                obj.setEditorMode("AlignLayerMode", 0 if layer_active else 2)
            if hasattr(obj, "Slope") and hasattr(obj, "SlopeEdge"):
                slope_val = obj.Slope.Value if hasattr(obj.Slope, "Value") else 0.0
                obj.setEditorMode("SlopeEdge", 0 if abs(slope_val) > 1e-6 else 2)
            # Restore Height read-only state
            layers = self.get_layers(obj)
            if layers:
                if hasattr(obj, "Material") and obj.Material:
                    if hasattr(obj.Material, "Thicknesses"):
                        if 0 not in obj.Material.Thicknesses:
                            obj.setEditorMode("Height", ["ReadOnly"])
                        else:
                            obj.setEditorMode("Height", 0)

    def execute(self, obj):
        "creates the structure shape"

        import Part
        import DraftGeomUtils

        if self.clone(obj):
            return
        if obj.Base and not self.ensureBase(obj):
            return

        # --- Slab: sync Height with multi-material total, manage read-only,
        #     and keep AlignLayer enum up to date ---
        if getattr(obj, "IfcType", "") == "Slab":
            self._update_align_layer_enum(obj)
            layers = self.get_layers(obj)
            if layers:
                total = sum([abs(l) for l in layers])
                if obj.Height.Value != total:
                    obj.Height = total
                if hasattr(obj, "Material") and obj.Material:
                    if hasattr(obj.Material, "Thicknesses"):
                        if 0 not in obj.Material.Thicknesses:
                            obj.setEditorMode("Height", ["ReadOnly"])
                        else:
                            obj.setEditorMode("Height", 0)
            else:
                obj.setEditorMode("Height", 0)
        # --- End slab Height sync ---

        base = None
        pl = obj.Placement

        # PropertySet support
        propSetPickedUuidPrev = self.ArchSkPropSetPickedUuid
        propSetListPrev = self.ArchSkPropSetListPrev
        propSetSelectedNamePrev = obj.ArchSketchPropertySet
        propSetSelectedNameCur = None
        propSetListCur = None
        if Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy, "getPropertySet"):
                # get full list of PropertySet
                propSetListCur = baseProxy.getPropertySet(obj.Base)
                # get updated name (if any) of the selected PropertySet
                propSetSelectedNameCur = baseProxy.getPropertySet(
                    obj.Base, propSetUuid=propSetPickedUuidPrev
                )
        if propSetSelectedNameCur:  # True if selection is not deleted
            if propSetListPrev != propSetListCur:
                obj.ArchSketchPropertySet = propSetListCur
                obj.ArchSketchPropertySet = propSetSelectedNameCur
                self.ArchSkPropSetListPrev = propSetListCur
            # elif propSetListPrev == propSetListCur:
            # pass  #nothing to do in this case
            # but if below, though (propSetListPrev == propSetListCur)
            elif propSetSelectedNamePrev != propSetSelectedNameCur:
                obj.ArchSketchPropertySet = propSetSelectedNameCur
        else:  # True if selection is deleted
            if propSetListCur:
                if propSetListPrev != propSetListCur:
                    obj.ArchSketchPropertySet = propSetListCur
                    obj.ArchSketchPropertySet = "Default"
                # else:  # Seems no need ...
                # obj.PropertySet = 'Default'

        extdata = self.getExtrusionData(obj)

        if extdata:
            sh = extdata[0]
            if not isinstance(sh, list):
                sh = [sh]
            ev = extdata[1]
            if not isinstance(ev, list):
                ev = [ev]
            pla = extdata[2]
            if not isinstance(pla, list):
                pla = [pla]
            base = []
            extrusion_length = 0.0
            for i in range(len(sh)):
                shi = sh[i]
                if i < len(ev):
                    evi = ev[i]
                else:
                    evi = ev[-1]
                    if isinstance(evi, FreeCAD.Vector):
                        evi = FreeCAD.Vector(evi)
                    else:
                        evi = evi.copy()
                if i < len(pla):
                    pli = pla[i]
                else:
                    pli = pla[-1].copy()
                shi.Placement = pli.multiply(shi.Placement)
                if isinstance(evi, FreeCAD.Vector):
                    extv = pla[0].Rotation.multVec(evi)
                    shi = shi.extrude(extv)
                else:
                    try:
                        shi = evi.makePipe(shi)
                    except Part.OCCError:
                        FreeCAD.Console.PrintError(
                            translate(
                                "Arch",
                                "Error: The base shape could not be extruded along this tool object",
                            )
                            + "\n"
                        )
                        return
                base.append(shi)
                extrusion_length += evi.Length
            if len(base) == 1:
                base = base[0]
            else:
                base = Part.makeCompound(base)
            obj.ComputedLength = FreeCAD.Units.Quantity(extrusion_length, FreeCAD.Units.Length)
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                elif obj.Base.Shape.Solids:
                    base = obj.Base.Shape.copy()
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(
                                translate("Arch", "This mesh is an invalid solid") + "\n"
                            )
                            obj.Base.ViewObject.show()
        if (not base) and (not obj.Additions):
            # FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            return

        base = self.processSubShapes(obj, base, pl)
        self.applyShape(obj, base, pl)

        # FIX 3: apply_material_hatches not gated on IfcType == "Slab"
        # Removed IfcType == "Slab" gate — columns, beams, and slabs all
        # support HatchSurfaces / HatchCaps / HatchAllFaces toggles.
        self.apply_material_hatches(obj)

    def get_layers(self, obj):
        """Returns a list of layer thicknesses for multi-material slab support.
        Only active when IfcType is Slab and a multi-material is assigned.
        """
        layers = []
        if not (getattr(obj, "IfcType", "") == "Slab"):
            return layers
        height = obj.Height.Value
        if hasattr(obj, "Material") and obj.Material:
            if hasattr(obj.Material, "Materials"):
                thicknesses = [abs(t) for t in obj.Material.Thicknesses]
                restwidth = height - sum(thicknesses)
                varwidth = 0
                if restwidth > 0:
                    varcount = [t for t in thicknesses if t == 0]
                    if varcount:
                        varwidth = restwidth / len(varcount)
                for t in obj.Material.Thicknesses:
                    if t:
                        layers.append(t)
                    elif varwidth:
                        layers.append(varwidth)
        return layers

    def _update_align_layer_enum(self, obj):
        """Rebuild the AlignLayer enumeration from the current multi-material.
        Guarded against re-entrancy. Preserves selection if name still exists.
        """
        if getattr(self, "_updating_align_layer", False):
            return
        if not hasattr(obj, "AlignLayer"):
            return
        if getattr(obj, "IfcType", "") != "Slab":
            return
        self._updating_align_layer = True
        try:
            entries = ["None (use Align)"]
            layers = self.get_layers(obj)
            if layers and hasattr(obj, "Material") and obj.Material:
                if hasattr(obj.Material, "Materials"):
                    for i, mat in enumerate(obj.Material.Materials):
                        if (
                            hasattr(obj.Material, "Names")
                            and i < len(obj.Material.Names)
                            and obj.Material.Names[i]
                        ):
                            name = obj.Material.Names[i]
                        elif hasattr(mat, "Label"):
                            name = mat.Label
                        else:
                            name = "Layer {}".format(i + 1)
                        entries.append(name)
            try:
                current = obj.AlignLayer
            except Exception:
                current = entries[0]
            if current not in entries:
                current = entries[0]
            try:
                existing = list(obj.getEnumerationsOfProperty("AlignLayer"))
            except Exception:
                existing = []
            if existing != entries:
                obj.AlignLayer = entries
            obj.AlignLayer = current
        finally:
            self._updating_align_layer = False

    def _align_to_z_offset(self, obj, total):
        """Convert the global Align enum to a raw z_offset for the full stack."""
        align = getattr(obj, "Align", "Bottom")
        if align == "Center":
            return -total / 2.0
        elif align == "Top":
            return -total
        else:  # Bottom
            return 0.0

    def _compute_z_offset(self, obj, layers):
        """Return the full z_offset for slab layer stacking.

        Priority:
        1. AlignLayer + AlignLayerMode (layer-specific pin)
        2. Align (global Bottom/Center/Top)
        3. Offset (manual shift) — always added on top
        """
        total = sum(abs(l) for l in layers)
        align_layer = getattr(obj, "AlignLayer", "None (use Align)")

        if align_layer and align_layer != "None (use Align)":
            layer_idx = None
            if hasattr(obj, "Material") and obj.Material:
                if hasattr(obj.Material, "Materials"):
                    for i, mat in enumerate(obj.Material.Materials):
                        if (
                            hasattr(obj.Material, "Names")
                            and i < len(obj.Material.Names)
                            and obj.Material.Names[i]
                        ):
                            name = obj.Material.Names[i]
                        elif hasattr(mat, "Label"):
                            name = mat.Label
                        else:
                            name = "Layer {}".format(i + 1)
                        if name == align_layer:
                            layer_idx = i
                            break

            if layer_idx is not None:
                layer_mode = getattr(obj, "AlignLayerMode", "Bottom")
                cum = sum(abs(l) for l in layers[:layer_idx])
                if layer_mode == "Bottom":
                    z_offset = -cum
                elif layer_mode == "Center":
                    z_offset = -(cum + abs(layers[layer_idx]) / 2.0)
                else:  # Top
                    z_offset = -(cum + abs(layers[layer_idx]))
            else:
                z_offset = self._align_to_z_offset(obj, total)
        else:
            z_offset = self._align_to_z_offset(obj, total)

        manual_offset = getattr(obj, "Offset", None)
        if manual_offset is not None:
            val = manual_offset.Value if hasattr(manual_offset, "Value") else 0.0
            z_offset += val

        return z_offset

    def getExtrusionData(self, obj):
        """returns (shape,extrusion vector or path,placement) or None"""
        if hasattr(obj, "IfcType"):
            IfcType = obj.IfcType
        else:
            IfcType = None
        import Part
        import DraftGeomUtils

        data = ArchComponent.Component.getExtrusionData(self, obj)
        if data:
            if not isinstance(data[0], list):
                # multifuses not considered here
                return data
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        baseface = None
        extrusion = None
        normal = None
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None
                    elif obj.Base.Shape.Faces:
                        if not DraftGeomUtils.isCoplanar(obj.Base.Shape.Faces, tol=0.01):
                            return None
                        else:
                            baseface = obj.Base.Shape.copy()
                    elif obj.Base.Shape.Wires:
                        # ArchSketch feature :
                        # Get base shape wires, and faceMaker, for Structure (slab. etc.) from Base Objects if they store and provide by getStructureBaseShapeWires()
                        # (thickness, normal/extrusion, length, width, baseface maybe for later) of structure (slab etc.)
                        structureBaseShapeWires = []
                        baseShapeWires = []  # baseSlabWires / baseSlabOpeningWires = None
                        faceMaker = None

                        if (
                            hasattr(obj.Base, "Proxy")
                            and obj.ArchSketchData
                            and hasattr(obj.Base.Proxy, "getStructureBaseShapeWires")
                        ):
                            propSetUuid = self.ArchSkPropSetPickedUuid

                            # provide selected edges, or groups, in obj.ArchSketchEdges for processing in getStructureBaseShapeWires() (getSortedClusters) as override
                            structureBaseShapeWires = obj.Base.Proxy.getStructureBaseShapeWires(
                                obj.Base, propSetUuid=propSetUuid
                            )
                            # get slab wires; use original wires if structureBaseShapeWires() provided none
                            if (
                                structureBaseShapeWires
                            ):  # would be false (none) if both base ArchSketch and obj do not have the edges stored / inputted by user
                                # if structureBaseShapeWires is {dict}
                                baseShapeWires = structureBaseShapeWires.get("slabWires")
                                faceMaker = structureBaseShapeWires.get("faceMaker")
                        elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                            skGeom = obj.Base.GeometryFacadeList
                            skGeomEdges = []
                            skPlacement = (
                                obj.Base.Placement
                            )  # Get Sketch's placement to restore later
                            # Get ArchSketch edges to construct ArchStructure
                            # No need to test obj.ArchSketchData ...
                            for ig, geom in enumerate(skGeom):
                                # Construction mode edges should be ignored if
                                # ArchSketchEdges, otherwise, ArchSketchEdges data
                                # needs to take out those in Construction before
                                # using as parameters.
                                if (not obj.ArchSketchEdges and not geom.Construction) or str(
                                    ig
                                ) in obj.ArchSketchEdges:
                                    # support Line, Arc, Circle, Ellipse, BSplineCurve for Sketch
                                    # as Base at the moment
                                    if isinstance(
                                        geom.Geometry,
                                        (
                                            Part.LineSegment,
                                            Part.Circle,
                                            Part.ArcOfCircle,
                                            Part.Ellipse,
                                            Part.BSplineCurve,
                                        ),
                                    ):
                                        skGeomEdgesI = geom.Geometry.toShape()
                                        skGeomEdges.append(skGeomEdgesI)
                            clusterTransformed = []
                            for cluster in Part.getSortedClusters(skGeomEdges):
                                edgesTransformed = []
                                for edge in cluster:
                                    edge.Placement = edge.Placement.multiply(skPlacement)
                                    edgesTransformed.append(edge)
                                clusterTransformed.append(edgesTransformed)
                            for clusterT in clusterTransformed:
                                baseShapeWires.append(Part.Wire(clusterT))

                        if not baseShapeWires:
                            baseShapeWires = obj.Base.Shape.Wires
                        if faceMaker or (obj.FaceMaker != "None"):
                            if not faceMaker:
                                faceMaker = obj.FaceMaker
                            try:
                                baseface = Part.makeFace(
                                    baseShapeWires, "Part::FaceMaker" + str(faceMaker)
                                )
                            except Exception:
                                FreeCAD.Console.PrintError(
                                    translate("Arch", "Facemaker returned an error") + "\n"
                                )
                                # Not returning even Part.makeFace fails, fall back to 'non-Part.makeFace' method
                        if not baseface:
                            for w in baseShapeWires:
                                if not w.isClosed():
                                    p0 = w.OrderedVertexes[0].Point
                                    p1 = w.OrderedVertexes[-1].Point
                                    if p0 != p1:
                                        e = Part.LineSegment(p0, p1).toShape()
                                        w.add(e)
                                w.fix(0.1, 0, 1)  # fixes self-intersecting wires
                                f = Part.Face(w)
                                # check if it is 1st face (f) created from w in baseShapeWires; if not, fuse()
                                if baseface:
                                    baseface = baseface.fuse(f)
                                else:
                                    # TODO use Part.Shape() rather than shape.copy() ... ?
                                    baseface = f.copy()
        elif length and width and height:
            # check if this was a profil based arch structure
            # profile-based structures should use XY plane orientation
            use_profile_orientation = hasattr(obj, "Profile") and obj.Profile

            if (length > height) and (IfcType in ["Beam", "Column"]):
                h2 = height / 2 or 0.5
                w2 = width / 2 or 0.5
                if use_profile_orientation:
                    v1 = Vector(-w2, -h2, 0)
                    v2 = Vector(w2, -h2, 0)
                    v3 = Vector(w2, h2, 0)
                    v4 = Vector(-w2, h2, 0)
                else:
                    v1 = Vector(0, -w2, -h2)
                    v2 = Vector(0, w2, -h2)
                    v3 = Vector(0, w2, h2)
                    v4 = Vector(0, -w2, h2)
            else:
                l2 = length / 2 or 0.5
                w2 = width / 2 or 0.5
                v1 = Vector(-l2, -w2, 0)
                v2 = Vector(l2, -w2, 0)
                v3 = Vector(l2, w2, 0)
                v4 = Vector(-l2, w2, 0)
            import Part

            baseface = Part.Face(Part.makePolygon([v1, v2, v3, v4, v1]))
        if baseface:
            if hasattr(obj, "Tool") and obj.Tool:
                tool = obj.Tool
                edges = DraftGeomUtils.get_referenced_edges(tool)
                if len(edges) > 0:
                    extrusion = Part.Wire(Part.__sortEdges__(edges))
                    if hasattr(obj, "ToolOffsetFirst"):
                        offset_start = float(obj.ToolOffsetFirst.getValueAs("mm"))
                    else:
                        offset_start = 0.0
                    if hasattr(obj, "ToolOffsetLast"):
                        offset_end = float(obj.ToolOffsetLast.getValueAs("mm"))
                    else:
                        offset_end = 0.0
                    if offset_start != 0.0 or offset_end != 0.0:
                        extrusion = DraftGeomUtils.get_extended_wire(
                            extrusion, offset_start, offset_end
                        )
                    if hasattr(obj, "BasePerpendicularToTool") and obj.BasePerpendicularToTool:
                        pl = FreeCAD.Placement()
                        if hasattr(obj, "BaseRotation"):
                            pl.rotate(
                                FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), -obj.BaseRotation
                            )
                        if hasattr(obj, "BaseOffsetX") and hasattr(obj, "BaseOffsetY"):
                            pl.translate(FreeCAD.Vector(obj.BaseOffsetX, obj.BaseOffsetY, 0))
                        if hasattr(obj, "BaseMirror") and obj.BaseMirror:
                            pl.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1, 0), 180)
                        baseface.Placement = DraftGeomUtils.get_placement_perpendicular_to_wire(
                            extrusion
                        ).multiply(pl)
            else:
                if obj.Normal.Length:
                    normal = Vector(obj.Normal).normalize()
                else:
                    normal = baseface.Faces[0].normalAt(
                        0, 0
                    )  # TODO to use ArchSketch's 'normal' for consistency
            base = None
            placement = None
            inverse_placement = None
            if len(baseface.Faces) > 1:
                base = []
                placement = []
                hint = baseface.Faces[0].normalAt(0, 0)  # TODO anything to do ?
                for f in baseface.Faces:
                    bf, pf = self.rebase(f, hint)
                    base.append(bf)
                    placement.append(pf)
                inverse_placement = placement[0].inverse()
            else:
                base, placement = self.rebase(baseface)
                inverse_placement = placement.inverse()
            if extrusion:
                if (
                    len(extrusion.Edges) == 1
                    and DraftGeomUtils.geomType(extrusion.Edges[0]) == "Line"
                ):
                    extrusion = DraftGeomUtils.vec(extrusion.Edges[0], True)
                if isinstance(extrusion, FreeCAD.Vector):
                    extrusion = inverse_placement.Rotation.multVec(extrusion)
            elif normal:
                normal = inverse_placement.Rotation.multVec(normal)
                if not normal:
                    normal = Vector(0, 0, 1)
                if not normal.Length:
                    normal = Vector(0, 0, 1)
                extrusion = normal
                if (length > height) and (IfcType in ["Beam", "Column"]):
                    if length:
                        extrusion = normal.multiply(length)
                else:
                    if height:
                        extrusion = normal.multiply(height)
            if extrusion:
                # --- Multi-material layer support for slabs ---
                # FIX 1+2: Slope + Align + Offset for single-material slabs
                if (
                    getattr(obj, "IfcType", "") == "Slab"
                    and not isinstance(base, list)
                    and isinstance(extrusion, FreeCAD.Vector)
                    and not (hasattr(obj, "Tool") and obj.Tool)
                ):
                    import math

                    layers = self.get_layers(obj)
                    unit_n = FreeCAD.Vector(extrusion).normalize()

                    # ── Resolve slope ──────────────────────────────────────────
                    # Works for both multi-material AND single-material slabs.
                    slope_angle = getattr(obj, "Slope", 0)
                    if hasattr(slope_angle, "Value"):
                        slope_angle = slope_angle.Value

                    slope_base = None
                    if abs(slope_angle) > 1e-6:
                        x_axis = None
                        slope_edge_link = getattr(obj, "SlopeEdge", None)
                        if (
                            slope_edge_link
                            and len(slope_edge_link) >= 2
                            and slope_edge_link[0]
                            and slope_edge_link[1]
                        ):
                            try:
                                linked_obj = slope_edge_link[0]
                                sub_names = slope_edge_link[1]
                                sub_name = sub_names[0] if sub_names else None
                                if sub_name and hasattr(linked_obj, "Shape"):
                                    edge = linked_obj.Shape.getElement(sub_name)
                                    x_axis = edge.tangentAt(
                                        edge.FirstParameter
                                    ).normalize()
                            except Exception:
                                x_axis = None

                        if x_axis is None:
                            try:
                                outer_wire = base.Wires[0]
                                x_axis = outer_wire.Edges[0].tangentAt(
                                    outer_wire.Edges[0].FirstParameter
                                ).normalize()
                            except Exception:
                                x_axis = FreeCAD.Vector(1, 0, 0)

                        try:
                            pivot = base.CenterOfMass
                            rot = FreeCAD.Rotation(x_axis, slope_angle)
                            t1 = FreeCAD.Matrix()
                            t1.move(-pivot)
                            r = rot.toMatrix()
                            t2 = FreeCAD.Matrix()
                            t2.move(pivot)
                            slope_mat = t2.multiply(r.multiply(t1))
                            slope_base = base.transformGeometry(slope_mat)
                        except Exception:
                            slope_base = None

                    working_base = slope_base if slope_base is not None else base

                    # ── Multi-material path ────────────────────────────────────
                    if layers:
                        z_offset = self._compute_z_offset(obj, layers)
                        bases_list, extrusions_list, placements_list = [], [], []
                        cum_offset = z_offset

                        for layer in layers:
                            if layer > 0:
                                layer_face = working_base.copy()
                                layer_face.translate(unit_n * cum_offset)
                                bases_list.append(layer_face)
                                extrusions_list.append(unit_n * layer)
                                placements_list.append(placement)
                            cum_offset += abs(layer)

                        if bases_list:
                            return (bases_list, extrusions_list, placements_list)

                    # ── Single-material path ───────────────────────────────────
                    # Apply Align and Offset even without layers.
                    else:
                        total = extrusion.Length   # = obj.Height for a slab
                        z_offset = 0.0

                        # Global alignment (Bottom/Center/Top)
                        align = getattr(obj, "Align", "Bottom")
                        if align == "Center":
                            z_offset = -total / 2.0
                        elif align == "Top":
                            z_offset = -total

                        # Manual offset added on top
                        manual_offset = getattr(obj, "Offset", None)
                        if manual_offset is not None:
                            val = manual_offset.Value if hasattr(manual_offset, "Value") else 0.0
                            z_offset += val

                        if abs(z_offset) > 1e-6 or slope_base is not None:
                            # Apply z_offset translation to working_base
                            final_base = working_base.copy()
                            if abs(z_offset) > 1e-6:
                                final_base.translate(unit_n * z_offset)
                            return (final_base, extrusion, placement)
                # --- End multi-material layer block ---

                return (base, extrusion, placement)
        return None

    def onChanged(self, obj, prop):

        # check the flag indicating if we are currently in the process of
        # restoring document; if not, no further code is run as getExtrusionData()
        # below return error when some properties are not added by onDocumentRestored()
        if FreeCAD.ActiveDocument.Restoring:
            return

        if hasattr(obj, "IfcType"):
            IfcType = obj.IfcType
        else:
            IfcType = None
        self.hideSubobjects(obj, prop)
        if prop in ["Shape", "ResetNodes", "NodesOffset"]:
            # ResetNodes is not a property but it allows us to use this function to force reset the nodes
            nodes = None
            extdata = self.getExtrusionData(obj)
            if extdata and not isinstance(extdata[0], list):
                nodes = extdata[0]
                if IfcType in ["Beam", "Column"]:
                    if not isinstance(extdata[1], FreeCAD.Vector):
                        nodes = extdata[1]
                    elif extdata[1].Length > 0:
                        if hasattr(nodes, "CenterOfMass"):
                            import Part

                            nodes = Part.LineSegment(
                                nodes.CenterOfMass, nodes.CenterOfMass.add(extdata[1])
                            ).toShape()
                if isinstance(extdata[1], FreeCAD.Vector):
                    nodes.Placement = nodes.Placement.multiply(extdata[2])
            offset = FreeCAD.Vector()
            if hasattr(obj, "NodesOffset"):
                offset = FreeCAD.Vector(0, 0, obj.NodesOffset.Value)
            if obj.Nodes and (prop != "ResetNodes"):
                if hasattr(self, "nodes"):
                    if self.nodes:
                        if obj.Nodes != self.nodes:
                            # nodes are set manually: don't touch them
                            return
                else:
                    # nodes haven't been calculated yet, but are set (file load)
                    # we set the nodes now but don't change the property
                    if nodes:
                        self.nodes = [v.Point.add(offset) for v in nodes.Vertexes]
                        return
            # we set the nodes
            if nodes:
                self.nodes = [v.Point.add(offset) for v in nodes.Vertexes]
                obj.Nodes = self.nodes

        # --- Update AlignLayer enum when material or type changes ---
        if prop in ["Material", "IfcType"]:
            self._update_align_layer_enum(obj)

        # --- Slab-only property visibility ---
        is_slab = getattr(obj, "IfcType", "") == "Slab"

        for p in ["Align", "AlignLayer", "AlignLayerMode", "Offset", "Slope", "SlopeEdge"]:
            if hasattr(obj, p):
                obj.setEditorMode(p, 0 if is_slab else 2)

        if is_slab:
            align_layer = getattr(obj, "AlignLayer", "None (use Align)")
            layer_active = bool(align_layer and align_layer != "None (use Align)")

            if hasattr(obj, "Align"):
                obj.setEditorMode("Align", ["ReadOnly"] if layer_active else 0)
            if hasattr(obj, "AlignLayerMode"):
                obj.setEditorMode("AlignLayerMode", 0 if layer_active else 2)

            if hasattr(obj, "Slope") and hasattr(obj, "SlopeEdge"):
                slope_val = obj.Slope.Value if hasattr(obj.Slope, "Value") else 0.0
                obj.setEditorMode("SlopeEdge", 0 if abs(slope_val) > 1e-6 else 2)
        # --- End slab-only visibility ---

        ArchComponent.Component.onChanged(self, obj, prop)

        if prop == "ArchSketchPropertySet" and Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy, "getPropertySet"):
                uuid = baseProxy.getPropertySet(obj, propSetName=obj.ArchSketchPropertySet)
                self.ArchSkPropSetPickedUuid = uuid
        if (
            hasattr(obj, "ArchSketchData")
            and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"
        ):
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
        else:
            if hasattr(obj, "ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj, "ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])

    def getNodeEdges(self, obj):
        "returns a list of edges from structural nodes"

        edges = []
        if obj.Nodes:
            import Part

            for i in range(len(obj.Nodes) - 1):
                edges.append(
                    Part.LineSegment(
                        obj.Placement.multVec(obj.Nodes[i]), obj.Placement.multVec(obj.Nodes[i + 1])
                    ).toShape()
                )
            if hasattr(obj.ViewObject, "NodeType"):
                if (obj.ViewObject.NodeType == "Area") and (len(obj.Nodes) > 2):
                    edges.append(
                        Part.LineSegment(
                            obj.Placement.multVec(obj.Nodes[-1]),
                            obj.Placement.multVec(obj.Nodes[0]),
                        ).toShape()
                    )
        return edges


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self, vobj):

        ArchComponent.ViewProviderComponent.__init__(self, vobj)

        # setProperties of ArchComponent will be overwritten
        # thus setProperties from ArchComponent will be explicit called to get the properties
        ArchComponent.ViewProviderComponent.setProperties(self, vobj)

        self.setProperties(vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Structure")

    def setProperties(self, vobj):

        pl = vobj.PropertiesList
        if not "ShowNodes" in pl:
            vobj.addProperty(
                "App::PropertyBool",
                "ShowNodes",
                "Nodes",
                QT_TRANSLATE_NOOP("App::Property", "If the nodes are visible or not"),
                locked=True,
            ).ShowNodes = False
        if not "NodeLine" in pl:
            vobj.addProperty(
                "App::PropertyFloat",
                "NodeLine",
                "Nodes",
                QT_TRANSLATE_NOOP("App::Property", "The width of the nodes line"),
                locked=True,
            )
        if not "NodeSize" in pl:
            vobj.addProperty(
                "App::PropertyFloat",
                "NodeSize",
                "Nodes",
                QT_TRANSLATE_NOOP("App::Property", "The size of the node points"),
                locked=True,
            )
            vobj.NodeSize = 6
        if not "NodeColor" in pl:
            vobj.addProperty(
                "App::PropertyColor",
                "NodeColor",
                "Nodes",
                QT_TRANSLATE_NOOP("App::Property", "The color of the nodes line"),
                locked=True,
            )
            vobj.NodeColor = (1.0, 1.0, 1.0, 1.0)
        if not "NodeType" in pl:
            vobj.addProperty(
                "App::PropertyEnumeration",
                "NodeType",
                "Nodes",
                QT_TRANSLATE_NOOP("App::Property", "The type of structural node"),
                locked=True,
            )
            vobj.NodeType = ["Linear", "Area"]

    def onDocumentRestored(self, vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc

        if hasattr(self, "Object"):
            if hasattr(self.Object, "CloneOf"):
                if self.Object.CloneOf:
                    return ":/icons/Arch_Structure_Clone.svg"
        return ":/icons/Arch_Structure_Tree.svg"

    def updateData(self, obj, prop):

        if prop == "Nodes":
            if obj.Nodes:
                if hasattr(self, "nodes"):
                    p = []
                    self.pointset.numPoints.setValue(0)
                    self.lineset.coordIndex.deleteValues(0)
                    self.faceset.coordIndex.deleteValues(0)
                    for n in obj.Nodes:
                        p.append([n.x, n.y, n.z])
                    self.coords.point.setValues(0, len(p), p)
                    self.pointset.numPoints.setValue(len(p))
                    self.lineset.coordIndex.setValues(0, len(p) + 1, list(range(len(p))) + [-1])
                    if hasattr(obj.ViewObject, "NodeType"):
                        if (obj.ViewObject.NodeType == "Area") and (len(p) > 2):
                            self.coords.point.set1Value(len(p), p[0][0], p[0][1], p[0][2])
                            self.lineset.coordIndex.setValues(
                                0, len(p) + 2, list(range(len(p) + 1)) + [-1]
                            )
                            self.faceset.coordIndex.setValues(
                                0, len(p) + 1, list(range(len(p))) + [-1]
                            )

        elif prop in ["IfcType"]:
            if hasattr(obj.ViewObject, "NodeType"):
                if hasattr(obj, "IfcType"):
                    IfcType = obj.IfcType
                else:
                    IfcType = None
                if IfcType == "Slab":
                    obj.ViewObject.NodeType = "Area"
                else:
                    obj.ViewObject.NodeType = "Linear"
        else:
            # Per-layer material colors for slabs (mirrors _ViewProviderWall)
            if prop in ["Placement", "Shape", "Material"]:
                if hasattr(obj, "Material") and obj.Material and obj.Shape:
                    if hasattr(obj.Material, "Materials"):
                        activematerials = [
                            obj.Material.Materials[i]
                            for i in range(len(obj.Material.Materials))
                            if obj.Material.Thicknesses[i] >= 0
                        ]
                        if len(activematerials) == len(obj.Shape.Solids):
                            cols = []
                            for i, mat in enumerate(activematerials):
                                c = obj.ViewObject.ShapeColor
                                c = (
                                    c[0],
                                    c[1],
                                    c[2],
                                    1.0 - obj.ViewObject.Transparency / 100.0,
                                )
                                if "DiffuseColor" in mat.Material:
                                    if "(" in mat.Material["DiffuseColor"]:
                                        c = tuple(
                                            float(f)
                                            for f in mat.Material["DiffuseColor"]
                                            .strip("()")
                                            .split(",")
                                        )
                                if "Transparency" in mat.Material:
                                    c = (
                                        c[0],
                                        c[1],
                                        c[2],
                                        1.0 - float(mat.Material["Transparency"]),
                                    )
                                cols.extend(
                                    [c for _ in range(len(obj.Shape.Solids[i].Faces))]
                                )
                            obj.ViewObject.DiffuseColor = cols
            ArchComponent.ViewProviderComponent.updateData(self, obj, prop)
            if len(obj.ViewObject.DiffuseColor) > 1:
                obj.ViewObject.DiffuseColor = obj.ViewObject.DiffuseColor

    def onChanged(self, vobj, prop):

        if prop == "ShowNodes":
            if hasattr(self, "nodes"):
                vobj.Annotation.removeChild(self.nodes)
                del self.nodes
            if vobj.ShowNodes:
                from pivy import coin

                self.nodes = coin.SoAnnotation()
                self.coords = coin.SoCoordinate3()
                self.mat = coin.SoMaterial()
                self.pointstyle = coin.SoDrawStyle()
                self.pointstyle.style = coin.SoDrawStyle.POINTS
                self.pointset = coin.SoType.fromName("SoBrepPointSet").createInstance()
                self.linestyle = coin.SoDrawStyle()
                self.linestyle.style = coin.SoDrawStyle.LINES
                self.lineset = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
                self.facestyle = coin.SoDrawStyle()
                self.facestyle.style = coin.SoDrawStyle.FILLED
                self.shapehints = coin.SoShapeHints()
                self.shapehints.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
                self.fmat = coin.SoMaterial()
                self.fmat.transparency.setValue(0.75)
                self.faceset = coin.SoIndexedFaceSet()
                self.nodes.addChild(self.coords)
                self.nodes.addChild(self.mat)
                self.nodes.addChild(self.pointstyle)
                self.nodes.addChild(self.pointset)
                self.nodes.addChild(self.linestyle)
                self.nodes.addChild(self.lineset)
                self.nodes.addChild(self.facestyle)
                self.nodes.addChild(self.shapehints)
                self.nodes.addChild(self.fmat)
                self.nodes.addChild(self.faceset)
                vobj.Annotation.addChild(self.nodes)
                self.updateData(vobj.Object, "Nodes")
                self.onChanged(vobj, "NodeColor")
                self.onChanged(vobj, "NodeLine")
                self.onChanged(vobj, "NodeSize")

        elif prop == "NodeColor":
            if hasattr(self, "mat"):
                l = vobj.NodeColor
                self.mat.diffuseColor.setValue([l[0], l[1], l[2]])
                self.fmat.diffuseColor.setValue([l[0], l[1], l[2]])

        elif prop == "NodeLine":
            if hasattr(self, "linestyle"):
                self.linestyle.lineWidth = vobj.NodeLine

        elif prop == "NodeSize":
            if hasattr(self, "pointstyle"):
                self.pointstyle.pointSize = vobj.NodeSize

        elif prop == "NodeType":
            self.updateData(vobj.Object, "Nodes")

        else:
            ArchComponent.ViewProviderComponent.onChanged(self, vobj, prop)

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = StructureTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True


class StructureTaskPanel(ArchComponent.ComponentOptionsTaskPanel):
    """A task panel for Arch Structures that combines generic dimensions with node tools"""

    def __init__(self, obj):
        # FIX 5: Slab task panel shows Slope + Align controls
        # Define properties based on the IfcType
        if getattr(obj, "IfcType", "Beam") == "Slab":
            property_definitions = [
                {"prop": "Height", "label": translate("Arch", "Thickness")},
                {"prop": "Slope",  "label": translate("Arch", "Slope (°)")},
            ]
        else:
            # For Beams and Columns
            property_definitions = [
                {"prop": "Length", "label": translate("Arch", "Length")},
                {"prop": "Width",  "label": translate("Arch", "Width")},
                {"prop": "Height", "label": translate("Arch", "Height")},
            ]

        # Initialize generic parent (creates self.options_widget and self.baseform)
        super().__init__(obj, property_definitions)

        # Alias for compatibility with node/tool methods
        self.Object = self.obj

        self.nodes_widget = QtGui.QWidget()
        self.nodes_widget.setWindowTitle(QtGui.QApplication.translate("Arch", "Node Tools", None))
        lay = QtGui.QVBoxLayout(self.nodes_widget)

        self.resetButton = QtGui.QPushButton(self.nodes_widget)
        self.resetButton.setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))
        self.resetButton.setText(QtGui.QApplication.translate("Arch", "Reset Nodes", None))
        lay.addWidget(self.resetButton)
        self.resetButton.clicked.connect(self.resetNodes)

        self.editButton = QtGui.QPushButton(self.nodes_widget)
        self.editButton.setIcon(QtGui.QIcon(":/icons/Draft_Edit.svg"))
        self.editButton.setText(QtGui.QApplication.translate("Arch", "Edit Nodes", None))
        lay.addWidget(self.editButton)
        self.editButton.clicked.connect(self.editNodes)

        self.extendButton = QtGui.QPushButton(self.nodes_widget)
        self.extendButton.setIcon(QtGui.QIcon(":/icons/Snap_Perpendicular.svg"))
        self.extendButton.setText(QtGui.QApplication.translate("Arch", "Extend Nodes", None))
        self.extendButton.setToolTip(
            QtGui.QApplication.translate(
                "Arch",
                "Extends the nodes of this element to reach the nodes of another element",
                None,
            )
        )
        lay.addWidget(self.extendButton)
        self.extendButton.clicked.connect(self.extendNodes)

        self.connectButton = QtGui.QPushButton(self.nodes_widget)
        self.connectButton.setIcon(QtGui.QIcon(":/icons/Snap_Intersection.svg"))
        self.connectButton.setText(QtGui.QApplication.translate("Arch", "Connect Nodes", None))
        self.connectButton.setToolTip(
            QtGui.QApplication.translate(
                "Arch", "Connects nodes of this element with the nodes of another element", None
            )
        )
        lay.addWidget(self.connectButton)
        self.connectButton.clicked.connect(self.connectNodes)

        self.toggleButton = QtGui.QPushButton(self.nodes_widget)
        self.toggleButton.setIcon(QtGui.QIcon(":/icons/dagViewVisible.svg"))
        self.toggleButton.setText(QtGui.QApplication.translate("Arch", "Toggle All Nodes", None))
        self.toggleButton.setToolTip(
            QtGui.QApplication.translate(
                "Arch", "Toggles all structural nodes of the document on/off", None
            )
        )
        lay.addWidget(self.toggleButton)
        self.toggleButton.clicked.connect(self.toggleNodes)

        self.extrusion_widget = QtGui.QWidget()
        self.extrusion_widget.setWindowTitle(
            QtGui.QApplication.translate("Arch", "Extrusion Tools", None)
        )
        lay = QtGui.QVBoxLayout(self.extrusion_widget)

        self.selectToolButton = QtGui.QPushButton(self.extrusion_widget)
        self.selectToolButton.setIcon(QtGui.QIcon())
        self.selectToolButton.setText(QtGui.QApplication.translate("Arch", "Select Tool", None))
        self.selectToolButton.setToolTip(
            QtGui.QApplication.translate(
                "Arch", "Selects object or edges to be used as a tool (extrusion path)", None
            )
        )
        lay.addWidget(self.selectToolButton)
        self.selectToolButton.clicked.connect(self.setSelectionFromTool)

        # FIX 4: Slab-specific widget: Slope Edge picker
        form_widgets = [self.options_widget, self.nodes_widget,
                        self.extrusion_widget, self.baseform]

        if getattr(obj, "IfcType", "Beam") == "Slab":
            self.slab_widget = QtGui.QWidget()
            self.slab_widget.setWindowTitle(
                QtGui.QApplication.translate("Arch", "Slab Tools", None))
            slab_lay = QtGui.QVBoxLayout(self.slab_widget)

            # Label explaining how SlopeEdge works
            info = QtGui.QLabel(
                QtGui.QApplication.translate(
                    "Arch",
                    "Select an edge in the 3D view, then click\n"
                    "'Set Slope Edge' to define the drainage pivot.",
                    None))
            info.setWordWrap(True)
            slab_lay.addWidget(info)

            self.setSlopeEdgeButton = QtGui.QPushButton(self.slab_widget)
            self.setSlopeEdgeButton.setIcon(QtGui.QIcon(":/icons/Snap_Endpoint.svg"))
            self.setSlopeEdgeButton.setText(
                QtGui.QApplication.translate("Arch", "Set Slope Edge", None))
            self.setSlopeEdgeButton.setToolTip(
                QtGui.QApplication.translate(
                    "Arch",
                    "Select an edge in the 3D view first, then click here "
                    "to use it as the drainage slope pivot axis.",
                    None))
            slab_lay.addWidget(self.setSlopeEdgeButton)
            self.setSlopeEdgeButton.clicked.connect(self.setSlopeEdge)

            self.clearSlopeEdgeButton = QtGui.QPushButton(self.slab_widget)
            self.clearSlopeEdgeButton.setIcon(QtGui.QIcon(":/icons/edit-cleartext.svg"))
            self.clearSlopeEdgeButton.setText(
                QtGui.QApplication.translate("Arch", "Clear Slope Edge", None))
            self.clearSlopeEdgeButton.setToolTip(
                QtGui.QApplication.translate(
                    "Arch",
                    "Remove the slope edge reference. The first edge of the "
                    "base face will be used as the default pivot.",
                    None))
            slab_lay.addWidget(self.clearSlopeEdgeButton)
            self.clearSlopeEdgeButton.clicked.connect(self.clearSlopeEdge)

            # Current slope edge display
            self.slopeEdgeLabel = QtGui.QLabel("")
            self._update_slope_edge_label()
            slab_lay.addWidget(self.slopeEdgeLabel)

            form_widgets.insert(0, self.slab_widget)

        self.form = form_widgets

        self.observer = None
        self.nodevis = None

    def setSlopeEdge(self):
        """Read the current 3D selection and assign the first selected edge
        as the SlopeEdge property of the slab."""
        sel = FreeCADGui.Selection.getSelectionEx()
        for s in sel:
            for sub_name in s.SubElementNames:
                if sub_name.startswith("Edge"):
                    try:
                        self.Object.SlopeEdge = (s.Object, [sub_name])
                        self._update_slope_edge_label()
                        FreeCAD.ActiveDocument.recompute()
                        FreeCAD.Console.PrintMessage(
                            f"SlopeEdge set to {s.Object.Label} → {sub_name}\n"
                        )
                        return
                    except Exception as e:
                        FreeCAD.Console.PrintError(
                            f"Could not set SlopeEdge: {e}\n"
                        )
                        return
        FreeCAD.Console.PrintWarning(
            "No edge selected. Select an edge in the 3D view first.\n"
        )

    def clearSlopeEdge(self):
        """Remove the SlopeEdge reference. The first base-face edge is used."""
        try:
            self.Object.SlopeEdge = None
            self._update_slope_edge_label()
            FreeCAD.ActiveDocument.recompute()
            FreeCAD.Console.PrintMessage("SlopeEdge cleared.\n")
        except Exception as e:
            FreeCAD.Console.PrintError(f"Could not clear SlopeEdge: {e}\n")

    def _update_slope_edge_label(self):
        """Update the label showing the current SlopeEdge assignment."""
        if not hasattr(self, "slopeEdgeLabel"):
            return
        se = getattr(self.Object, "SlopeEdge", None)
        if se and len(se) >= 2 and se[0] and se[1]:
            try:
                label = f"Edge: {se[0].Label} → {se[1][0]}"
            except Exception:
                label = "Edge: (set)"
        else:
            label = "Edge: (not set — using first base edge)"
        self.slopeEdgeLabel.setText(label)

    def editNodes(self):

        FreeCADGui.Control.closeDialog()
        FreeCADGui.runCommand("Draft_Edit")

    def resetNodes(self):

        self.Object.Proxy.onChanged(self.Object, "ResetNodes")

    def extendNodes(self, other=None):

        if not other:
            self.observer = StructSelectionObserver(self.extendNodes)
            FreeCADGui.Selection.addObserver(self.observer)
            FreeCAD.Console.PrintMessage(translate("Arch", "Choose another Structure object:"))
        else:
            FreeCADGui.Selection.removeObserver(self.observer)
            self.observer = None
            if Draft.getType(other) != "Structure":
                FreeCAD.Console.PrintError(
                    translate("Arch", "The chosen object is not a Structure") + "\n"
                )
            else:
                if not other.Nodes:
                    FreeCAD.Console.PrintError(
                        translate("Arch", "The chosen object has no structural nodes") + "\n"
                    )
                else:
                    if (len(self.Object.Nodes) != 2) or (len(other.Nodes) != 2):
                        FreeCAD.Console.PrintError(
                            translate("Arch", "One of these objects has more than 2 nodes") + "\n"
                        )
                    else:
                        import DraftGeomUtils

                        nodes1 = [self.Object.Placement.multVec(v) for v in self.Object.Nodes]
                        nodes2 = [other.Placement.multVec(v) for v in other.Nodes]
                        intersect = DraftGeomUtils.findIntersection(
                            nodes1[0], nodes1[1], nodes2[0], nodes2[1], True, True
                        )
                        if not intersect:
                            FreeCAD.Console.PrintError(
                                translate("Arch", "Unable to find a suitable intersection point")
                                + "\n"
                            )
                        else:
                            intersect = intersect[0]
                            FreeCAD.Console.PrintMessage(translate("Arch", "Intersection found.\n"))
                            if DraftGeomUtils.findClosest(intersect, nodes1) == 0:
                                self.Object.Nodes = [
                                    self.Object.Placement.inverse().multVec(intersect),
                                    self.Object.Nodes[1],
                                ]
                            else:
                                self.Object.Nodes = [
                                    self.Object.Nodes[0],
                                    self.Object.Placement.inverse().multVec(intersect),
                                ]

    def connectNodes(self, other=None):

        if not other:
            self.observer = StructSelectionObserver(self.connectNodes)
            FreeCADGui.Selection.addObserver(self.observer)
            FreeCAD.Console.PrintMessage(translate("Arch", "Choose another Structure object:"))
        else:
            FreeCADGui.Selection.removeObserver(self.observer)
            self.observer = None
            if Draft.getType(other) != "Structure":
                FreeCAD.Console.PrintError(
                    translate("Arch", "The chosen object is not a Structure") + "\n"
                )
            else:
                if not other.Nodes:
                    FreeCAD.Console.PrintError(
                        translate("Arch", "The chosen object has no structural nodes") + "\n"
                    )
                else:
                    if (len(self.Object.Nodes) != 2) or (len(other.Nodes) != 2):
                        FreeCAD.Console.PrintError(
                            translate("Arch", "One of these objects has more than 2 nodes") + "\n"
                        )
                    else:
                        import DraftGeomUtils

                        nodes1 = [self.Object.Placement.multVec(v) for v in self.Object.Nodes]
                        nodes2 = [other.Placement.multVec(v) for v in other.Nodes]
                        intersect = DraftGeomUtils.findIntersection(
                            nodes1[0], nodes1[1], nodes2[0], nodes2[1], True, True
                        )
                        if not intersect:
                            FreeCAD.Console.PrintError(
                                translate("Arch", "Unable to find a suitable intersection point")
                                + "\n"
                            )
                        else:
                            intersect = intersect[0]
                            FreeCAD.Console.PrintMessage(
                                translate("Arch", "Intersection found.") + "\n"
                            )
                            if DraftGeomUtils.findClosest(intersect, nodes1) == 0:
                                self.Object.Nodes = [
                                    self.Object.Placement.inverse().multVec(intersect),
                                    self.Object.Nodes[1],
                                ]
                            else:
                                self.Object.Nodes = [
                                    self.Object.Nodes[0],
                                    self.Object.Placement.inverse().multVec(intersect),
                                ]
                            if DraftGeomUtils.findClosest(intersect, nodes2) == 0:
                                other.Nodes = [
                                    other.Placement.inverse().multVec(intersect),
                                    other.Nodes[1],
                                ]
                            else:
                                other.Nodes = [
                                    other.Nodes[0],
                                    other.Placement.inverse().multVec(intersect),
                                ]

    def toggleNodes(self):

        if self.nodevis:
            for obj in self.nodevis:
                obj[0].ViewObject.ShowNodes = obj[1]
            self.nodevis = None
        else:
            self.nodevis = []
            for obj in FreeCAD.ActiveDocument.Objects:
                if hasattr(obj.ViewObject, "ShowNodes"):
                    self.nodevis.append([obj, obj.ViewObject.ShowNodes])
                    obj.ViewObject.ShowNodes = True

    def setSelectionFromTool(self):
        FreeCADGui.Selection.clearSelection()
        if hasattr(self.Object, "Tool"):
            tool = self.Object.Tool
            if hasattr(tool, "Shape") and tool.Shape:
                FreeCADGui.Selection.addSelection(tool)
            else:
                if not isinstance(tool, list):
                    tool = [tool]
                for o, subs in tool:
                    FreeCADGui.Selection.addSelection(o, subs)
        QtCore.QObject.disconnect(
            self.selectToolButton, QtCore.SIGNAL("clicked()"), self.setSelectionFromTool
        )
        QtCore.QObject.connect(
            self.selectToolButton, QtCore.SIGNAL("clicked()"), self.setToolFromSelection
        )
        self.selectToolButton.setText(QtGui.QApplication.translate("Arch", "Done", None))

    def setToolFromSelection(self):
        objectList = []
        selEx = FreeCADGui.Selection.getSelectionEx()
        for selExi in selEx:
            if len(selExi.SubElementNames) == 0:
                # Add entirely selected objects
                objectList.append(selExi.Object)
            else:
                subElementsNames = [
                    subElementName
                    for subElementName in selExi.SubElementNames
                    if subElementName.startswith("Edge")
                ]
                # Check that at least an edge is selected from the object's shape
                if len(subElementsNames) > 0:
                    objectList.append((selExi.Object, subElementsNames))
        if self.Object.getTypeIdOfProperty("Tool") != "App::PropertyLinkSubList":
            # Upgrade property Tool from App::PropertyLink to App::PropertyLinkSubList (note: Undo/Redo fails)
            self.Object.removeProperty("Tool")
            self.Object.addProperty(
                "App::PropertyLinkSubList",
                "Tool",
                "Structure",
                QT_TRANSLATE_NOOP("App::Property", "An optional extrusion path for this element"),
                locked=True,
            )
        self.Object.Tool = objectList
        QtCore.QObject.disconnect(
            self.selectToolButton, QtCore.SIGNAL("clicked()"), self.setToolFromSelection
        )
        QtCore.QObject.connect(
            self.selectToolButton, QtCore.SIGNAL("clicked()"), self.setSelectionFromTool
        )
        self.selectToolButton.setText(QtGui.QApplication.translate("Arch", "Select Tool", None))

    def accept(self):
        if self.observer:
            FreeCADGui.Selection.removeObserver(self.observer)
        if self.nodevis:
            self.toggleNodes()

        # Trigger the generic property-saving logic in ComponentOptionsTaskPanel
        return super().accept()


class StructSelectionObserver:

    def __init__(self, callback):
        self.callback = callback

    def addSelection(self, docName, objName, sub, pos):
        print("got ", objName)
        obj = FreeCAD.getDocument(docName).getObject(objName)
        self.callback(obj)


class _StructuralSystem(
    ArchComponent.Component
):  # OBSOLETE - All Arch objects can now be based on axes
    "The Structural System object"

    def __init__(self, obj):

        ArchComponent.Component.__init__(self, obj)
        self.Type = "StructuralSystem"

        obj.addProperty(
            "App::PropertyLinkList",
            "Axes",
            "Arch",
            QT_TRANSLATE_NOOP("App::Property", "Axes systems this structure is built on"),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyIntegerList",
            "Exclude",
            "Arch",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The element numbers to exclude when this structure is based on axes",
            ),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyBool",
            "Align",
            "Arch",
            QT_TRANSLATE_NOOP("App::Property", "If true the element are aligned with axes"),
            locked=True,
        ).Align = False

    def loads(self, state):

        self.Type = "StructuralSystem"

    def execute(self, obj):
        "creates the structure shape"

        import Part
        import DraftGeomUtils

        # creating base shape
        pl = obj.Placement
        if obj.Base:
            if hasattr(obj.Base, "Shape"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.Solids:
                    return

                base = None

                # applying axes
                pts = self.getAxisPoints(obj)
                if hasattr(obj, "Align"):
                    if obj.Align == False:
                        apl = self.getAxisPlacement(obj)
                    if obj.Align:
                        apl = None
                else:
                    apl = self.getAxisPlacement(obj)

                if pts:
                    fsh = []
                    for i in range(len(pts)):
                        sh = obj.Base.Shape.copy()
                        if hasattr(obj, "Exclude"):
                            if i in obj.Exclude:
                                continue
                        if apl:
                            sh.Placement.Rotation = sh.Placement.Rotation.multiply(apl.Rotation)
                        sh.translate(pts[i])
                        fsh.append(sh)

                    if fsh:
                        base = Part.makeCompound(fsh)
                        base = self.processSubShapes(obj, base, pl)

                if base:
                    if not base.isNull():
                        if base.isValid() and base.Solids:
                            if base.Volume < 0:
                                base.reverse()
                            if base.Volume < 0:
                                FreeCAD.Console.PrintError(
                                    translate("Arch", "Could not compute a shape")
                                )
                                return
                            base = base.removeSplitter()
                            obj.Shape = base
                            if not pl.isNull():
                                obj.Placement = pl

    def getAxisPoints(self, obj):
        "returns the gridpoints of linked axes"
        import DraftGeomUtils

        pts = []
        if len(obj.Axes) == 1:
            if hasattr(obj, "Align"):
                if obj.Align:
                    p0 = obj.Axes[0].Shape.Edges[0].Vertexes[1].Point
                    for e in obj.Axes[0].Shape.Edges:
                        p = e.Vertexes[1].Point
                        p = p.sub(p0)
                        pts.append(p)
                else:
                    for e in obj.Axes[0].Shape.Edges:
                        pts.append(e.Vertexes[0].Point)
            else:
                for e in obj.Axes[0].Shape.Edges:
                    pts.append(e.Vertexes[0].Point)
        elif len(obj.Axes) >= 2:
            set1 = obj.Axes[0].Shape.Edges
            set2 = obj.Axes[1].Shape.Edges
            for e1 in set1:
                for e2 in set2:
                    pts.extend(DraftGeomUtils.findIntersection(e1, e2))
        return pts

    def getAxisPlacement(self, obj):
        "returns an axis placement"
        if obj.Axes:
            return obj.Axes[0].Placement
        return None


class _ViewProviderStructuralSystem(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structural System object"

    def getIcon(self):

        import Arch_rc

        return ":/icons/Arch_StructuralSystem_Tree.svg"


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("Arch_StructuralSystem", CommandStructuralSystem())
    FreeCADGui.addCommand("Arch_StructuresFromSelection", CommandStructuresFromSelection())

    class _ArchStructureGroupCommand:

        def GetCommands(self):
            return ("Arch_StructuralSystem", "Arch_StructuresFromSelection")

        def GetResources(self):
            return {
                "MenuText": QT_TRANSLATE_NOOP("Arch_StructureTools", "Structure Tools"),
                "ToolTip": QT_TRANSLATE_NOOP("Arch_StructureTools", "Structure tools"),
            }

        def IsActive(self):
            return not FreeCAD.ActiveDocument is None

    FreeCADGui.addCommand("Arch_StructureTools", _ArchStructureGroupCommand())