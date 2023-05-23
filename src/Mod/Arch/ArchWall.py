#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

"""This module provides tools to build Wall objects.  Walls are simple
objects, usually vertical, typically obtained by giving a thickness to a base
line, then extruding it vertically.

Examples
--------
TODO put examples here.

"""

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import draftguitools.gui_trackers as DraftTrackers
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.  Walls are simple objects,
#  usually vertical, typically obtained by giving a thickness to a base line,
#  then extruding it vertically.

__title__  = "FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

def makeWall(baseobj=None,height=None,length=None,width=None,align=None,face=None,name=None):
    """Create a wall based on a given object, and returns the generated wall.

    TODO: It is unclear what defines which units this function uses.

    Parameters
    ----------
    baseobj: <Part::PartFeature>, optional
        The base object with which to build the wall. This can be a sketch, a
        draft object, a face, or a solid. It can also be left as None.
    height: float, optional
        The height of the wall.
    length: float, optional
        The length of the wall. Not used if the wall is based off an object.
        Will use Arch default if left empty.
    width: float, optional
        The width of the wall. Not used if the base object is a face.  Will use
        Arch default if left empty.
    align: str, optional
        Either "Center", "Left", or "Right". Effects the alignment of the wall
        on its baseline.
    face: int, optional
        The index number of a face on the given baseobj, to base the wall on.
    name: str, optional
        The name to give to the created wall.

    Returns
    -------
    <Part::FeaturePython>
        Returns the generated wall.

    Notes
    -----
    Creates a new <Part::FeaturePython> object, and turns it into a parametric wall
    object. This <Part::FeaturePython> object does not yet have any shape.

    The wall then uses the baseobj.Shape as the basis to extrude out a wall shape,
    giving the new <Part::FeaturePython> object a shape.

    It then hides the original baseobj.
    """

    if not FreeCAD.ActiveDocument:
        FreeCAD.Console.PrintError("No active document. Aborting\n")
        return
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Wall")
    if name:
        obj.Label = name
    else:
        obj.Label = translate("Arch","Wall")
    _Wall(obj)
    if FreeCAD.GuiUp:
        _ViewProviderWall(obj.ViewObject)
    if baseobj:
        if hasattr(baseobj,'Shape') or baseobj.isDerivedFrom("Mesh::Feature"):
            obj.Base = baseobj
        else:
            FreeCAD.Console.PrintWarning(str(translate("Arch","Walls can only be based on Part or Mesh objects")))
    if face:
        obj.Face = face
    if length:
        obj.Length = length
    if width:
        obj.Width = width
    else:
        obj.Width = p.GetFloat("WallWidth",200)
    if height:
        obj.Height = height
    else:
        obj.Height = p.GetFloat("WallHeight",3000)
    if align:
        obj.Align = align
    else:
        obj.Align = ["Center","Left","Right"][p.GetInt("WallAlignment",0)]
    if obj.Base and FreeCAD.GuiUp:
        if Draft.getType(obj.Base) != "Space":
            obj.Base.ViewObject.hide()
    return obj

def joinWalls(walls,delete=False):
    """Join the given list of walls into one sketch-based wall.

    Take the first wall in the list, and adds on the other walls in the list.
    Return the modified first wall.

    Setting delete to True, will delete the other walls. Only join walls
    if the walls have the same width, height and alignment.

    Parameters
    ----------
    walls: list of <Part::FeaturePython>
        List containing the walls to add to the first wall in the list. Walls must
        be based off a base object.
    delete: bool, optional
        If True, deletes the other walls in the list.

    Returns
    -------
    <Part::FeaturePython>
    """

    import Part
    if not walls:
        return None
    if not isinstance(walls,list):
        walls = [walls]
    if not areSameWallTypes(walls):
        return None
    deleteList = []
    base = walls.pop()
    if base.Base:
        if base.Base.Shape.Faces:
            return None
        # Use ArchSketch if SketchArch add-on is present
        if Draft.getType(base.Base) == "ArchSketch":
            sk = base.Base
        else:
            try:
                import ArchSketchObject
                newSk=ArchSketchObject.makeArchSketch()
            except:
                if Draft.getType(base.Base) != "Sketcher::SketchObject":
                    newSk=FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","WallTrace")
                else:
                    newSk=None
            if newSk:
                sk = Draft.makeSketch(base.Base,autoconstraints=True, addTo=newSk)
                base.Base = sk
            else:
                sk = base.Base
    for w in walls:
        if w.Base:
            if not w.Base.Shape.Faces:
                for e in w.Base.Shape.Edges:
                    l = e.Curve
                    if isinstance(l,Part.Line):
                        l = Part.LineSegment(e.Vertexes[0].Point,e.Vertexes[-1].Point)
                    sk.addGeometry(l)
                    deleteList.append(w.Name)
    if delete:
        for n in deleteList:
            FreeCAD.ActiveDocument.removeObject(n)
    FreeCAD.ActiveDocument.recompute()
    base.ViewObject.show()
    return base

def mergeShapes(w1,w2):
    """Not currently implemented.

    Return a Shape built on two walls that share same properties and have a
    coincident endpoint.
    """

    if not areSameWallTypes([w1,w2]):
        return None
    if (not hasattr(w1.Base,"Shape")) or (not hasattr(w2.Base,"Shape")):
        return None
    if w1.Base.Shape.Faces or w2.Base.Shape.Faces:
        return None

    # TODO fix this
    return None

    eds = w1.Base.Shape.Edges + w2.Base.Shape.Edges
    import DraftGeomUtils
    w = DraftGeomUtils.findWires(eds)
    if len(w) == 1:
        #print("found common wire")
        normal,length,width,height = w1.Proxy.getDefaultValues(w1)
        print(w[0].Edges)
        sh = w1.Proxy.getBase(w1,w[0],normal,width,height)
        print(sh)
        return sh
    return None

def areSameWallTypes(walls):
    """Check if a list of walls have the same height, width and alignment.

    Parameters
    ----------
    walls: list of <ArchComponent.Component>

    Returns
    -------
    bool
        True if the walls have the same height, width and alignment, False if
        otherwise.
    """

    for att in ["Width","Height","Align"]:
        value = None
        for w in walls:
            if not hasattr(w,att):
                return False
            if not value:
                value = getattr(w,att)
            else:
                if type(value) == float:
                    if round(value,Draft.precision()) != round(getattr(w,att),Draft.precision()):
                        return False
                else:
                    if value != getattr(w,att):
                        return False
    return True



