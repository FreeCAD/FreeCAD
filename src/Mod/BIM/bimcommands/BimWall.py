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

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


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

        return {'Pixmap'  : 'Arch_Wall',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Wall","Wall"),
                'Accel': "W, A",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Wall","Creates a wall object from scratch or from a selected object (wire, face or solid)")}

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
        self.Align = ["Center","Left","Right"][params.get_param_arch("WallAlignment")]
        self.MultiMat = None
        self.Length = None
        self.lengthValue = 0
        self.featureName = "Wall"
        self.Width = params.get_param_arch("WallWidth")
        self.Height = params.get_param_arch("WallHeight")
        self.Offset = params.get_param_arch("WallOffset")
        self.JOIN_WALLS_SKETCHES = params.get_param_arch("joinWallSketches")
        self.AUTOJOIN = params.get_param_arch("autoJoinWalls")
        sel = FreeCADGui.Selection.getSelectionEx()
        self.existing = []
        self.wp = None

        if sel:
            # automatic mode
            if Draft.getType(sel[0].Object) != "Wall":
                self.doc.openTransaction(translate("Arch","Create Wall"))
                FreeCADGui.addModule("Arch")
                for selobj in sel:
                    if Draft.getType(selobj.Object) == "Space" \
                            and selobj.HasSubObjects \
                            and "Face" in selobj.SubElementNames[0]:
                        idx = int(selobj.SubElementNames[0][4:])
                        FreeCADGui.doCommand("obj = Arch.makeWall(FreeCAD.ActiveDocument."+selobj.Object.Name+",face="+str(idx)+")")
                    else:
                        FreeCADGui.doCommand("obj = Arch.makeWall(FreeCAD.ActiveDocument."+selobj.Object.Name+")")
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
        FreeCADGui.Snapper.getPoint(callback=self.getPoint,
                                    extradlg=self.taskbox(),
                                    title=translate("Arch", "First point of wall"))
        FreeCADGui.draftToolBar.continueCmd.show()

    def getPoint(self,point=None,obj=None):
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
            FreeCADGui.Snapper.getPoint(last=self.points[0],
                                        callback=self.getPoint,
                                        movecallback=self.update,
                                        extradlg=self.taskbox(),
                                        title=translate("Arch", "Next point"),
                                        mode="line")

        elif len(self.points) == 2:
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.tracker.off()

            self.doc.openTransaction(translate("Arch", "Create Wall"))

            # Some code in gui_utils.autogroup requires a wall shape to determine
            # the target group. We therefore need to create a wall first.
            self.addDefault()
            wall = self.doc.Objects[-1]
            wallGrp = wall.getParentGroup()

            if (self.JOIN_WALLS_SKETCHES or self.AUTOJOIN) \
                    and self.existing \
                    and self.existing[-1].getParentGroup() == wallGrp:
                oldWall = self.existing[-1]
                if self.JOIN_WALLS_SKETCHES and ArchWall.areSameWallTypes([wall, oldWall]):
                    FreeCADGui.doCommand(
                        "Arch.joinWalls([wall, doc." + oldWall.Name + "], "
                        + "delete=True, deletebase=True)"
                    )
                elif self.AUTOJOIN:
                    if wallGrp is not None:
                        FreeCADGui.doCommand("wall.getParentGroup().removeObject(wall)")
                    FreeCADGui.doCommand("Arch.addComponents(wall, doc." + oldWall.Name + ")")

            self.doc.commitTransaction()
            self.doc.recompute()
            # gui_utils.end_all_events()  # Causes a crash on Linux.
            self.tracker.finalize()
            if FreeCADGui.draftToolBar.continueMode:
                self.Activated()

    def addDefault(self):
        """Create a wall using a line segment, with all parameters as the default.

        Used solely by _CommandWall.getPoint() when the interactive mode has
        selected two points.
        """
        from draftutils import params

        sta = self.wp.get_local_coords(self.points[0])
        end = self.wp.get_local_coords(self.points[1])

        FreeCADGui.doCommand("import Part")
        FreeCADGui.addModule("Draft")
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("doc = FreeCAD.ActiveDocument")
        FreeCADGui.doCommand("wp = WorkingPlane.get_working_plane()")
        FreeCADGui.doCommand(
            "trace = Part.LineSegment(FreeCAD." + str(sta) + ", FreeCAD." + str(end) + ")"
        )
        if params.get_param_arch("WallSketches"):
            # Use ArchSketch if SketchArch add-on is present
            try:
                import ArchSketchObject
                FreeCADGui.doCommand("import ArchSketchObject")
                FreeCADGui.doCommand("base = ArchSketchObject.makeArchSketch()")
            except:
                FreeCADGui.doCommand(
                    "base = doc.addObject(\"Sketcher::SketchObject\", \"WallTrace\")"
                )
            FreeCADGui.doCommand("base.Placement = wp.get_placement()")
            FreeCADGui.doCommand("base.addGeometry(trace)")
        else:
            FreeCADGui.doCommand("base = Draft.make_line(trace)")
            # The created line should not stay selected as this causes an issue in continue mode.
            # Two walls would then be created based on the same line.
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.doCommand("base.Placement = wp.get_placement()")
            FreeCADGui.doCommand("doc.recompute()")
        FreeCADGui.doCommand(
            "wall = Arch.makeWall(base, width=" + str(self.Width)
            + ", height=" + str(self.Height) + ", align=\"" + str(self.Align) + "\")"
        )
        FreeCADGui.doCommand("wall.Normal = wp.axis")
        if self.MultiMat:
            FreeCADGui.doCommand("wall.Material = doc." + self.MultiMat.Name)
        FreeCADGui.doCommand("doc.recompute()")  # required as some autogroup code requires the wall shape
        FreeCADGui.doCommand("Draft.autogroup(wall)")

    def update(self,point,info):
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
            dv = DraftVecUtils.scaleTo(dv,self.Width/2)
            if self.Align == "Center":
                self.tracker.update([b,point])
            elif self.Align == "Left":
                self.tracker.update([b.add(dv),point.add(dv)])
            else:
                dv = dv.negative()
                self.tracker.update([b.add(dv),point.add(dv)])
            if self.Length:
                self.Length.setText(FreeCAD.Units.Quantity(bv.Length,FreeCAD.Units.Length).UserString)

    def taskbox(self):
        """Set up a simple gui widget for the interactive mode."""

        from PySide import QtCore, QtGui
        import Draft
        from draftutils import params
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Wall options"))
        grid = QtGui.QGridLayout(w)

        # Wall presets input
        comboWallPresets = QtGui.QComboBox()
        comboWallPresets.addItem(translate("Arch","Wall Presets"))
        comboWallPresets.setToolTip(translate("Arch","This list shows all the MultiMaterials objects of this document. Create some to define wall types."))
        self.multimats = []
        self.MultiMat = None
        for o in self.doc.Objects:
            if Draft.getType(o) == "MultiMaterial":
                self.multimats.append(o)
                comboWallPresets.addItem(o.Label)
        if hasattr(FreeCAD,"LastArchMultiMaterial"):
            for i,o in enumerate(self.multimats):
                if o.Name == FreeCAD.LastArchMultiMaterial:
                    comboWallPresets.setCurrentIndex(i+1)
                    self.MultiMat = o
        grid.addWidget(comboWallPresets,0,0,1,2)

        # Wall length input
        labelLength = QtGui.QLabel(translate("Arch","Length"))
        self.Length = ui.createWidget("Gui::InputField")
        self.Length.setText("0.00 mm")
        grid.addWidget(labelLength,1,0,1,1)
        grid.addWidget(self.Length,1,1,1,1)

        # Wall width input
        labelWidth = QtGui.QLabel(translate("Arch","Width"))
        inputWidth = ui.createWidget("Gui::InputField")
        inputWidth.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(labelWidth,2,0,1,1)
        grid.addWidget(inputWidth,2,1,1,1)

        # Wall height input
        labelHeight = QtGui.QLabel(translate("Arch","Height"))
        inputHeight = ui.createWidget("Gui::InputField")
        inputHeight.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        grid.addWidget(labelHeight,3,0,1,1)
        grid.addWidget(inputHeight,3,1,1,1)

        # Wall alignment input
        labelAlignment = QtGui.QLabel(translate("Arch","Alignment"))
        comboAlignment = QtGui.QComboBox()
        items = [translate("Arch","Center"),translate("Arch","Left"),translate("Arch","Right")]
        comboAlignment.addItems(items)
        comboAlignment.setCurrentIndex(["Center","Left","Right"].index(self.Align))
        grid.addWidget(labelAlignment,4,0,1,1)
        grid.addWidget(comboAlignment,4,1,1,1)

        # Wall offset input
        labelOffset = QtGui.QLabel(translate("Arch", "Offset"))
        inputOffset = ui.createWidget("Gui::InputField")
        inputOffset.setText(FreeCAD.Units.Quantity(
            self.Offset,
            FreeCAD.Units.Length).UserString)
        grid.addWidget(labelOffset, 5, 0, 1, 1)
        grid.addWidget(inputOffset, 5, 1, 1, 1)

        # Wall "use sketches" checkbox
        labelUseSketches = QtGui.QLabel(translate("Arch","Use sketches"))
        checkboxUseSketches = QtGui.QCheckBox()
        checkboxUseSketches.setObjectName("UseSketches")
        checkboxUseSketches.setLayoutDirection(QtCore.Qt.RightToLeft)
        labelUseSketches.setBuddy(checkboxUseSketches)
        checkboxUseSketches.setChecked(params.get_param_arch("WallSketches"))
        grid.addWidget(labelUseSketches,6,0,1,1)
        grid.addWidget(checkboxUseSketches,6,1,1,1)

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
        if hasattr(checkboxUseSketches, "checkStateChanged"): # Qt version >= 6.7.0
            checkboxUseSketches.checkStateChanged.connect(self.setUseSketch)
        else: # Qt version < 6.7.0
            checkboxUseSketches.stateChanged.connect(self.setUseSketch)
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

    def setMat(self,d):
        """Simple callback for the interactive mode gui widget to set material."""

        if d == 0:
            self.MultiMat = None
            del FreeCAD.LastArchMultiMaterial
        elif d <= len(self.multimats):
            self.MultiMat = self.multimats[d-1]
            FreeCAD.LastArchMultiMaterial = self.MultiMat.Name

    def setLength(self,d):
        """Simple callback for the interactive mode gui widget to set length."""

        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.lengthValue = d

    def setWidth(self,d):
        """Simple callback for the interactive mode gui widget to set width."""

        from draftutils import params
        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Width = d
        self.tracker.width(d)
        params.set_param_arch("WallWidth",d)


    def setHeight(self,d):
        """Simple callback for the interactive mode gui widget to set height."""

        from draftutils import params
        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Height = d
        self.tracker.height(d)
        params.set_param_arch("WallHeight",d)

    def setAlign(self,i):
        """Simple callback for the interactive mode gui widget to set alignment."""

        from draftutils import params
        self.Align = ["Center","Left","Right"][i]
        params.set_param_arch("WallAlignment",i)

    def setOffset(self, d):
        """Simple callback for the interactive mode GUI widget to set offset."""

        from draftutils import params
        if isinstance(d, FreeCAD.Units.Quantity):
            d = d.Value
        self.Offset = d
        params.set_param_arch("WallOffset", d)

    def setUseSketch(self, i):
        """Simple callback to set if walls should based on sketches."""

        from draftutils import params
        params.set_param_arch("WallSketches",bool(getattr(i, "value", i)))

    def createFromGUI(self):
        """Callback to create wall by using the _CommandWall.taskbox()"""

        self.doc.openTransaction(translate("Arch","Create Wall"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand('wall = Arch.makeWall(length='+str(self.lengthValue)+',width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        if self.MultiMat:
            FreeCADGui.doCommand("wall.Material = FreeCAD.ActiveDocument."+self.MultiMat.Name)
        self.doc.commitTransaction()
        self.doc.recompute()
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.escape()


FreeCADGui.addCommand('Arch_Wall', Arch_Wall())
