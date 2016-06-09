# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

import FreeCAD, Path
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate

from . import PathUtils
from .PathUtils import fmt

"""Helix Drill object and FreeCAD command"""

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

def hollow_cylinder(cyl):
    """Test if this is a hollow cylinder"""
    from Part import Circle
    circle1 = None
    line = None
    for edge in cyl.Edges:
        if isinstance(edge.Curve, Circle):
            if circle1 is None:
                circle1 = edge
            else:
                circle2 = edge
        else:
            line = edge
    center = (circle1.Curve.Center + circle2.Curve.Center).scale(0.5, 0.5, 0.5)
    p = (circle1.valueAt(circle1.ParameterRange[0]) + circle2.valueAt(circle1.ParameterRange[0])).scale(0.5, 0.5, 0.5)
    to_outside = (p - center).normalize()
    u, v = cyl.Surface.parameter(p)
    normal = cyl.normalAt(u, v).normalize()

    cos_a = to_outside.dot(normal)

    if cos_a > 1.0 - 1e-12:
        return False
    elif cos_a < -1.0 + 1e-12:
        return True
    else:
        raise Exception("Strange cylinder encountered, cannot determine if it is hollow or not")

def z_cylinder(cyl):
    """ Test if cylinder is aligned to z-Axis"""
    if cyl.Surface.Axis.x != 0.0:
        return False
    if cyl.Surface.Axis.y != 0.0:
        return False
    return True


def connected(edge, face):
    for otheredge in face.Edges:
        if edge.isSame(otheredge):
            return True
    return False


def helix_cut(center, r_out, r_in, dr, zmax, zmin, dz, safe_z, tool_diameter, vfeed, hfeed, direction, startside):
    """
    center: 2-tuple
      (x0,y0) coordinates of center
    r_out, r_in: floats
      radial range, cut from outer radius r_out in layers of dr to inner radius r_in
    zmax, zmin: floats
      z-range, cut from zmax in layers of dz down to zmin
    safe_z: float
      safety layer height
    tool_diameter: float
      Width of tool
    """
    from numpy import ceil, allclose, linspace

    if (zmax <= zmin):
        return

    out = "(helix_cut <{0}, {1}>, {2})".format(center[0], center[1],
                ", ".join(map(str, (r_out, r_in, dr, zmax, zmin, dz, safe_z, tool_diameter, vfeed, hfeed, direction, startside))))

    x0, y0 = center
    nz = max(int(ceil((zmax - zmin)/dz)), 2)
    zi = linspace(zmax, zmin, 2 * nz + 1)

    if dr > tool_diameter:
        FreeCAD.Console.PrintWarning("PathHelix: Warning, shortening dr to tool diameter!\n")
        dr = tool_diameter

    def xyz(x=None, y=None, z=None):
        out = ""
        if x is not None:
            out += " X" + fmt(x)
        if y is not None:
            out += " Y" + fmt(y)
        if z is not None:
            out += " Z" + fmt(z)
        return out

    def rapid(x=None, y=None, z=None):
        return "G0" + xyz(x,y,z) + "\n"

    def F(f=None):
        return (" F" + fmt(f) if f else "")

    def feed(x=None, y=None, z=None, f=None):
        return "G1" + xyz(x,y,z) + F(f) + "\n"

    def arc(x,y,i,j,z,f):
        if direction == "CW":
            code = "G2"
        elif direction == "CCW":
            code = "G3"
        return code + " I" + fmt(i) + " J" + fmt(j) + " X" + fmt(x) + " Y" + fmt(y) + " Z" + fmt(z) + F(f) + "\n"

    def helix_cut_r(r):
        out = ""
        out += rapid(x=x0+r,y=y0)
        out += rapid(z=zmax + tool_diameter)
        out += feed(z=zmax,f=vfeed)
        z=zmin
        for i in range(1,nz+1):
            out += arc(x0-r, y0, i=-r, j=0.0, z = zi[2*i-1], f=hfeed)
            out += arc(x0+r, y0, i= r, j=0.0, z = zi[2*i],   f=hfeed)
        out += arc(x0-r, y0, i=-r, j=0.0, z = zmin, f=hfeed)
        out += arc(x0+r, y0, i=r,  j=0.0, z = zmin, f=hfeed)
        out += feed(z=zmax + tool_diameter, f=vfeed)
        out += rapid(z=safe_z)
        return out

    assert(r_out > 0.0)
    assert(r_in >=  0.0)

    msg = None
    if r_out < 0.0:
        msg = "r_out < 0"
    elif r_in > 0 and r_out - r_in < tool_diameter:
        msg = "r_out - r_in = {0} is < tool diameter of {1}".format(r_out - r_in, tool_diamater)
    elif r_in == 0.0 and not r_out > tool_diameter/2.:
        msg = "Cannot drill a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, tool_diameter)
    elif not startside in ["inside", "outside"]:
        msg = "Invalid value for parameter 'startside'"

    if msg:
        out += "(ERROR: Hole at {0}:".format((x0, y0, zmax)) + msg + ")\n"
        FreeCAD.Console.PrintError("PathHelix: Hole at {0}:".format((x0, y0, zmax)) + msg + "\n")
        return out

    if r_in > 0:
        out += "(annulus mode)\n"
        r_out = r_out - tool_diameter/2
        r_in  = r_in  + tool_diameter/2
        if abs((r_out - r_in) / dr) < 1e-5:
            radii = [(r_out + r_in)/2]
        else:
            nr = max(int(ceil((r_out - r_in)/dr)), 2)
            radii = linspace(r_out, r_in, nr)
    elif r_out < dr:
        out += "(single helix mode)\n"
        radii = [r_out - tool_diameter/2]
        assert(radii[0] > 0)
    else:
        out += "(full hole mode)\n"
        r_out = r_out - tool_diameter/2
        r_in  = dr/2

        nr = max(1 + int(ceil((r_out - r_in)/dr)), 2)
        radii = linspace(r_out, r_in, nr)
        assert(all(radii > 0))

    if startside == "inside":
        radii = radii[::-1]

    for r in radii:
        out += "(radius {0})\n".format(r)
        out += helix_cut_r(r)

    return out