class _CommandWall:
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
        """Determines whether or not the Arch Wall tool is active.

        Inactive commands are indicated by a greyed-out icon in the menus and
        toolbars.
        """

        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        """Executed when Arch Wall is called.

        Creates a wall from the object selected by the user. If no objects are
        selected, enters an interactive mode to create a wall using selected
        points to create a base.
        """

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Align = ["Center","Left","Right"][p.GetInt("WallAlignment",0)]
        self.MultiMat = None
        self.Length = None
        self.lengthValue = 0
        self.continueCmd = False
        self.Width = p.GetFloat("WallWidth",200)
        self.Height = p.GetFloat("WallHeight",3000)
        self.JOIN_WALLS_SKETCHES = p.GetBool("joinWallSketches",False)
        self.AUTOJOIN = p.GetBool("autoJoinWalls",True)
        sel = FreeCADGui.Selection.getSelectionEx()
        done = False
        self.existing = []

        if sel:
            # automatic mode
            import Draft
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
            self.tracker = DraftTrackers.boxTracker()
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                FreeCAD.DraftWorkingPlane.setup()
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

        if obj:
            if Draft.getType(obj) == "Wall":
                if not obj in self.existing:
                    self.existing.append(obj)
        if point is None:
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
                                        title=translate("Arch","Next point")+":",mode="line")

        elif len(self.points) == 2:
            import Part
            l = Part.LineSegment(FreeCAD.DraftWorkingPlane.getLocalCoords(self.points[0]),
                                 FreeCAD.DraftWorkingPlane.getLocalCoords(self.points[1]))
            self.tracker.finalize()
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
                    w = joinWalls(self.existing)
                    if w:
                        if areSameWallTypes([w,self]):
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
            if self.continueCmd:
                self.Activated()

    def addDefault(self):
        """Create a wall using a line segment, with all parameters as the default.

        Used solely by _CommandWall.getPoint() when the interactive mode has
        selected two points.

        Relies on the assumption that FreeCADGui.doCommand() has already
        created a Part.LineSegment assigned as the variable "trace"
        """

        FreeCADGui.addModule("Draft")
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("WallSketches",True):
            # Use ArchSketch if SketchArch add-on is present
            try:
                import ArchSketchObject
                FreeCADGui.doCommand('import ArchSketchObject')
                FreeCADGui.doCommand('base=ArchSketchObject.makeArchSketch()')
            except:
                FreeCADGui.doCommand('base=FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject","WallTrace")')
            FreeCADGui.doCommand('base.Placement = FreeCAD.DraftWorkingPlane.getPlacement()')
            FreeCADGui.doCommand('base.addGeometry(trace)')
        else:
            FreeCADGui.doCommand('base=Draft.makeLine(trace)')
            FreeCADGui.doCommand('FreeCAD.ActiveDocument.recompute()')
        FreeCADGui.doCommand('wall = Arch.makeWall(base,width='+str(self.Width)+',height='+str(self.Height)+',align="'+str(self.Align)+'")')
        FreeCADGui.doCommand('wall.Normal = FreeCAD.DraftWorkingPlane.getNormal()')
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

        if FreeCADGui.Control.activeDialog():
            b = self.points[0]
            n = FreeCAD.DraftWorkingPlane.axis
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
        if hasattr(FreeCADGui,"draftToolBar"):
            value4.setChecked(FreeCADGui.draftToolBar.continueMode)
            self.continueCmd = FreeCADGui.draftToolBar.continueMode
        grid.addWidget(label4,5,0,1,1)
        grid.addWidget(value4,5,1,1,1)

        label5 = QtGui.QLabel(translate("Arch","Use sketches"))
        value5 = QtGui.QCheckBox()
        value5.setObjectName("UseSketches")
        value5.setLayoutDirection(QtCore.Qt.RightToLeft)
        label5.setBuddy(value5)
        value5.setChecked(FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetBool("WallSketches",True))
        grid.addWidget(label5,6,0,1,1)
        grid.addWidget(value5,6,1,1,1)

        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(value2,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value3,QtCore.SIGNAL("currentIndexChanged(int)"),self.setAlign)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("stateChanged(int)"),self.setUseSketch)
        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("returnPressed()"),value1.setFocus)
        QtCore.QObject.connect(self.Length,QtCore.SIGNAL("returnPressed()"),value1.selectAll)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("returnPressed()"),value2.setFocus)
        QtCore.QObject.connect(value1,QtCore.SIGNAL("returnPressed()"),value2.selectAll)
        QtCore.QObject.connect(value2,QtCore.SIGNAL("returnPressed()"),self.createFromGUI)
        QtCore.QObject.connect(matCombo,QtCore.SIGNAL("currentIndexChanged(int)"),self.setMat)
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

        self.lengthValue = d

    def setWidth(self,d):
        """Simple callback for the interactive mode gui widget to set width."""

        self.Width = d
        self.tracker.width(d)
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetFloat("WallWidth",d)


    def setHeight(self,d):
        """Simple callback for the interactive mode gui widget to set height."""

        self.Height = d
        self.tracker.height(d)
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetFloat("WallHeight",d)

    def setAlign(self,i):
        """Simple callback for the interactive mode gui widget to set alignment."""

        self.Align = ["Center","Left","Right"][i]
        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetInt("WallAlignment",i)

    def setContinue(self,i):
        """Simple callback to set if the interactive mode will restart when finished.

        This allows for several walls to be placed one after another.
        """

        self.continueCmd = bool(i)
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)

    def setUseSketch(self,i):
        """Simple callback to set if walls should join their base sketches when possible."""

        FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").SetBool("joinWallSketches",bool(i))

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


class _CommandMergeWalls:
    """The command definition for the Arch workbench's gui tool, Arch MergeWalls.

    A tool for merging walls.

    Join two or more walls by using the ArchWall.joinWalls() function.

    Find documentation on the end user usage of Arch Wall here:
    https://wiki.freecad.org/Arch_MergeWalls
    """

    def GetResources(self):
        """Returns a dictionary with the visual aspects of the Arch MergeWalls tool."""

        return {'Pixmap'  : 'Arch_MergeWalls',
                'MenuText': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merge Walls"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_MergeWalls","Merges the selected walls, if possible")}

    def IsActive(self):
        """Determines whether or not the Arch MergeWalls tool is active.

        Inactive commands are indicated by a greyed-out icon in the menus and
        toolbars.
        """

        return bool(FreeCADGui.Selection.getSelection())

    def Activated(self):
        """Executed when Arch MergeWalls is called.

        Call ArchWall.joinWalls() on walls selected by the user, with the
        delete option enabled. If the user has selected a single wall, check to
        see if the wall has any Additions that are walls. If so, merges these
        additions to the wall, deleting the additions.
        """

        walls = FreeCADGui.Selection.getSelection()
        if len(walls) == 1:
            if Draft.getType(walls[0]) == "Wall":
                ostr = "FreeCAD.ActiveDocument."+ walls[0].Name
                ok = False
                for o in walls[0].Additions:
                    if Draft.getType(o) == "Wall":
                        ostr += ",FreeCAD.ActiveDocument." + o.Name
                        ok = True
                if ok:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Merge Wall"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.joinWalls(["+ostr+"],delete=True)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    return
                else:
                    FreeCAD.Console.PrintWarning(translate("Arch","The selected wall contains no subwall to merge"))
                    return
            else:
                FreeCAD.Console.PrintWarning(translate("Arch","Please select only wall objects"))
                return
        for w in walls:
            if Draft.getType(w) != "Wall":
                FreeCAD.Console.PrintMessage(translate("Arch","Please select only wall objects"))
                return
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Merge Walls"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("Arch.joinWalls(FreeCADGui.Selection.getSelection(),delete=True)")
        FreeCAD.ActiveDocument.commitTransaction()

