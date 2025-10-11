# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM wall command"""

import FreeCAD
import FreeCADGui
from enum import Enum

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

class WallBaselineMode(Enum):
    NONE = 0
    DRAFT_LINE = 1
    SKETCH = 2


class Arch_Wall:
    """The command definition for the Arch workbench's gui tool, Arch Wall.

    A tool for creating Arch walls.

    Create a wall from the object selected by the user. If no objects are
    selected, enter an interactive mode to create a wall using selected points
    to create a base.

    Find documentation on the end user usage of Arch Wall here:
    https://wiki.freecad.org/Arch_Wall
    """

    def GetResources(self):
        """Returns a dictionary with the visual aspects of the Arch Wall tool."""

        return {
            "Pixmap": "Arch_Wall",
            "MenuText": QT_TRANSLATE_NOOP("Arch_Wall", "Wall"),
            "Accel": "W, A",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_Wall",
                "Creates a wall object from scratch or from a selected object (wire, face or solid)",
            ),
        }

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        """Executed when Arch Wall is called.

        Creates a wall from the object selected by the user. If no objects are
        selected, enters an interactive mode to create a wall using selected
        points to create a base.
        """

        import Draft
        import WorkingPlane
        from draftutils import params
        import draftguitools.gui_trackers as DraftTrackers

        self.doc = FreeCAD.ActiveDocument
        self.Align = ["Center", "Left", "Right"][params.get_param_arch("WallAlignment")]
        self.MultiMat = None
        self.Length = None
        self.lengthValue = 0
        self.featureName = "Wall"
        self.Width = params.get_param_arch("WallWidth")
        self.Height = params.get_param_arch("WallHeight")
        self.Offset = params.get_param_arch("WallOffset")
        # Baseline creation mode (NONE / DRAFT_LINE / SKETCH)
        mode_index = PARAMS.GetInt("WallBaseline", 0)
        try:
            self.baseline_mode = WallBaselineMode(mode_index)
        except Exception:
            self.baseline_mode = WallBaselineMode.NONE
        self.AUTOJOIN = params.get_param_arch("autoJoinWalls")
        sel = FreeCADGui.Selection.getSelectionEx()
        self.existing = []
        self.wp = None

        if sel:
            # automatic mode
            if Draft.getType(sel[0].Object) != "Wall":
                self.doc.openTransaction(translate("Arch", "Create Wall"))
                FreeCADGui.addModule("Arch")
                for selobj in sel:
                    if (
                        Draft.getType(selobj.Object) == "Space"
                        and selobj.HasSubObjects
                        and "Face" in selobj.SubElementNames[0]
                    ):
                        idx = int(selobj.SubElementNames[0][4:])
                        FreeCADGui.doCommand(
                            "obj = Arch.makeWall(FreeCAD.ActiveDocument."
                            + selobj.Object.Name
                            + ",face="
                            + str(idx)
                            + ")"
                        )
                    else:
                        FreeCADGui.doCommand(
                            "obj = Arch.makeWall(FreeCAD.ActiveDocument." + selobj.Object.Name + ")"
                        )
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
                self.doc.commitTransaction()
                self.doc.recompute()
                return

        # interactive mode

        FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
        self.points = []
        self.wp = WorkingPlane.get_working_plane()
        self.tracker = DraftTrackers.boxTracker()
        FreeCADGui.Snapper.getPoint(
            callback=self.getPoint,
            extradlg=self.taskbox(),
            title=translate("Arch", "First point of wall"),
        )
        FreeCADGui.draftToolBar.continueCmd.show()

    def getPoint(self, point=None, obj=None):
        """Callback for clicks during interactive mode.

        When method _CommandWall.Activated() has entered the interactive mode,
        this callback runs when the user clicks.

        Parameters
        ----------
        point: <class 'Base.Vector'>
            The point the user has selected.
        obj: <Part::Feature>, optional
            The object the user's cursor snapped to, if any.
        """

        import Draft
        import ArchWall
        from draftutils import gui_utils

        if obj:
            if Draft.getType(obj) == "Wall":
                if not obj in self.existing:
                    self.existing.append(obj)
        if point is None:
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.tracker.finalize()
            return
        self.points.append(point)
        if len(self.points) == 1:
            self.tracker.width(self.Width)
            self.tracker.height(self.Height)
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(
                last=self.points[0],
                callback=self.getPoint,
                movecallback=self.update,
                extradlg=self.taskbox(),
                title=translate("Arch", "Next point"),
                mode="line",
            )

        elif len(self.points) == 2:
            self.create_wall()

    def _create_baseless_wall(self, p0, p1):
        """Creates a baseless wall and ensures all steps are macro-recordable."""

        line_vector = p1.sub(p0)
        length = line_vector.Length
        midpoint = (p0 + p1) * 0.5
        direction = line_vector.normalize()
        rotation = FreeCAD.Rotation(FreeCAD.Vector(1,0,0), direction)
        placement = FreeCAD.Placement(midpoint, rotation)

        wall_var = "new_baseless_wall"

        # Construct command strings
        placement_str = (f"FreeCAD.Placement(FreeCAD.Vector({placement.Base.x}, {placement.Base.y}, {placement.Base.z}), "
                         f"FreeCAD.Rotation({placement.Rotation.Q[0]}, {placement.Rotation.Q[1]}, {placement.Rotation.Q[2]}, {placement.Rotation.Q[3]}))")

        make_wall_cmd = (f"{wall_var} = Arch.makeWall(length={length}, width={self.Width}, "
                         f"height={self.Height}, align='{self.Align}')")

        # Execute creation and placement commands
        FreeCADGui.doCommand("import Arch")
        FreeCADGui.doCommand(make_wall_cmd)
        FreeCADGui.doCommand(f"{wall_var}.Placement = {placement_str}")

        # Get a reference to the newly created object to find its final name
        newly_created_wall = self.doc.Objects[-1]

        # Now, issue the autogroup command using the object's actual name
        FreeCADGui.doCommand("import Draft")
        FreeCADGui.doCommand(f"Draft.autogroup(FreeCAD.ActiveDocument.{newly_created_wall.Name})")

        return newly_created_wall.Name

    def _create_baseline_object(self, p0, p1):
        """Creates a baseline object (Draft line or Sketch) and returns its name."""

        placement = self.wp.get_placement()
        placement_str = (f"FreeCAD.Placement(FreeCAD.Vector({placement.Base.x}, {placement.Base.y}, {placement.Base.z}), "
                         f"FreeCAD.Rotation({placement.Rotation.Q[0]}, {placement.Rotation.Q[1]}, {placement.Rotation.Q[2]}, {placement.Rotation.Q[3]}))")

        trace_cmd = (f"trace = Part.LineSegment(FreeCAD.Vector({p0.x}, {p0.y}, {p0.z}), "
                     f"FreeCAD.Vector({p1.x}, {p1.y}, {p1.z}))")

        FreeCADGui.doCommand("import FreeCAD")
        FreeCADGui.doCommand("import Part")
        FreeCADGui.doCommand(trace_cmd)

        baseline_name = None
        if self.baseline_mode == WallBaselineMode.DRAFT_LINE:
            FreeCADGui.doCommand("import Draft")

            # Methodical object capture: get doc state before creation
            objects_before = set(self.doc.Objects)

            # Execute creation command without trying to set a name
            FreeCADGui.doCommand("base = Draft.make_line(trace)")

            # Find the newly created object by set difference
            objects_after = set(self.doc.Objects)
            new_objects = objects_after - objects_before

            if len(new_objects) == 1:
                new_line = new_objects.pop()
                baseline_name = new_line.Name
                # Now apply placement to the correctly identified object
                FreeCADGui.doCommand(f"FreeCAD.ActiveDocument.{baseline_name}.Placement = {placement_str}")

        elif self.baseline_mode == WallBaselineMode.SKETCH:
            baseline_name = "WallTrace"
            # This works because addObject() allows setting the name
            FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.addObject('Sketcher::SketchObject', '{baseline_name}')")
            FreeCADGui.doCommand(f"base.Placement = {placement_str}")
            FreeCADGui.doCommand("base.addGeometry(trace)")

        FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")

        if baseline_name:
            # Verify the object exists before returning its name
            obj = self.doc.getObject(baseline_name)
            return obj.Name if obj else None
        return None

    def _create_wall_from_baseline(self, baseline_name):
        """Creates a wall from a baseline object, ensuring all steps are macro-recordable."""

        if baseline_name is None:
            return None

        # Get a reference to the baseline object to ensure it exists, although we use its name in
        # the command
        base_obj = self.doc.getObject(baseline_name)
        if base_obj is None:
            return None

        # Use a unique variable name for the wall in the command string
        wall_var = "new_wall_from_base"

        # Construct command strings
        make_wall_cmd = (f"{wall_var} = Arch.makeWall(FreeCAD.ActiveDocument.{baseline_name}, "
                         f"width={self.Width}, height={self.Height}, align='{self.Align}')")
        set_normal_cmd = f"{wall_var}.Normal = FreeCAD.{self.wp.axis}"

        # Execute creation and property-setting commands
        FreeCADGui.doCommand("import Arch")
        FreeCADGui.doCommand(make_wall_cmd)
        FreeCADGui.doCommand(set_normal_cmd)

        # Get a reference to the newly created object to find its final name
        newly_created_wall = self.doc.Objects[-1]

        # Now, issue the autogroup command using the object's actual name
        FreeCADGui.doCommand("import Draft")
        FreeCADGui.doCommand(f"Draft.autogroup(FreeCAD.ActiveDocument.{newly_created_wall.Name})")

        return newly_created_wall.Name

    def create_wall(self):
        """Orchestrate wall creation according to the baseline mode."""
        import Arch

        p0 = self.wp.get_local_coords(self.points[0])
        p1 = self.wp.get_local_coords(self.points[1])

        self.tracker.off()
        FreeCAD.activeDraftCommand = None
        FreeCADGui.Snapper.off()

        self.doc.openTransaction(translate("Arch", "Create Wall"))

        # Ensure baseline_mode is initialized (some tests call create_wall()
        # directly without going through Activated()).
        if not hasattr(self, 'baseline_mode'):
            try:
                self.baseline_mode = WallBaselineMode(PARAMS.GetInt("WallBaseline", 0))
            except Exception:
                self.baseline_mode = WallBaselineMode.NONE

        if self.baseline_mode == WallBaselineMode.NONE:
            wall_name = self._create_baseless_wall(p0, p1)
            # If autojoin is requested and something was snapped to, add components
            if self.existing and self.AUTOJOIN and wall_name:
                FreeCADGui.doCommand(f"Arch.addComponents(FreeCAD.ActiveDocument.{wall_name}, FreeCAD.ActiveDocument.{self.existing[0].Name})")
        else:
            baseline_name = self._create_baseline_object(p0, p1)
            if baseline_name and self.baseline_mode == WallBaselineMode.SKETCH and self.existing:
                # try to join existing sketches first
                joined = Arch.joinWalls(self.existing)
                if joined:
                    # add trace geometry to joined sketch
                    try:
                        FreeCADGui.doCommand(f"FreeCAD.ActiveDocument.{joined.Name}.Base.addGeometry(trace)")
                        FreeCADGui.doCommand(f"FreeCAD.ActiveDocument.removeObject('{baseline_name}')")
                    except Exception:
                        # fallback: create a wall from the baseline
                        self._create_wall_from_baseline(baseline_name)
                else:
                    self._create_wall_from_baseline(baseline_name)
            else:
                wall_name = self._create_wall_from_baseline(baseline_name)
                if self.existing and self.AUTOJOIN and wall_name:
                    FreeCADGui.doCommand(f"Arch.addComponents(FreeCAD.ActiveDocument.{wall_name}, FreeCAD.ActiveDocument.{self.existing[0].Name})")

        self.doc.commitTransaction()
        self.doc.recompute()
        self.tracker.finalize()
        if FreeCADGui.draftToolBar.continueMode:
            self.Activated()

    def update(self, point, info):
        # info parameter is not used but needed for compatibility with the snapper

        """Callback for the mouse moving during the interactive mode.

        Update the active dialog box to show the coordinates of the location of
        the cursor. Also show the length the line would take, if the user
        selected that point.

        Parameters
        ----------
        point: <class 'Base.Vector'>
            The point the cursor is currently at, or has snapped to.
        """

        import DraftVecUtils

        if FreeCADGui.Control.activeDialog():
            b = self.points[0]
            n = self.wp.axis
            bv = point.sub(b)
            dv = bv.cross(n)
            dv = DraftVecUtils.scaleTo(dv, self.Width / 2)
            if self.Align == "Center":
                self.tracker.update([b, point])
            elif self.Align == "Left":
                self.tracker.update([b.add(dv), point.add(dv)])
            else:
                dv = dv.negative()
                self.tracker.update([b.add(dv), point.add(dv)])
            if self.Length:
                self.Length.setText(
                    FreeCAD.Units.Quantity(bv.Length, FreeCAD.Units.Length).UserString
                )

    def taskbox(self):
        """Set up a simple gui widget for the interactive mode."""

        from PySide import QtCore, QtGui
        import Draft
        from draftutils import params

        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch", "Wall options"))
        grid = QtGui.QGridLayout(w)

        # Wall presets input
        comboWallPresets = QtGui.QComboBox()
        comboWallPresets.addItem(translate("Arch", "Wall Presets"))
        comboWallPresets.setToolTip(
            translate(
                "Arch",
                "This list shows all the MultiMaterials objects of this document. Create some to define wall types.",
            )
        )
        self.multimats = []
        self.MultiMat = None
        for o in self.doc.Objects:
            if Draft.getType(o) == "MultiMaterial":
                self.multimats.append(o)
                comboWallPresets.addItem(o.Label)
        if hasattr(FreeCAD, "LastArchMultiMaterial"):
            for i, o in enumerate(self.multimats):
                if o.Name == FreeCAD.LastArchMultiMaterial:
                    comboWallPresets.setCurrentIndex(i + 1)
                    self.MultiMat = o
        grid.addWidget(comboWallPresets, 0, 0, 1, 2)

        # Wall length input
        labelLength = QtGui.QLabel(translate("Arch", "Length"))
        self.Length = ui.createWidget("Gui::InputField")
        self.Length.setText("0.00 mm")
        grid.addWidget(labelLength, 1, 0, 1, 1)
        grid.addWidget(self.Length, 1, 1, 1, 1)

        # Wall width input
        labelWidth = QtGui.QLabel(translate("Arch", "Width"))
        inputWidth = ui.createWidget("Gui::InputField")
        inputWidth.setText(FreeCAD.Units.Quantity(self.Width, FreeCAD.Units.Length).UserString)
        grid.addWidget(labelWidth, 2, 0, 1, 1)
        grid.addWidget(inputWidth, 2, 1, 1, 1)

        # Wall height input
        labelHeight = QtGui.QLabel(translate("Arch", "Height"))
        inputHeight = ui.createWidget("Gui::InputField")
        inputHeight.setText(FreeCAD.Units.Quantity(self.Height, FreeCAD.Units.Length).UserString)
        grid.addWidget(labelHeight, 3, 0, 1, 1)
        grid.addWidget(inputHeight, 3, 1, 1, 1)

        # Wall alignment input
        labelAlignment = QtGui.QLabel(translate("Arch", "Alignment"))
        comboAlignment = QtGui.QComboBox()
        items = [translate("Arch", "Center"), translate("Arch", "Left"), translate("Arch", "Right")]
        comboAlignment.addItems(items)
        comboAlignment.setCurrentIndex(["Center", "Left", "Right"].index(self.Align))
        grid.addWidget(labelAlignment, 4, 0, 1, 1)
        grid.addWidget(comboAlignment, 4, 1, 1, 1)

        # Wall offset input
        labelOffset = QtGui.QLabel(translate("Arch", "Offset"))
        inputOffset = ui.createWidget("Gui::InputField")
        inputOffset.setText(FreeCAD.Units.Quantity(self.Offset, FreeCAD.Units.Length).UserString)
        grid.addWidget(labelOffset, 5, 0, 1, 1)
        grid.addWidget(inputOffset, 5, 1, 1, 1)

        # Wall baseline dropdown
        labelBaseline = QtGui.QLabel(translate("Arch","Baseline"))
        comboBaseline = QtGui.QComboBox()
        comboBaseline.setObjectName("Baseline")
        labelBaseline.setBuddy(comboBaseline)
        comboBaseline.addItems([translate("Arch","No baseline"), translate("Arch","Draft line"), translate("Arch","Sketch")])
        comboBaseline.setCurrentIndex(PARAMS.GetInt("WallBaseline",0))
        grid.addWidget(labelBaseline,6,0,1,1)
        grid.addWidget(comboBaseline,6,1,1,1)

        # Enable/disable inputOffset based on inputAlignment
        def updateOffsetState(index):
            alignment = ["Center", "Left", "Right"][index]
            inputOffset.setEnabled(alignment in ["Left", "Right"])

        # Connect the signal
        comboAlignment.currentIndexChanged.connect(updateOffsetState)

        # Initialize the state of inputOffset
        updateOffsetState(comboAlignment.currentIndex())

        # Connect the signals to the slots for value changes
        self.Length.valueChanged.connect(self.setLength)
        inputWidth.valueChanged.connect(self.setWidth)
        inputHeight.valueChanged.connect(self.setHeight)
        comboAlignment.currentIndexChanged.connect(self.setAlign)
        inputOffset.valueChanged.connect(self.setOffset)
        comboBaseline.currentIndexChanged.connect(self.setBaseline)
        comboWallPresets.currentIndexChanged.connect(self.setMat)

        # Define the workflow of the input fields:
        # Pressing Enter will cycle through Length, Width, and
        # finally on Height the wall will be created
        self.Length.returnPressed.connect(inputWidth.setFocus)
        self.Length.returnPressed.connect(inputWidth.selectAll)
        inputWidth.returnPressed.connect(inputHeight.setFocus)
        inputWidth.returnPressed.connect(inputHeight.selectAll)
        inputHeight.returnPressed.connect(self.createFromGUI)
        inputOffset.returnPressed.connect(self.createFromGUI)

        return w

    def setMat(self, d):
        """Simple callback for the interactive mode gui widget to set material."""

        if d == 0:
            self.MultiMat = None
            del FreeCAD.LastArchMultiMaterial
        elif d <= len(self.multimats):
            self.MultiMat = self.multimats[d - 1]
            FreeCAD.LastArchMultiMaterial = self.MultiMat.Name

    def setLength(self, d):
        """Simple callback for the interactive mode gui widget to set length."""

        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.lengthValue = d

    def setWidth(self, d):
        """Simple callback for the interactive mode gui widget to set width."""

        from draftutils import params

        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Width = d
        self.tracker.width(d)
        params.set_param_arch("WallWidth", d)

    def setHeight(self, d):
        """Simple callback for the interactive mode gui widget to set height."""

        from draftutils import params

        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Height = d
        self.tracker.height(d)
        params.set_param_arch("WallHeight", d)

    def setAlign(self, i):
        """Simple callback for the interactive mode gui widget to set alignment."""

        from draftutils import params

        self.Align = ["Center", "Left", "Right"][i]
        params.set_param_arch("WallAlignment", i)

    def setOffset(self, d):
        """Simple callback for the interactive mode GUI widget to set offset."""

        from draftutils import params

        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Offset = d
        params.set_param_arch("WallOffset", d)

    def setBaseline(self,i):
        """Simple callback to set the wall baseline creation mode."""
        try:
            self.baseline_mode = WallBaselineMode(i)
        except Exception:
            self.baseline_mode = WallBaselineMode.NONE
        PARAMS.SetInt("WallBaseline",i)

    def createFromGUI(self):
        """Callback to create wall by using the _CommandWall.taskbox()"""

        self.doc.openTransaction(translate("Arch", "Create Wall"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand(
            "wall = Arch.makeWall(length="
            + str(self.lengthValue)
            + ",width="
            + str(self.Width)
            + ",height="
            + str(self.Height)
            + ',align="'
            + str(self.Align)
            + '")'
        )
        if self.MultiMat:
            FreeCADGui.doCommand("wall.Material = FreeCAD.ActiveDocument." + self.MultiMat.Name)
        self.doc.commitTransaction()
        self.doc.recompute()
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.escape()


FreeCADGui.addCommand("Arch_Wall", Arch_Wall())
