# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""BIM wall command"""


import os
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
        self.Align = ["Center","Left","Right"][params.get_param_arch("WallAlignment")]
        self.MultiMat = None
        self.Length = None
        self.lengthValue = 0
        self.continueCmd = False
        self.Width = params.get_param_arch("WallWidth")
        self.Height = params.get_param_arch("WallHeight")
        self.JOIN_WALLS_SKETCHES = params.get_param_arch("joinWallSketches")
        self.AUTOJOIN = params.get_param_arch("autoJoinWalls")
        sel = FreeCADGui.Selection.getSelectionEx()
        done = False
        self.existing = []
        self.wp = None

        if sel:
            # automatic mode
            if Draft.getType(sel[0].Object) != "Wall":
                FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
                FreeCADGui.addModule("Arch")
                for selobj in sel:
                    if Draft.getType(selobj.Object) == "Space":
                        spacedone = False
                        if selobj.HasSubObjects:
                            if "Face" in selobj.SubElementNames[0]:
                                idx = int(selobj.SubElementNames[0][4:])
                                FreeCADGui.doCommand("obj = Arch.makeWall(FreeCAD.ActiveDocument."+selobj.Object.Name+",face="+str(idx)+")")
                                spacedone = True
                        if not spacedone:
                            FreeCADGui.doCommand('obj = Arch.makeWall(FreeCAD.ActiveDocument.'+selobj.Object.Name+')')
                    else:
                        FreeCADGui.doCommand('obj = Arch.makeWall(FreeCAD.ActiveDocument.'+selobj.Object.Name+')')
                FreeCADGui.addModule("Draft")
                FreeCADGui.doCommand("Draft.autogroup(obj)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                done = True

        if not done:
            # interactive mode

            self.points = []
            self.wp = WorkingPlane.get_working_plane()
            self.tracker = DraftTrackers.boxTracker()
            FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
            FreeCADGui.Snapper.getPoint(callback=self.getPoint,
                                        extradlg=self.taskbox(),
                                        title=translate("Arch","First point of wall")+":")

    def getPoint(self,point=None,obj=None):
        """Callback for clicks during interactive mode.

        When method _CommandWall.Activated() has entered the interactive mode,
        this callback runs when the user clicks.

        Parameters
        ----------
        point: <class 'Base.Vector'>
            The point the user has selected.
        obj: <Part::PartFeature>, optional
            The object the user's cursor snapped to, if any.
        """

        import Draft
        import Part
        import Arch
        import ArchWall
        from draftutils import gui_utils
        if obj:
            if Draft.getType(obj) == "Wall":
                if not obj in self.existing:
                    self.existing.append(obj)
        if point is None:
            self.tracker.finalize()
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
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
                                        title=translate("Arch","Next point")+":",mode="line")

        elif len(self.points) == 2:
            l = Part.LineSegment(self.wp.get_local_coords(self.points[0]),
                                 self.wp.get_local_coords(self.points[1]))
            self.tracker.off()
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand('import Part')
            FreeCADGui.doCommand('trace=Part.LineSegment(FreeCAD.'+str(l.StartPoint)+',FreeCAD.'+str(l.EndPoint)+')')
            if not self.existing:
                # no existing wall snapped, just add a default wall
                self.addDefault()
            else:
                if self.JOIN_WALLS_SKETCHES:
                    # join existing subwalls first if possible, then add the new one
                    w = Arch.joinWalls(self.existing)
                    if w:
                        if ArchWall.areSameWallTypes([w,self]):
                            FreeCADGui.doCommand('FreeCAD.ActiveDocument.'+w.Name+'.Base.addGeometry(trace)')
                        else:
                            # if not possible, add new wall as addition to the existing one
                            self.addDefault()
                            if self.AUTOJOIN:
                                FreeCADGui.doCommand('Arch.addComponents(FreeCAD.ActiveDocument.'+FreeCAD.ActiveDocument.Objects[-1].Name+',FreeCAD.ActiveDocument.'+w.Name+')')
                    else:
                        self.addDefault()
                else:
                    # add new wall as addition to the first existing one
                    self.addDefault()
                    if self.AUTOJOIN:
                        FreeCADGui.doCommand('Arch.addComponents(FreeCAD.ActiveDocument.'+FreeCAD.ActiveDocument.Objects[-1].Name+',FreeCAD.ActiveDocument.'+self.existing[0].Name+')')
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            # gui_utils.end_all_events()  # Causes a crash on Linux.
            self.tracker.finalize()
            if self.continueCmd:
                self.Activated()

    def addDefault(self):
        """Create a wall using a line segment, with all parameters as the default.

        Used solely by _CommandWall.getPoint() when the interactive mode has
        selected two points.

        Relies on the assumption that FreeCADGui.doCommand() has already
        created a Part.LineSegment assigned as the variable "trace"
        """

        from draftutils import params
        FreeCADGui.addModule("Draft")
        FreeCADGui.addModule("WorkingPlane")
        FreeCADGui.doCommand("wp = WorkingPlane.get_working_plane()")
        if params.get_param_arch("WallSketches"):
            # Use ArchSketch if SketchArch add-on is present
            try:
                import ArchSketchObject
                FreeCADGui.doCommand('import ArchSketchObject')
                FreeCADGui.doCommand('base=ArchSketchObject.makeArchSketch()')
            except:
                FreeCADGui.doCommand('base=FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","WallTrace")')
            FreeCADGui.doCommand('base.Placement = wp.get_placement()')
            FreeCADGui.doCommand('base.addGeometry(trace)')
        else:
            FreeCADGui.doCommand('base=Draft.make_line(trace)')
            # The created line should not stay selected as this causes an issue in continue mode.
            # Two walls would then be created based on the same line.
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.doCommand('base.Placement = wp.get_placement()')
            FreeCADGui.doCommand('FreeCAD.ActiveDocument.recompute()')
        FreeCADGui.doCommand('wall = Arch.makeWall(base,width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        FreeCADGui.doCommand('wall.Normal = wp.axis')
        if self.MultiMat:
            FreeCADGui.doCommand("wall.Material = FreeCAD.ActiveDocument."+self.MultiMat.Name)
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

        matCombo = QtGui.QComboBox()
        matCombo.addItem(translate("Arch","Wall Presets..."))
        matCombo.setToolTip(translate("Arch","This list shows all the MultiMaterials objects of this document. Create some to define wall types."))
        self.multimats = []
        self.MultiMat = None
        for o in FreeCAD.ActiveDocument.Objects:
            if Draft.getType(o) == "MultiMaterial":
                self.multimats.append(o)
                matCombo.addItem(o.Label)
        if hasattr(FreeCAD,"LastArchMultiMaterial"):
            for i,o in enumerate(self.multimats):
                if o.Name == FreeCAD.LastArchMultiMaterial:
                    matCombo.setCurrentIndex(i+1)
                    self.MultiMat = o
        grid.addWidget(matCombo,0,0,1,2)

        label5 = QtGui.QLabel(translate("Arch","Length"))
        self.Length = ui.createWidget("Gui::InputField")
        self.Length.setText("0.00 mm")
        grid.addWidget(label5,1,0,1,1)
        grid.addWidget(self.Length,1,1,1,1)

        label1 = QtGui.QLabel(translate("Arch","Width"))
        value1 = ui.createWidget("Gui::InputField")
        value1.setText(FreeCAD.Units.Quantity(self.Width,FreeCAD.Units.Length).UserString)
        grid.addWidget(label1,2,0,1,1)
        grid.addWidget(value1,2,1,1,1)

        label2 = QtGui.QLabel(translate("Arch","Height"))
        value2 = ui.createWidget("Gui::InputField")
        value2.setText(FreeCAD.Units.Quantity(self.Height,FreeCAD.Units.Length).UserString)
        grid.addWidget(label2,3,0,1,1)
        grid.addWidget(value2,3,1,1,1)

        label3 = QtGui.QLabel(translate("Arch","Alignment"))
        value3 = QtGui.QComboBox()
        items = [translate("Arch","Center"),translate("Arch","Left"),translate("Arch","Right")]
        value3.addItems(items)
        value3.setCurrentIndex(["Center","Left","Right"].index(self.Align))
        grid.addWidget(label3,4,0,1,1)
        grid.addWidget(value3,4,1,1,1)

        label4 = QtGui.QLabel(translate("Arch","Con&tinue"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        self.continueCmd = params.get_param("ContinueMode")
        value4.setChecked(self.continueCmd)
        grid.addWidget(label4,5,0,1,1)
        grid.addWidget(value4,5,1,1,1)

        label5 = QtGui.QLabel(translate("Arch","Use sketches"))
        value5 = QtGui.QCheckBox()
        value5.setObjectName("UseSketches")
        value5.setLayoutDirection(QtCore.Qt.RightToLeft)
        label5.setBuddy(value5)
        value5.setChecked(params.get_param_arch("WallSketches"))
        grid.addWidget(label5,6,0,1,1)
        grid.addWidget(value5,6,1,1,1)

        self.Length.valueChanged.connect(self.setLength)
        value1.valueChanged.connect(self.setWidth)
        value2.valueChanged.connect(self.setHeight)
        value3.currentIndexChanged.connect(self.setAlign)
        value4.stateChanged.connect(self.setContinue)
        value5.stateChanged.connect(self.setUseSketch)
        self.Length.returnPressed.connect(value1.setFocus)
        self.Length.returnPressed.connect(value1.selectAll)
        value1.returnPressed.connect(value2.setFocus)
        value1.returnPressed.connect(value2.selectAll)
        value2.returnPressed.connect(self.createFromGUI)
        matCombo.currentIndexChanged.connect(self.setMat)
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

    def setContinue(self,i):
        """Simple callback to set if the interactive mode will restart when finished.

        This allows for several walls to be placed one after another.
        """

        from draftutils import params
        self.continueCmd = bool(i)
        params.set_param("ContinueMode", bool(i))

    def setUseSketch(self,i):
        """Simple callback to set if walls should based on sketches."""

        from draftutils import params
        params.set_param_arch("WallSketches",bool(i))

    def createFromGUI(self):
        """Callback to create wall by using the _CommandWall.taskbox()"""

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Wall"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand('wall = Arch.makeWall(length='+str(self.lengthValue)+',width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        if self.MultiMat:
            FreeCADGui.doCommand("wall.Material = FreeCAD.ActiveDocument."+self.MultiMat.Name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.escape()


FreeCADGui.addCommand('Arch_Wall', Arch_Wall())