class _Wall(ArchComponent.Component):
    """The Wall object.

    Turns a <App::FeaturePython> into a wall object, then uses a
    <Part::Feature> to create the wall's shape.

    Walls are simple objects, usually vertical, typically obtained by giving a
    thickness to a base line, then extruding it vertically.

    Parameters
    ----------
    obj: <App::FeaturePython>
        The object to turn into a wall. Note that this is not the object that
        forms the basis for the new wall's shape. That is given later.
    """

    def __init__(self, obj):
        ArchComponent.Component.__init__(self, obj)
        self.setProperties(obj)
        obj.IfcType = "Wall"

    def setProperties(self, obj):
        """Give the wall its wall specific properties, such as its alignment.

        You can learn more about properties here:
        https://wiki.freecad.org/property

        parameters
        ----------
        obj: <part::featurepython>
            The object to turn into a wall.
        """

        lp = obj.PropertiesList
        if not "Length" in lp:
            obj.addProperty("App::PropertyLength","Length","Wall",QT_TRANSLATE_NOOP("App::Property","The length of this wall. Not used if this wall is based on an underlying object"))
        if not "Width" in lp:
            obj.addProperty("App::PropertyLength","Width","Wall",QT_TRANSLATE_NOOP("App::Property","The width of this wall. Not used if this wall is based on a face"))

        # To be combined into Width when PropertyLengthList is available
        if not "OverrideWidth" in lp:
            obj.addProperty("App::PropertyFloatList","OverrideWidth","Wall",QT_TRANSLATE_NOOP("App::Property","This overrides Width attribute to set width of each segment of wall.  Ignored if Base object provides Widths information, with getWidths() method.  (The 1st value override 'Width' attribute for 1st segment of wall; if a value is zero, 1st value of 'OverrideWidth' will be followed)"))			# see DraftGeomUtils.offsetwire()

        if not "OverrideAlign" in lp:
            obj.addProperty("App::PropertyStringList","OverrideAlign","Wall",QT_TRANSLATE_NOOP("App::Property","This overrides Align attribute to set Align of each segment of wall.  Ignored if Base object provides Aligns information, with getAligns() method.  (The 1st value override 'Align' attribute for 1st segment of wall; if a value is not 'Left, Right, Center', 1st value of 'OverrideAlign' will be followed)"))			# see DraftGeomUtils.offsetwire()

        if not "Height" in lp:
            obj.addProperty("App::PropertyLength","Height","Wall",QT_TRANSLATE_NOOP("App::Property","The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid"))
        if not "Area" in lp:
            obj.addProperty("App::PropertyArea","Area","Wall",QT_TRANSLATE_NOOP("App::Property","The area of this wall as a simple Height * Length calculation"))
            obj.setEditorMode("Area",1)
        if not "Align" in lp:
            obj.addProperty("App::PropertyEnumeration","Align","Wall",QT_TRANSLATE_NOOP("App::Property","The alignment of this wall on its base object, if applicable"))
            obj.Align = ['Left','Right','Center']
        if not "Normal" in lp:
            obj.addProperty("App::PropertyVector","Normal","Wall",QT_TRANSLATE_NOOP("App::Property","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        if not "Face" in lp:
            obj.addProperty("App::PropertyInteger","Face","Wall",QT_TRANSLATE_NOOP("App::Property","The face number of the base object used to build this wall"))
        if not "Offset" in lp:
            obj.addProperty("App::PropertyDistance","Offset","Wall",QT_TRANSLATE_NOOP("App::Property","The offset between this wall and its baseline (only for left and right alignments)"))

        # See getExtrusionData(), removeSplitters are no longer used
        #if not "Refine" in lp:
        #    obj.addProperty("App::PropertyEnumeration","Refine","Wall",QT_TRANSLATE_NOOP("App::Property","Select whether or not and the method to remove splitter of the Wall. Currently Draft removeSplitter and Part removeSplitter available but may not work on complex sketch."))
        #    obj.Refine = ['No','DraftRemoveSplitter','PartRemoveSplitter']
        # TODO - To implement in Arch Component ?

        if not "MakeBlocks" in lp:
            obj.addProperty("App::PropertyBool","MakeBlocks","Blocks",QT_TRANSLATE_NOOP("App::Property","Enable this to make the wall generate blocks"))
        if not "BlockLength" in lp:
            obj.addProperty("App::PropertyLength","BlockLength","Blocks",QT_TRANSLATE_NOOP("App::Property","The length of each block"))
        if not "BlockHeight" in lp:
            obj.addProperty("App::PropertyLength","BlockHeight","Blocks",QT_TRANSLATE_NOOP("App::Property","The height of each block"))
        if not "OffsetFirst" in lp:
            obj.addProperty("App::PropertyLength","OffsetFirst","Blocks",QT_TRANSLATE_NOOP("App::Property","The horizontal offset of the first line of blocks"))
        if not "OffsetSecond" in lp:
            obj.addProperty("App::PropertyLength","OffsetSecond","Blocks",QT_TRANSLATE_NOOP("App::Property","The horizontal offset of the second line of blocks"))
        if not "Joint" in lp:
            obj.addProperty("App::PropertyLength","Joint","Blocks",QT_TRANSLATE_NOOP("App::Property","The size of the joints between each block"))
        if not "CountEntire" in lp:
            obj.addProperty("App::PropertyInteger","CountEntire","Blocks",QT_TRANSLATE_NOOP("App::Property","The number of entire blocks"))
            obj.setEditorMode("CountEntire",1)
        if not "CountBroken" in lp:
            obj.addProperty("App::PropertyInteger","CountBroken","Blocks",QT_TRANSLATE_NOOP("App::Property","The number of broken blocks"))
            obj.setEditorMode("CountBroken",1)
        self.Type = "Wall"

    def onDocumentRestored(self,obj):
        """Method run when the document is restored. Re-adds the Arch component, and Arch wall properties."""

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):
        """Method run when the object is recomputed.

        Extrude the wall from the Base shape if possible. Processe additions
        and subtractions. Assign the resulting shape as the shape of the wall.

        Add blocks if the MakeBlocks property is assigned. If the Base shape is
        a mesh, just copy the mesh.
        """

        if self.clone(obj):
            return

        import Part
        import DraftGeomUtils
        base = None
        pl = obj.Placement
        extdata = self.getExtrusionData(obj)
        if extdata:
            bplates = extdata[0]
            extv = extdata[2].Rotation.multVec(extdata[1])
            if isinstance(bplates,list):
                shps = []
                # Test : if base is Sketch, then fuse all solid; otherwise, makeCompound
                sketchBaseToFuse = obj.Base.getLinkedObject().isDerivedFrom("Sketcher::SketchObject")
                # but turn this off if we have layers, otherwise layers get merged
                if hasattr(obj,"Material") and obj.Material \
                and hasattr(obj.Material,"Materials") and obj.Material.Materials:
                    sketchBaseToFuse = False
                for b in bplates:
                    b.Placement = extdata[2].multiply(b.Placement)
                    b = b.extrude(extv)

                    # See getExtrusionData() - not fusing baseplates there - fuse solids here
                    # Remarks - If solids are fused, but exportIFC.py use underlying baseplates w/o fuse, the result in ifc look slightly different from in FC.

                    if sketchBaseToFuse:
                        if shps:
                            shps = shps.fuse(b) #shps.fuse(b)
                        else:
                            shps=b
                    else:
                        shps.append(b)
                    # TODO - To let user to select whether to fuse (slower) or to do a compound (faster) only ?

                if sketchBaseToFuse:
                    base = shps
                else:
                    base = Part.makeCompound(shps)
            else:
                bplates.Placement = extdata[2].multiply(bplates.Placement)
                base = bplates.extrude(extv)
        if obj.Base:
            if hasattr(obj.Base,'Shape'):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                elif obj.Base.Shape.Solids:
                    base = Part.Shape(obj.Base.Shape)
                # blocks calculation
                elif hasattr(obj,"MakeBlocks") and hasattr(self,"basewires"):
                    if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                        #print "calculating blocks"
                        if len(self.basewires) == 1:
                            blocks = []
                            n = FreeCAD.Vector(extv)
                            n.normalize()
                            cuts1 = []
                            cuts2 = []
                            if obj.BlockLength.Value:
                                for i in range(2):
                                    if i == 0:
                                        offset = obj.OffsetFirst.Value
                                    else:
                                        offset = obj.OffsetSecond.Value
                                    # only 1 wire (first) is supported
                                    if len(obj.Base.Shape.Edges) == 1:
                                        # If there is a single edge, the wire was used
                                        baseEdges = self.basewires[0].Edges
                                    elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                                        # if obj.Base is Sketch, self.baseWires[0] returned is already a list of edge
                                        baseEdges = self.basewires[0]
                                    else:
                                        # otherwise, it is wire
                                        baseEdges = self.basewires[0].Edges
                                    for edge in baseEdges:
                                        while offset < (edge.Length-obj.Joint.Value):
                                            #print i," Edge ",edge," : ",edge.Length," - ",offset
                                            if offset:
                                                t = edge.tangentAt(offset)
                                                p = t.cross(n)
                                                p.multiply(1.1*obj.Width.Value+obj.Offset.Value)
                                                p1 = edge.valueAt(offset).add(p)
                                                p2 = edge.valueAt(offset).add(p.negative())
                                                sh = Part.LineSegment(p1,p2).toShape()
                                                if obj.Joint.Value:
                                                    sh = sh.extrude(-t.multiply(obj.Joint.Value))
                                                sh = sh.extrude(n)
                                                if i == 0:
                                                    cuts1.append(sh)
                                                else:
                                                    cuts2.append(sh)
                                            offset += (obj.BlockLength.Value + obj.Joint.Value)
                                        offset -= (edge.Length - obj.Joint.Value)

                            if isinstance(bplates,list):
                                bplates = bplates[0]
                            if obj.BlockHeight.Value:
                                fsize = obj.BlockHeight.Value + obj.Joint.Value
                                bh = obj.BlockHeight.Value
                            else:
                                fsize = obj.Height.Value
                                bh = obj.Height.Value
                            bvec = FreeCAD.Vector(n)
                            bvec.multiply(bh)
                            svec = FreeCAD.Vector(n)
                            svec.multiply(fsize)
                            if cuts1:
                                plate1 = bplates.cut(cuts1).Faces
                            else:
                                plate1 = bplates.Faces
                            blocks1 = Part.makeCompound([f.extrude(bvec) for f in plate1])
                            if cuts2:
                                plate2 = bplates.cut(cuts2).Faces
                            else:
                                plate2 = bplates.Faces
                            blocks2 = Part.makeCompound([f.extrude(bvec) for f in plate2])
                            interval = extv.Length/(fsize)
                            entire = int(interval)
                            rest = (interval - entire)
                            for i in range(entire):
                                if i % 2: # odd
                                    b = Part.Shape(blocks2)
                                else:
                                    b = Part.Shape(blocks1)
                                if i:
                                    t = FreeCAD.Vector(svec)
                                    t.multiply(i)
                                    b.translate(t)
                                blocks.append(b)
                            if rest:
                                rest = extv.Length - (entire * fsize)
                                rvec = FreeCAD.Vector(n)
                                rvec.multiply(rest)
                                if entire % 2:
                                    b = Part.makeCompound([f.extrude(rvec) for f in plate2])
                                else:
                                    b = Part.makeCompound([f.extrude(rvec) for f in plate1])
                                t = FreeCAD.Vector(svec)
                                t.multiply(entire)
                                b.translate(t)
                                blocks.append(b)
                            if blocks:
                                base = Part.makeCompound(blocks)

                        else:
                            FreeCAD.Console.PrintWarning(translate("Arch","Cannot compute blocks for wall")+obj.Label+"\n")

            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(translate("Arch","This mesh is an invalid solid")+"\n")
                            obj.Base.ViewObject.show()
        if not base:
            #FreeCAD.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            #return
            # walls can be made of only a series of additions and have no base shape
            base = Part.Shape()

        base = self.processSubShapes(obj,base,pl)

        self.applyShape(obj,base,pl)

        # count blocks
        if hasattr(obj,"MakeBlocks"):
            if obj.MakeBlocks:
                fvol = obj.BlockLength.Value * obj.BlockHeight.Value * obj.Width.Value
                if fvol:
                    #print("base volume:",fvol)
                    #for s in base.Solids:
                        #print(abs(s.Volume - fvol))
                    ents = [s for s in base.Solids if abs(s.Volume - fvol) < 1]
                    obj.CountEntire = len(ents)
                    obj.CountBroken = len(base.Solids) - len(ents)
                else:
                    obj.CountEntire = 0
                    obj.CountBroken = 0

        # set the length property
        if obj.Base:
            if hasattr(obj.Base,'Shape'):
                if obj.Base.Shape.Edges:
                    if not obj.Base.Shape.Faces:
                        if hasattr(obj.Base.Shape,"Length"):
                            l = obj.Base.Shape.Length
                            if obj.Length.Value != l:
                                obj.Length = l
                                self.oldLength = None # delete the stored value to prevent triggering base change below

        # set the Area property
        obj.Area = obj.Length.Value * obj.Height.Value

    def onBeforeChange(self,obj,prop):
        """Method called before the object has a property changed.

        Specifically, this method is called before the value changes.

        If "Length" has changed, record the old length so that .onChanged() can
        be sure that the base needs to be changed.

        Also call ArchComponent.Component.onBeforeChange().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "Length":
            self.oldLength = obj.Length.Value
        ArchComponent.Component.onBeforeChange(self,obj,prop)

    def onChanged(self, obj, prop):
        """Method called when the object has a property changed.

        If length has changed, extend the length of the Base object, if the
        Base object only has a single edge to extend.

        Also hide subobjects.

        Also call ArchComponent.Component.onChanged().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "Length":
            if (obj.Base and obj.Length.Value
                    and hasattr(self,"oldLength") and (self.oldLength is not None)
                    and (self.oldLength != obj.Length.Value)):

                if hasattr(obj.Base,'Shape'):
                    if len(obj.Base.Shape.Edges) == 1:
                        import DraftGeomUtils
                        e = obj.Base.Shape.Edges[0]
                        if DraftGeomUtils.geomType(e) == "Line":
                            if e.Length != obj.Length.Value:
                                v = e.Vertexes[-1].Point.sub(e.Vertexes[0].Point)
                                v.normalize()
                                v.multiply(obj.Length.Value)
                                p2 = e.Vertexes[0].Point.add(v)
                                if Draft.getType(obj.Base) == "Wire":
                                    #print "modifying p2"
                                    obj.Base.End = p2
                                elif Draft.getType(obj.Base) == "Sketcher::SketchObject":
                                    try:
                                        obj.Base.recompute() # Fix for the 'GeoId index out range' error.
                                        obj.Base.movePoint(0, 2, obj.Base.Placement.inverse().multVec(p2))
                                    except Exception: # This 'GeoId index out range' error should no longer occur.
                                        print("Debug: The base sketch of this wall could not be changed, because the sketch has not been edited yet in this session (this is a bug in FreeCAD). Try entering and exiting edit mode in this sketch first, and then changing the wall length should work.")
                                else:
                                    FreeCAD.Console.PrintError(translate("Arch","Error: Unable to modify the base object of this wall")+"\n")

        self.hideSubobjects(obj,prop)
        ArchComponent.Component.onChanged(self,obj,prop)

    def getFootprint(self,obj):
        """Get the faces that make up the base/foot of the wall.

        Returns
        -------
        list of <Part.Face>
            The faces that make up the foot of the wall.
        """

        faces = []
        if obj.Shape:
            for f in obj.Shape.Faces:
                if f.normalAt(0,0).getAngle(FreeCAD.Vector(0,0,-1)) < 0.01:
                    if abs(abs(f.CenterOfMass.z) - abs(obj.Shape.BoundBox.ZMin)) < 0.001:
                        faces.append(f)
        return faces

    def getExtrusionData(self,obj):
        """Get data needed to extrude the wall from a base object.

        take the Base object, and find a base face to extrude
        out, a vector to define the extrusion direction and distance.

        Rebase the base face to the (0,0,0) origin.

        Return the base face, rebased, with the extrusion vector, and the
        <Base.Placement> needed to return the face back to its original
        position.

        Returns
        -------
        tuple of (<Part.Face>, <Base.Vector>, <Base.Placement>)
            Tuple containing the base face, the vector for extrusion, and the
            placement needed to move the face back from the (0,0,0) origin.
        """

        import Part
        import DraftGeomUtils

        # If ArchComponent.Component.getExtrusionData() can successfully get
        # extrusion data, just use that.
        data = ArchComponent.Component.getExtrusionData(self,obj)
        if data:
            if not isinstance(data[0],list):
                # multifuses not considered here
                return data
        length  = obj.Length.Value

        # TODO currently layers were not supported when len(basewires) > 0	##( or 1 ? )
        width = 0

        # Get width of each edge segment from Base Objects if they store it
        # (Adding support in SketchFeaturePython, DWire...)
        widths = []  # [] or None are both False
        if obj.Base:
            if hasattr(obj.Base, 'Proxy'):
                if hasattr(obj.Base.Proxy, 'getWidths'):
                    # Return a list of Width corresponding to indexes of sorted
                    # edges of Sketch.
                    widths = obj.Base.Proxy.getWidths(obj.Base)

        # Get width of each edge/wall segment from ArchWall.OverrideWidth if
        # Base Object does not provide it
        if not widths:
            if obj.OverrideWidth:
                if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when ArchSketch.getWidth() not implemented yet):-
                    # sort the width list in OverrrideWidth to correspond to indexes of sorted edges of Sketch
                    try:
                        import ArchSketchObject
                    except Exception:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        widths = ArchSketchObject.sortSketchWidth(obj.Base, obj.OverrideWidth)
                    except Exception:
                        widths = obj.OverrideWidth
                else:
                    # If Base Object is not Sketch, but e.g. DWire, the width
                    # list in OverrrideWidth just correspond to sequential
                    # order of edges
                    widths = obj.OverrideWidth
            elif obj.Width:
                widths = [obj.Width.Value]
            else:
                # having no width is valid for walls so the user doesn't need to be warned
                # it just disables extrusions and return none
                #print ("Width & OverrideWidth & base.getWidths() should not be all 0 or None or [] empty list ")
                return None

        # Set 'default' width - for filling in any item in the list == 0 or None
        if obj.Width.Value:
            width = obj.Width.Value
        else:
            width = 200  # 'Default' width value

        # Get align of each edge segment from Base Objects if they store it.
        # (Adding support in SketchFeaturePython, DWire...)
        aligns = []
        if obj.Base:
            if hasattr(obj.Base, 'Proxy'):
                if hasattr(obj.Base.Proxy, 'getAligns'):
                    # Return a list of Align corresponds to indexes of sorted
                    # edges of Sketch.
                    aligns = obj.Base.Proxy.getAligns(obj.Base)
        # Get align of each edge/wall segment from ArchWall.OverrideAlign if
        # Base Object does not provide it
        if not aligns:
            if obj.OverrideAlign:
                if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when
                    # ArchSketch.getAligns() not implemented yet):- sort the
                    # align list in OverrideAlign to correspond to indexes of
                    # sorted edges of Sketch
                    try:
                        import ArchSketchObject
                    except Exception:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        aligns = ArchSketchObject.sortSketchAlign(obj.Base, obj.OverrideAlign)
                    except Exception:
                        aligns = obj.OverrideAlign
                else:
                    # If Base Object is not Sketch, but e.g. DWire, the align
                    # list in OverrideAlign just correspond to sequential order
                    # of edges
                    aligns = obj.OverrideAlign
            else:
                aligns = [obj.Align]

        # set 'default' align - for filling in any item in the list == 0 or None
        align = obj.Align  # or aligns[0]

        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        if not height:
            return None
        if obj.Normal == Vector(0,0,0):
            normal = Vector(0,0,1)
        else:
            normal = Vector(obj.Normal)
        base = None
        placement = None
        self.basewires = None

        # build wall layers
        layers = []
        if hasattr(obj,"Material"):
            if obj.Material:
                if hasattr(obj.Material,"Materials"):
                    thicknesses = [abs(t) for t in obj.Material.Thicknesses]
                    # multimaterials
                    varwidth = 0
                    restwidth = width - sum(thicknesses)
                    if restwidth > 0:
                        varwidth = [t for t in thicknesses if t == 0]
                        if varwidth:
                            varwidth = restwidth/len(varwidth)
                    for t in obj.Material.Thicknesses:
                        if t:
                            layers.append(t)
                        elif varwidth:
                            layers.append(varwidth)

        if obj.Base:
            if hasattr(obj.Base,'Shape'):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        return None

                    # If the user has defined a specific face of the Base
                    # object to build the wall from, extrude from that face,
                    # and return the extrusion moved to (0,0,0), normal of the
                    # face, and placement to move the extrusion back to its
                    # original position.
                    elif obj.Face > 0:
                        if len(obj.Base.Shape.Faces) >= obj.Face:
                            face = obj.Base.Shape.Faces[obj.Face-1]
                            if obj.Normal != Vector(0,0,0):
                                normal = face.normalAt(0,0)
                            if normal.getAngle(Vector(0,0,1)) > math.pi/4:
                                normal.multiply(width)
                                base = face.extrude(normal)
                                if obj.Align == "Center":
                                    base.translate(normal.negative().multiply(0.5))
                                elif obj.Align == "Right":
                                    base.translate(normal.negative())
                            else:
                                normal.multiply(height)
                                base = face.extrude(normal)
                            base, placement = self.rebase(base)
                            return (base,normal,placement)

                    # If the Base has faces, but no specific one has been
                    # selected, rebase the faces and continue.
                    elif obj.Base.Shape.Faces:
                        if not DraftGeomUtils.isCoplanar(obj.Base.Shape.Faces):
                            return None
                        else:
                            base,placement = self.rebase(obj.Base.Shape)

                    # If the object is a single edge, use that as the
                    # basewires.
                    elif len(obj.Base.Shape.Edges) == 1:
                        self.basewires = [Part.Wire(obj.Base.Shape.Edges)]

                    # Sort Sketch edges consistently with below procedures
                    # without using Sketch.Shape.Edges - found the latter order
                    # in some corner case != getSortedClusters()
                    elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                        self.basewires = []
                        skGeom = obj.Base.GeometryFacadeList
                        skGeomEdges = []
                        skPlacement = obj.Base.Placement  # Get Sketch's placement to restore later
                        for i in skGeom:
                            if not i.Construction:
                                # support Line, Arc, Circle for Sketch as Base at the moment
                                if isinstance(i.Geometry, (Part.LineSegment, Part.Circle, Part.ArcOfCircle)):
                                    skGeomEdgesI = i.Geometry.toShape()
                                    skGeomEdges.append(skGeomEdgesI)
                        for cluster in Part.getSortedClusters(skGeomEdges):
                            clusterTransformed = []
                            for edge in cluster:
                                edge.Placement = edge.Placement.multiply(skPlacement)  ## TODO add attribute to skip Transform...
                                clusterTransformed.append(edge)
                            # Only use cluster of edges rather than turning into wire
                            self.basewires.append(clusterTransformed)

                        # Use Sketch's Normal for all edges/wires generated
                        # from sketch for consistency. Discussion on checking
                        # normal of sketch.Placement vs
                        # sketch.getGlobalPlacement() -
                        # https://forum.freecad.org/viewtopic.php?f=22&t=39341&p=334275#p334275
                        # normal = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))
                        normal = obj.Base.getGlobalPlacement().Rotation.multVec(FreeCAD.Vector(0,0,1))

                    else:
                        self.basewires = obj.Base.Shape.Wires

                        # Found case that after sorting below, direction of
                        # edges sorted are not as 'expected' thus resulted in
                        # bug - e.g. a Dwire with edges/vertexes in clockwise
                        # order, 1st vertex is Forward as expected.  After
                        # sorting below, edges sorted still in clockwise order
                        # - no problem, but 1st vertex of each edge become
                        # Reverse rather than Forward.

                        # See FC discussion -
                        # https://forum.freecad.org/viewtopic.php?f=23&t=48275&p=413745#p413745

                        #self.basewires = []
                        #for cluster in Part.getSortedClusters(obj.Base.Shape.Edges):
                        #    for c in Part.sortEdges(cluster):
                        #        self.basewires.append(Part.Wire(c))
                        # if not sketch, e.g. Dwire, can have wire which is 3d
                        # so not on the placement's working plane - below
                        # applied to Sketch not applicable here
                        #normal = obj.Base.getGlobalPlacement().Rotation.multVec(FreeCAD.Vector(0,0,1))
                        #normal = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,1))

                    if self.basewires:
                        if (len(self.basewires) == 1) and layers:
                            self.basewires = [self.basewires[0] for l in layers]
                        layeroffset = 0
                        baseface = None

                        for i,wire in enumerate(self.basewires):

                            # Check number of edges per 'wire' and get the 1st edge
                            if isinstance(wire,Part.Wire):
                                edgeNum = len(wire.Edges)
                                e = wire.Edges[0]
                            elif isinstance(wire[0],Part.Edge):
                                edgeNum = len(wire)
                                e = wire[0]

                            for n in range(0,edgeNum,1):  # why these not work - range(edgeNum), range(0,edgeNum) ...

                                # Fill the aligns list with ArchWall's default
                                # align entry and with same number of items as
                                # number of edges
                                try:
                                    if aligns[n] not in ['Left', 'Right', 'Center']:
                                        aligns[n] = align
                                except Exception:
                                    aligns.append(align)

                                # Fill the widths List with ArchWall's default
                                # width entry and with same number of items as
                                # number of edges
                                try:
                                    if not widths[n]:
                                        widths[n] = width
                                except Exception:
                                    widths.append(width)

                            # Get a direction vector orthogonal to both the
                            # normal of the face/sketch and the direction the
                            # wire was drawn in. IE: along the width direction
                            # of the wall.
                            if isinstance(e.Curve,(Part.Circle,Part.Ellipse)):
                                dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
                            else:
                                dvec = DraftGeomUtils.vec(e).cross(normal)

                            if not DraftVecUtils.isNull(dvec):
                                dvec.normalize()
                            face = None

                            curAligns = aligns[0]
                            off = obj.Offset.Value

                            if curAligns == "Left":

                                if layers:
                                    curWidth = []
                                    for n in range(edgeNum):
                                        curWidth.append(abs(layers[i]))
                                    off = off+layeroffset
                                    dvec.multiply(curWidth[0])
                                    layeroffset += abs(curWidth[0])
                                else:
                                    curWidth = widths
                                    dvec.multiply(width)

                                # Now DraftGeomUtils.offsetWire() support
                                # similar effect as ArchWall Offset
                                #
                                #if off:
                                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)

                                # Get the 'offseted' wire taking into account
                                # of Width and Align of each edge, and overall
                                # Offset
                                w2 = DraftGeomUtils.offsetWire(wire, dvec,
                                                               bind=False,
                                                               occ=False,
                                                               widthList=curWidth,
                                                               offsetMode=None,
                                                               alignList=aligns,
                                                               normal=normal,
                                                               basewireOffset=off)

                                # Get the 'base' wire taking into account of
                                # width and align of each edge
                                w1 = DraftGeomUtils.offsetWire(wire, dvec,
                                                               bind=False,
                                                               occ=False,
                                                               widthList=curWidth,
                                                               offsetMode="BasewireMode",
                                                               alignList=aligns,
                                                               normal=normal,
                                                               basewireOffset=off)

                                face = DraftGeomUtils.bind(w1, w2, per_segment=True)

                            elif curAligns == "Right":
                                dvec = dvec.negative()

                                if layers:
                                    curWidth = []
                                    for n in range(edgeNum):
                                        curWidth.append(abs(layers[i]))
                                    off = off+layeroffset
                                    dvec.multiply(curWidth[0])
                                    layeroffset += abs(curWidth[0])
                                else:
                                    curWidth = widths
                                    dvec.multiply(width)

                                # Now DraftGeomUtils.offsetWire() support similar effect as ArchWall Offset
                                #
                                #if off:
                                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)


                                w2 = DraftGeomUtils.offsetWire(wire, dvec,
                                                               bind=False,
                                                               occ=False,
                                                               widthList=curWidth,
                                                               offsetMode=None,
                                                               alignList=aligns,
                                                               normal=normal,
                                                               basewireOffset=off)

                                w1 = DraftGeomUtils.offsetWire(wire, dvec,
                                                               bind=False,
                                                               occ=False,
                                                               widthList=curWidth,
                                                               offsetMode="BasewireMode",
                                                               alignList=aligns,
                                                               normal=normal,
                                                               basewireOffset=off)

                                face = DraftGeomUtils.bind(w1, w2, per_segment=True)

                            #elif obj.Align == "Center":
                            elif curAligns == "Center":
                                if layers:
                                    totalwidth=sum([abs(l) for l in layers])
                                    curWidth = abs(layers[i])
                                    off = totalwidth/2-layeroffset
                                    d1 = Vector(dvec).multiply(off)
                                    w1 = DraftGeomUtils.offsetWire(wire, d1)
                                    layeroffset += curWidth
                                    off = totalwidth/2-layeroffset
                                    d1 = Vector(dvec).multiply(off)
                                    w2 = DraftGeomUtils.offsetWire(wire, d1)
                                else:
                                    dvec.multiply(width)

                                    w2 = DraftGeomUtils.offsetWire(wire, dvec,
                                                                   bind=False,
                                                                   occ=False,
                                                                   widthList=widths,
                                                                   offsetMode=None,
                                                                   alignList=aligns,
                                                                   normal=normal,
                                                                   basewireOffset=off)
                                    w1 = DraftGeomUtils.offsetWire(wire, dvec,
                                                                   bind=False,
                                                                   occ=False,
                                                                   widthList=widths,
                                                                   offsetMode="BasewireMode",
                                                                   alignList=aligns,
                                                                   normal=normal,
                                                                   basewireOffset=off)
                                face = DraftGeomUtils.bind(w1, w2, per_segment=True)

                            del widths[0:edgeNum]
                            del aligns[0:edgeNum]
                            if face:

                                if layers and (layers[i] < 0):
                                    # layers with negative values are not drawn
                                    continue

                                if baseface:

                                    # To allow exportIFC.py to work properly on
                                    # sketch, which use only 1st face / wire,
                                    # do not fuse baseface here So for a sketch
                                    # with multiple wires, each returns
                                    # individual face (rather than fusing
                                    # together) for exportIFC.py to work
                                    # properly
                                    # "ArchWall - Based on Sketch Issues" - https://forum.freecad.org/viewtopic.php?f=39&t=31235

                                    # "Bug #2408: [PartDesign] .fuse is splitting edges it should not"
                                    # - https://forum.freecad.org/viewtopic.php?f=10&t=20349&p=346237#p346237
                                    # - bugtracker - https://freecad.org/tracker/view.php?id=2408

                                    # Try Part.Shell before removeSplitter
                                    # - https://forum.freecad.org/viewtopic.php?f=10&t=20349&start=10
                                    # - 1st finding : if a rectangle + 1 line, can't removesSplitter properly...
                                    # - 2nd finding : if 2 faces do not touch, can't form a shell; then, subsequently for remaining faces even though touch each faces, can't form a shell

                                    baseface.append(face)
                                    # The above make Refine methods below (in else) useless, regardless removeSpitters yet to be improved for cases do not work well
                                    '''  Whether layers or not, all baseface.append(face) '''

                                else:
                                    baseface = [face]

                                    '''  Whether layers or not, all baseface = [face] '''

                        if baseface:
                            base,placement = self.rebase(baseface)
        else:
            if layers:
                totalwidth = sum([abs(l) for l in layers])
                offset = 0
                base = []
                for l in layers:
                    if l > 0:
                        l2 = length/2 or 0.5
                        w1 = -totalwidth/2 + offset
                        w2 = w1 + l
                        v1 = Vector(-l2,w1,0)
                        v2 = Vector(l2,w1,0)
                        v3 = Vector(l2,w2,0)
                        v4 = Vector(-l2,w2,0)
                        base.append(Part.Face(Part.makePolygon([v1,v2,v3,v4,v1])))
                    offset += abs(l)
            else:
                l2 = length/2 or 0.5
                w2 = width/2 or 0.5
                v1 = Vector(-l2,-w2,0)
                v2 = Vector(l2,-w2,0)
                v3 = Vector(l2,w2,0)
                v4 = Vector(-l2,w2,0)
                base = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
            placement = FreeCAD.Placement()
        if base and placement:
            normal.normalize()
            extrusion = normal.multiply(height)
            if placement.Rotation.Angle > 0:
                extrusion = placement.inverse().Rotation.multVec(extrusion)
            return (base,extrusion,placement)
        return None