class ObjectPathHelix(object):

    def __init__(self,obj):
        # Basic
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("Parent Object","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyLinkSubList","Features","Path",translate("Features","Selected features for the drill operation"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Set to False to disable code generation"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("Comment","An optional comment for this profile, will appear in G-Code"))

        # Helix specific
        obj.addProperty("App::PropertyEnumeration", "Direction", "Helix Drill",
            translate("Direction", "The direction of the circular cuts, clockwise (CW), or counter clockwise (CCW)"))
        obj.Direction = ['CW','CCW']

        obj.addProperty("App::PropertyEnumeration", "StartSide", "Helix Drill",
            translate("Direction", "Start cutting from the inside or outside"))
        obj.StartSide = ['inside','outside']

        obj.addProperty("App::PropertyLength", "DeltaR", "Helix Drill",
            translate("DeltaR", "Radius increment (must be smaller than tool diameter)"))
        obj.addProperty("App::PropertyBool", "Recursive", "Helix Drill",
            translate("Recursive", "If True, drill holes also in any subsequent smaller holes at the bottom of a hole"))

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "Clearance", "Depths",
            translate("Clearance","Safe distance above the top of the hole to which to retract the tool"))
        obj.addProperty("App::PropertyLength", "StepDown", "Depths",
            translate("StepDown","Incremental Step Down of Tool"))
        obj.addProperty("App::PropertyBool","UseStartDepth","Depths",
            translate("Use Start Depth","Set to True to manually specify a start depth"))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depths",
            translate("Start Depth","Starting Depth of Tool - first cut depth in Z"))
        obj.addProperty("App::PropertyBool","UseFinalDepth","Depths",
            translate("Use Final Depth","Set to True to manually specify a final depth"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depths",
            translate("Final Depth","Final Depth of Tool - lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "ThroughDepth", "Depths",
            translate("Through Depth","Add this amount of additional cutting depth to open-ended holes. Only used if UseFinalDepth is False"))

        # Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feeds",
            translate("Vert Feed","Feed rate for vertical mill moves, this includes the actual arcs"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feeds",
            translate("Horiz Feed","Feed rate for horizontal mill moves, these are mostly retractions to the safe distance above the object"))

        # The current tool number, read-only
        # this is apparently used internally, to keep track of tool chagnes
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The current tool in use"))
        obj.ToolNumber = (0,0,1000,1)
        obj.setEditorMode('ToolNumber',1) #make this read only

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
        from Part import Circle, Cylinder, Plane
        from math import sqrt
        if obj.Base:
            if not obj.Active:
                obj.Path = Path.Path("(helix cut operation inactive)")
                obj.ViewObject.Visibility = False
                return

            if not len(obj.InList) > 0:
                FreeCAD.Console.PrintError("PathHelix: Operation is not part of a project\n")
                obj.Path = Path.Path("(helix cut operation not part of any project)")
                obj.ViewObject.Visibility = False
                return

            project = obj.InList[0]
            obj.ToolNumber = int(PathUtils.changeTool(obj,project))
            tool = PathUtils.getTool(obj,obj.ToolNumber)

            if not tool:
                FreeCAD.Console.PrintError("PathHelix: No tool selected for helix cut operation, insert a tool change operation first\n")
                obj.Path = Path.Path("(ERROR: no tool selected for helix cut operation)")
                return

            def connected_cylinders(base, edge):
                cylinders = []
                for face in base.Shape.Faces:
                    if isinstance(face.Surface, Cylinder):
                        if connected(edge, face):
                            if z_cylinder(face):
                                cylinders.append((base, face))
                return cylinders

            cylinders = []

            for base, feature in obj.Features:
                subobj = getattr(base.Shape, feature)
                if subobj.ShapeType =='Face':
                    if isinstance(subobj.Surface, Cylinder):
                        if z_cylinder(subobj):
                            cylinders.append((base, subobj))
                    else:
                        # brute force triple-loop as FreeCAD does not expose
                        # any topology information...
                        for edge in subobj.Edges:
                            cylinders.extend(filter(lambda b_c: hollow_cylinder(b_c[1]), (connected_cylinders(base, edge))))

                if subobj.ShapeType == 'Edge':
                    cylinders.extend(connected_cylinders(base, subobj))

            output = '(helix cut operation'
            if obj.Comment:
                output  += ', '+ str(obj.Comment)+')\n'
            else:
                output  += ')\n'

            output += "G0 Z" + fmt(obj.Base[0].Shape.BoundBox.ZMax + float(obj.Clearance))

            drill_jobs = []

            for base, cylinder in cylinders:
                zsafe = cylinder.BoundBox.ZMax + obj.Clearance.Value
                xc, yc, zc = cylinder.Surface.Center

                if obj.Recursive:
                    cur_z = cylinder.BoundBox.ZMax
                    jobs = []

                    while cylinder:
                        # Find other edge of current cylinder
                        other_edge = None
                        for edge in cylinder.Edges:
                            if isinstance(edge.Curve, Circle) and edge.Curve.Center.z != cur_z:
                                other_edge = edge
                                break

                        next_z = other_edge.Curve.Center.z
                        dz = next_z - cur_z
                        r = cylinder.Surface.Radius

                        if dz < 0:
                            # This is a closed hole if the face connected to the current cylinder at next_z has
                            # the cylinder's edge as its OuterWire
                            closed = None
                            for face in base.Shape.Faces:
                                if connected(other_edge, face) and not face.isSame(cylinder.Faces[0]):
                                    wire = face.OuterWire
                                    if len(wire.Edges) == 1 and wire.Edges[0].isSame(other_edge):
                                        closed = True
                                    else:
                                        closed = False

                            if closed is None:
                                raise Exception("Cannot determine if this cylinder is closed on the z = {0} side".format(next_z))

                            jobs.append(dict(xc=xc, yc=yc, zmin=next_z, zmax=cur_z, r_out=r, r_in=0.0, closed=closed, zsafe=zsafe))

                        elif dz > 0:
                            new_jobs = []
                            for job in jobs:
                                if job["zmin"] < next_z < job["zmax"]:
                                    # split this job
                                    job1 = dict(job)
                                    job2 = dict(job)
                                    job1["zmin"] = next_z
                                    job2["zmax"] = next_z
                                    job2["r_in"] = r
                                    new_jobs.append(job1)
                                    new_jobs.append(job2)
                                else:
                                    new_jobs.append(job)
                            jobs = new_jobs
                        else:
                            FreeCAD.Console.PrintWarning("PathHelix: Encountered cylinder with zero height\n")
                            break

                        cur_z = next_z
                        cylinder = None
                        faces = []
                        for face in base.Shape.Faces:
                            if connected(other_edge, face):
                                if isinstance(face.Surface, Plane):
                                    faces.append(face)
                        # should only be one
                        face, = faces
                        for edge in face.Edges:
                            if not edge.isSame(other_edge):
                                for base, other_cylinder in connected_cylinders(base, edge):
                                    xo = other_cylinder.Surface.Center.x
                                    yo = other_cylinder.Surface.Center.y
                                    center_dist = sqrt((xo - xc)**2 + (yo - yc)**2)
                                    if center_dist + other_cylinder.Surface.Radius < r:
                                        cylinder = other_cylinder
                                        break

                    if obj.UseStartDepth:
                        jobs = [job for job in jobs if job["zmin"] < obj.StartDepth.Value]
                        if jobs:
                            jobs[0]["zmax"] = obj.StartDepth.Value
                    if obj.UseFinalDepth:
                        jobs = [job for job in jobs if job["zmax"] > obj.FinalDepth.Value]
                        if jobs:
                            jobs[-1]["zmin"] = obj.FinalDepth.Value
                    else:
                        if not jobs[-1]["closed"]:
                            jobs[-1]["zmin"] -= obj.ThroughDepth.Value

                    drill_jobs.extend(jobs)
                else:
                    if obj.UseStartDepth:
                        zmax = obj.StartDepth.Value
                    else:
                        zmax = cylinder.BoundBox.ZMax
                    if obj.UseFinalDepth:
                        zmin = obj.FinalDepth.Value
                    else:
                        zmin = cylinder.BoundBox.ZMin - obj.ThroughDepth.Value
                    drill_jobs.append(dict(xc=xc, yc=yc, zmin=zmin, zmax=zmax, r_out=cylinder.Surface.Radius, r_in=0.0, zsafe=zsafe))

            for job in drill_jobs:
                output += helix_cut((job["xc"], job["yc"]), job["r_out"], job["r_in"], obj.DeltaR.Value,
                                    job["zmax"], job["zmin"], obj.StepDown.Value,
                                    job["zsafe"], tool.Diameter,
                                    obj.VertFeed.Value, obj.HorizFeed.Value, obj.Direction, obj.StartSide)
                output += '\n'

            obj.Path = Path.Path(output)
            if obj.ViewObject:
                obj.ViewObject.Visibility = True


class ViewProviderPathHelix(object):
    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Helix.svg"

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskpanel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskpanel)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

class CommandPathHelix(object):
    def GetResources(self):
        return {'Pixmap'  : 'Path-Helix',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathHelix","PathHelix"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathHelix","Creates a helix cut from selected circles")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        import FreeCADGui
        import Path
        from PathScripts import PathUtils

        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) == 1:
            FreeCAD.Console.PrintError("Only considering first object for PathHelix!\n")
        selection = selection[0]

        if not len(selection.SubElementNames) > 0:
            FreeCAD.Console.PrintError("Select a face or circles to create helix cuts\n")

        # register the transaction for the undo stack
        try:
            FreeCAD.ActiveDocument.openTransaction(translate("PathHelix","Create a helix cut"))
            FreeCADGui.addModule("PathScripts.PathHelix")

            obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","PathHelix")
            ObjectPathHelix(obj)
            ViewProviderPathHelix(obj.ViewObject)

            obj.Base = selection.Object
            obj.Features = [(selection.Object, subobj) for subobj in selection.SubElementNames]
            obj.DeltaR = 1.0

            project = PathUtils.addToProject(obj)
            tl = PathUtils.changeTool(obj,project)
            if tl:
                obj.ToolNumber = tl
                tool = PathUtils.getTool(obj,obj.ToolNumber)
                if tool:
                    # start with 25% overlap
                    obj.DeltaR = tool.Diameter * 0.75

            obj.Active = True
            obj.Comment = ""

            obj.Direction = "CW"
            obj.StartSide = "inside"

            obj.Clearance = 10.0
            obj.StepDown = 1.0
            obj.UseStartDepth = False
            obj.StartDepth = 1.0
            obj.UseFinalDepth = False
            obj.FinalDepth = 0.0
            obj.ThroughDepth = 0.0
            obj.Recursive = True

            obj.VertFeed = 0.0
            obj.HorizFeed = 0.0

            # commit
            FreeCAD.ActiveDocument.commitTransaction()

        except:
            FreeCAD.ActiveDocument.abortTransaction()
            raise

        FreeCAD.ActiveDocument.recompute()