class _ViewProviderWall(ArchComponent.ViewProviderComponent):
    """The view provider for the wall object.

    Parameters
    ----------
    vobj: <Gui.ViewProviderDocumentObject>
        The view provider to turn into a wall view provider.
    """

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Wall")

    def getIcon(self):
        """Return the path to the appropriate icon.

        If a clone, return the cloned wall icon path. Otherwise return the
        Arch wall icon.

        Returns
        -------
        str
            Path to the appropriate icon .svg file.
        """

        import Arch_rc
        if hasattr(self,"Object"):
            if self.Object.CloneOf:
                return ":/icons/Arch_Wall_Clone.svg"
            elif (not self.Object.Base) and self.Object.Additions:
                return ":/icons/Arch_Wall_Tree_Assembly.svg"
        return ":/icons/Arch_Wall_Tree.svg"

    def attach(self,vobj):
        """Add display modes' data to the coin scenegraph.

        Add each display mode as a coin node, whose parent is this view
        provider.

        Each display mode's node includes the data needed to display the object
        in that mode. This might include colors of faces, or the draw style of
        lines. This data is stored as additional coin nodes which are children
        of the display mode node.

        Add the textures used in the Footprint display mode.
        """

        self.Object = vobj.Object
        from pivy import coin
        tex = coin.SoTexture2()
        image = Draft.loadTexture(Draft.svgpatterns()['simple'][1], 128)
        if not image is None:
            tex.image = image
        texcoords = coin.SoTextureCoordinatePlane()
        s = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetFloat("patternScale",0.01)
        texcoords.directionS.setValue(s,0,0)
        texcoords.directionT.setValue(0,s,0)
        self.fcoords = coin.SoCoordinate3()
        self.fset = coin.SoIndexedFaceSet()
        sep = coin.SoSeparator()
        sep.addChild(tex)
        sep.addChild(texcoords)
        sep.addChild(self.fcoords)
        sep.addChild(self.fset)
        vobj.RootNode.addChild(sep)
        ArchComponent.ViewProviderComponent.attach(self,vobj)

    def updateData(self,obj,prop):
        """Method called when the host object has a property changed.

        If the host object's Placement, Shape, or Material has changed, and the
        host object has a Material assigned, give the shape the color and
        transparency of the Material.

        Parameters
        ----------
        obj: <App::FeaturePython>
            The host object that has changed.
        prop: string
            The name of the property that has changed.
        """

        if prop in ["Placement","Shape","Material"]:
            if obj.ViewObject.DisplayMode == "Footprint":
                obj.ViewObject.Proxy.setDisplayMode("Footprint")
            if hasattr(obj,"Material"):
                if obj.Material and obj.Shape:
                    if hasattr(obj.Material,"Materials"):
                        activematerials = [obj.Material.Materials[i] for i in range(len(obj.Material.Materials)) if obj.Material.Thicknesses[i] >= 0]
                        if len(activematerials) == len(obj.Shape.Solids):
                            cols = []
                            for i,mat in enumerate(activematerials):
                                c = obj.ViewObject.ShapeColor
                                c = (c[0],c[1],c[2],obj.ViewObject.Transparency/100.0)
                                if 'DiffuseColor' in mat.Material:
                                    if "(" in mat.Material['DiffuseColor']:
                                        c = tuple([float(f) for f in mat.Material['DiffuseColor'].strip("()").split(",")])
                                if 'Transparency' in mat.Material:
                                    c = (c[0],c[1],c[2],float(mat.Material['Transparency']))
                                cols.extend([c for j in range(len(obj.Shape.Solids[i].Faces))])
                            obj.ViewObject.DiffuseColor = cols
        ArchComponent.ViewProviderComponent.updateData(self,obj,prop)
        if len(obj.ViewObject.DiffuseColor) > 1:
            # force-reset colors if changed
            obj.ViewObject.DiffuseColor = obj.ViewObject.DiffuseColor

    def getDisplayModes(self,vobj):
        """Define the display modes unique to the Arch Wall.

        Define mode Footprint, which only displays the footprint of the wall.
        Also add the display modes of the Arch Component.

        Returns
        -------
        list of str
            List containing the names of the new display modes.
        """

        modes = ArchComponent.ViewProviderComponent.getDisplayModes(self,vobj)+["Footprint"]
        return modes

    def setDisplayMode(self,mode):
        """Method called when the display mode changes.

        Called when the display mode changes, this method can be used to set
        data that wasn't available when .attach() was called.

        When Footprint is set as display mode, find the faces that make up the
        footprint of the wall, and give them a lined texture. Then display
        the wall as a wireframe.

        Then pass the displaymode onto Arch Component's .setDisplayMode().

        Parameters
        ----------
        mode: str
            The name of the display mode the view provider has switched to.

        Returns
        -------
        str:
            The name of the display mode the view provider has switched to.
        """

        self.fset.coordIndex.deleteValues(0)
        self.fcoords.point.deleteValues(0)
        if mode == "Footprint":
            if hasattr(self,"Object"):
                faces = self.Object.Proxy.getFootprint(self.Object)
                if faces:
                    verts = []
                    fdata = []
                    idx = 0
                    for face in faces:
                        tri = face.tessellate(1)
                        for v in tri[0]:
                            verts.append([v.x,v.y,v.z])
                        for f in tri[1]:
                            fdata.extend([f[0]+idx,f[1]+idx,f[2]+idx,-1])
                        idx += len(tri[0])
                    self.fcoords.point.setValues(verts)
                    self.fset.coordIndex.setValues(0,len(fdata),fdata)
            return "Wireframe"
        return ArchComponent.ViewProviderComponent.setDisplayMode(self,mode)

    def setupContextMenu(self, vobj, menu):
        super().contextMenuAddEdit(menu)

        actionFlipDirection = QtGui.QAction(QtGui.QIcon(":/icons/Arch_Wall_Tree.svg"),
                                            translate("Arch", "Flip direction"),
                                            menu)
        QtCore.QObject.connect(actionFlipDirection,
                               QtCore.SIGNAL("triggered()"),
                               self.flipDirection)
        menu.addAction(actionFlipDirection)

        super().contextMenuAddToggleSubcomponents(menu)

    def flipDirection(self):

        if hasattr(self,"Object") and self.Object:
            obj = self.Object
            if obj.Align == "Left":
                obj.Align = "Right"
                FreeCAD.ActiveDocument.recompute()
            elif obj.Align == "Right":
                obj.Align = "Left"
                FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Wall',_CommandWall())
    FreeCADGui.addCommand('Arch_MergeWalls',_CommandMergeWalls())