class TaskPanel(object):
    def __init__(self, obj):
        from Units import Quantity
        self.obj = obj

        ui = FreeCADGui.UiLoader()
        layout = QtGui.QGridLayout()

        headerStyle = "QLabel { font-weight: bold; font-size: large; }"
        grayed_out = "background-color: #d0d0d0;"

        self.previous_value = {}

        def addWidget(widget):
            row = layout.rowCount()
            layout.addWidget(widget, row, 0, columnSpan=2)

        def addWidgets(widget1, widget2):
            row = layout.rowCount()
            layout.addWidget(widget1, row, 0)
            layout.addWidget(widget2, row, 1)

        def heading(label):
            heading = QtGui.QLabel(label)
            heading.setStyleSheet(headerStyle)
            addWidget(heading)

        def addQuantity(property, label, activator=None, max=None):
            self.previous_value[property] = getattr(self.obj, property)
            widget = ui.createWidget("Gui::InputField")

            if activator:
                self.previous_value[activator] = getattr(self.obj, activator)
                currently_active = getattr(self.obj, activator)
                label = QtGui.QCheckBox(label)
                def change(state):
                    setattr(self.obj, activator, label.isChecked())
                    if label.isChecked():
                        widget.setStyleSheet("")
                    else:
                        widget.setStyleSheet(grayed_out)
                    self.obj.Proxy.execute(self.obj)
                    FreeCAD.ActiveDocument.recompute()
                label.stateChanged.connect(change)
                label.setChecked(currently_active)
                if not currently_active:
                    widget.setStyleSheet(grayed_out)
                label.setToolTip(self.obj.getDocumentationOfProperty(activator))
            else:
                label = QtGui.QLabel(label)
                label.setToolTip(self.obj.getDocumentationOfProperty(property))

            widget.setText(str(getattr(self.obj, property)))
            widget.setToolTip(self.obj.getDocumentationOfProperty(property))

            if max:
                # cannot use widget.setMaximum() as apparently ui.createWidget()
                # returns the object up-casted to QWidget.
                widget.setProperty("maximum", max)

            def change(quantity):
                if activator:
                    label.setChecked(True)
                setattr(self.obj, property, quantity)
                self.obj.Proxy.execute(self.obj)
                FreeCAD.ActiveDocument.recompute()

            QtCore.QObject.connect(widget, QtCore.SIGNAL("valueChanged(const Base::Quantity &)"), change)

            addWidgets(label, widget)
            return label, widget

        def addCheckBox(property, label):
            self.previous_value[property] = getattr(self.obj, property)
            widget = QtGui.QCheckBox(label)
            widget.setToolTip(self.obj.getDocumentationOfProperty(property))

            def change(state):
                setattr(self.obj, property, widget.isChecked())
                self.obj.Proxy.execute(self.obj)
                FreeCAD.ActiveDocument.recompute()
            widget.stateChanged.connect(change)

            widget.setChecked(getattr(self.obj, property))
            addWidget(widget)

        def addEnumeration(property, label, options):
            self.previous_value[property] = getattr(self.obj, property)
            label = QtGui.QLabel(label)
            label.setToolTip(self.obj.getDocumentationOfProperty(property))
            widget = QtGui.QComboBox()
            widget.setToolTip(self.obj.getDocumentationOfProperty(property))
            for option_label, option_value in options:
                widget.addItem(option_label)
            def change(index):
                setattr(self.obj, property, options[index][1])
                self.obj.Proxy.execute(self.obj)
                FreeCAD.ActiveDocument.recompute()
            widget.currentIndexChanged.connect(change)
            addWidgets(label, widget)

        heading("Drill parameters")
        addCheckBox("Active", "Operation is active")
        addCheckBox("Recursive", "Also mill subsequent holes")
        tool = PathUtils.getTool(self.obj,self.obj.ToolNumber)
        if not tool:
            drmax = None
        else:
            drmax = tool.Diameter
        addQuantity("DeltaR", "Step in Radius", max=drmax)
        addQuantity("StepDown", "Step in Z")
        addEnumeration("Direction", "Cut direction", [("Clockwise", "CW"), ("Counter-Clockwise", "CCW")])
        addEnumeration("StartSide", "Start Side", [("Start from inside", "inside"), ("Start from outside", "outside")])

        heading("Cutting Depths")
        addQuantity("Clearance", "Clearance Distance")
        addQuantity("StartDepth", "Absolute start height", "UseStartDepth")

        fdcheckbox, fdinput = addQuantity("FinalDepth", "Absolute final height", "UseFinalDepth")
        tdlabel, tdinput = addQuantity("ThroughDepth", "Extra drill depth for open holes")

        heading("Feeds")
        addQuantity("HorizFeed", "Horizontal Feed")
        addQuantity("VertFeed", "Vertical Feed")

        # make ThroughDepth and FinalDepth mutually exclusive
        def fd_change(state):
            if fdcheckbox.isChecked():
                tdinput.setStyleSheet(grayed_out)
            else:
                tdinput.setStyleSheet("")
        fdcheckbox.stateChanged.connect(fd_change)

        def td_change(quantity):
            fdcheckbox.setChecked(False)
        QtCore.QObject.connect(tdinput, QtCore.SIGNAL("valueChanged(const Base::Quantity &)"), td_change)

        if obj.UseFinalDepth:
            tdinput.setStyleSheet(grayed_out)

        # add
        widget = QtGui.QWidget()
        widget.setLayout(layout)
        self.form = widget

    def needsFullSpace(self):
        return True

    def accept(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()

    def reject(self):
        for property in self.previous_value:
            setattr(self.obj, property, self.previous_value[property])
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()

if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.addCommand('Path_Helix',CommandPathHelix())
