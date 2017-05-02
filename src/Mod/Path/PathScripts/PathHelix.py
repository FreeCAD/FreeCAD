# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Lorenz Hüdepohl <dev@stellardeath.org>             *
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

from . import PathUtils
from .PathUtils import fmt

import FreeCAD
import Path
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate

"""Helix Drill object and FreeCAD command"""

if FreeCAD.GuiUp:
    try:
        _encoding = QtGui.QApplication.UnicodeUTF8

        def translate(context, text, disambig=None):
            return QtGui.QApplication.translate(context, text, disambig,
                                                _encoding)

    except AttributeError:

        def translate(context, text, disambig=None):
            return QtGui.QApplication.translate(context, text, disambig)
else:
    def translate(context, text, disambig=None):
        return text


def z_cylinder(cyl):
    """ Test if cylinder is aligned to z-Axis"""
    axis = cyl.Surface.Axis
    if abs(axis.x) > 1e-10 * abs(axis.z):
        return False
    if abs(axis.y) > 1e-10 * abs(axis.z):
        return False
    return True


def connected(edge, face):
    for otheredge in face.Edges:
        if edge.isSame(otheredge):
            return True
    return False


def cylinders_in_selection():
    from Part import Cylinder
    selections = FreeCADGui.Selection.getSelectionEx()

    cylinders = []

    for selection in selections:
        base = selection.Object
        cylinders.append((base, []))
        for feature in selection.SubElementNames:
            subobj = getattr(base.Shape, feature)
            if subobj.ShapeType == 'Face':
                if isinstance(subobj.Surface, Cylinder):
                    if z_cylinder(subobj):
                        cylinders[-1][1].append(feature)

    return cylinders


def helix_cut(center, r_out, r_in, dr, zmax, zmin, dz, safe_z, tool_diameter, vfeed, hfeed, direction, startside):
    """
    center: 2-tuple
      (x0, y0) coordinates of center
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
                ", ".join(map(str, (r_out, r_in, dr, zmax, zmin, dz, safe_z,
                                    tool_diameter, vfeed, hfeed, direction, startside))))

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
        return "G0" + xyz(x, y, z) + "\n"

    def F(f=None):
        return (" F" + fmt(f) if f else "")

    def feed(x=None, y=None, z=None, f=None):
        return "G1" + xyz(x, y, z) + F(f) + "\n"

    def arc(x, y, i, j, z, f):
        if direction == "CW":
            code = "G2"
        elif direction == "CCW":
            code = "G3"
        return code + " I" + fmt(i) + " J" + fmt(j) + " X" + fmt(x) + " Y" + fmt(y) + " Z" + fmt(z) + F(f) + "\n"

    def helix_cut_r(r):
        out = ""
        out += rapid(x=x0+r, y=y0)
        out += rapid(z=zmax + tool_diameter)
        out += feed(z=zmax, f=vfeed)
        z = zmin
        for i in range(1, nz+1):
            out += arc(x0-r, y0, i=-r, j=0.0, z=zi[2*i-1], f=hfeed)
            out += arc(x0+r, y0, i= r, j=0.0, z=zi[2*i],   f=hfeed)
        out += arc(x0-r, y0, i=-r, j=0.0, z=zmin, f=hfeed)
        out += arc(x0+r, y0, i=r,  j=0.0, z=zmin, f=hfeed)
        out += feed(z=zmax + tool_diameter, f=vfeed)
        out += rapid(z=safe_z)
        return out

    assert(r_out > 0.0)
    assert(r_in >= 0.0)

    msg = None
    if r_out < 0.0:
        msg = "r_out < 0"
    elif r_in > 0 and r_out - r_in < tool_diameter:
        msg = "r_out - r_in = {0} is < tool diameter of {1}".format(r_out - r_in, tool_diameter)
    elif r_in == 0.0 and not r_out > tool_diameter/2.:
        msg = "Cannot drill a hole of diameter {0} with a tool of diameter {1}".format(2 * r_out, tool_diameter)
    elif startside not in ["inside", "outside"]:
        msg = "Invalid value for parameter 'startside'"

    if msg:
        out += "(ERROR: Hole at {0}:".format((x0, y0, zmax)) + msg + ")\n"
        FreeCAD.Console.PrintError("PathHelix: Hole at {0}:".format((x0, y0, zmax)) + msg + "\n")
        return out

    if r_in > 0:
        out += "(annulus mode)\n"
        r_out = r_out - tool_diameter/2
        r_in = r_in + tool_diameter/2
        if abs((r_out - r_in) / dr) < 1e-5:
            radii = [(r_out + r_in)/2]
        else:
            nr = max(int(ceil((r_out - r_in)/dr)), 2)
            radii = linspace(r_out, r_in, nr)
    elif r_out <= 2 * dr:
        out += "(single helix mode)\n"
        radii = [r_out - tool_diameter/2]
        assert(radii[0] > 0)
    else:
        out += "(full hole mode)\n"
        r_out = r_out - tool_diameter/2
        r_in = dr/2

        nr = max(1 + int(ceil((r_out - r_in)/dr)), 2)
        radii = linspace(r_out, r_in, nr)
        assert(all(radii > 0))

    if startside == "inside":
        radii = radii[::-1]

    for r in radii:
        out += "(radius {0})\n".format(r)
        out += helix_cut_r(r)

    return out


def features_by_centers(base, features):
    try:
        from scipy.spatial import KDTree
    except ImportError:
        from PathScripts.kdtree import KDTree

    features = sorted(features,
                      key=lambda feature: getattr(base.Shape, feature).Surface.Radius,
                      reverse=True)

    coordinates = [(cylinder.Surface.Center.x, cylinder.Surface.Center.y) for cylinder in
                   [getattr(base.Shape, feature) for feature in features]]

    tree = KDTree(coordinates)
    seen = {}

    by_centers = {}
    for n, feature in enumerate(features):
        if n in seen:
            continue
        seen[n] = True

        cylinder = getattr(base.Shape, feature)
        xc, yc, _ = cylinder.Surface.Center
        by_centers[xc, yc] = {cylinder.Surface.Radius: feature}

        for coord in tree.query_ball_point((xc, yc), cylinder.Surface.Radius):
            seen[coord] = True
            cylinder = getattr(base.Shape, features[coord])
            by_centers[xc, yc][cylinder.Surface.Radius] = features[coord]

    return by_centers


class ObjectPathHelix(object):

    def __init__(self, obj):
        # Basic
        obj.addProperty("App::PropertyLink", "ToolController", "Path",
                        translate("App::Property", "The tool controller that will be used to calculate the path"))
        obj.addProperty("App::PropertyLinkSubList", "Features", "Path",
                        translate("Features", "Selected features for the drill operation"))
        obj.addProperty("App::PropertyBool", "Active", "Path",
                        translate("Active", "Set to False to disable code generation"))
        obj.addProperty("App::PropertyString", "Comment", "Path",
                        translate("Comment", "An optional comment for this profile, will appear in G-Code"))

        # Helix specific
        obj.addProperty("App::PropertyEnumeration", "Direction", "Helix Drill",
                        translate("Direction", "The direction of the circular cuts, clockwise (CW), or counter clockwise (CCW)"))
        obj.Direction = ['CW', 'CCW']

        obj.addProperty("App::PropertyEnumeration", "StartSide", "Helix Drill",
                        translate("Direction", "Start cutting from the inside or outside"))
        obj.StartSide = ['inside', 'outside']

        obj.addProperty("App::PropertyLength", "DeltaR", "Helix Drill",
                        translate("DeltaR", "Radius increment (must be smaller than tool diameter)"))

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "Clearance", "Depths",
                        translate("Clearance", "Safe distance above the top of the hole to which to retract the tool"))
        obj.addProperty("App::PropertyLength", "StepDown", "Depths",
                        translate("StepDown", "Incremental Step Down of Tool"))
        obj.addProperty("App::PropertyBool", "UseStartDepth", "Depths",
                        translate("Use Start Depth", "Set to True to manually specify a start depth"))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depths",
                        translate("Start Depth", "Starting Depth of Tool - first cut depth in Z"))
        obj.addProperty("App::PropertyBool", "UseFinalDepth", "Depths",
                        translate("Use Final Depth", "Set to True to manually specify a final depth"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depths",
                        translate("Final Depth", "Final Depth of Tool - lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "ThroughDepth", "Depths",
                        translate("Through Depth", "Add this amount of additional cutting depth "
                                  "to open-ended holes. Only used if UseFinalDepth is False"))

        # The current tool number, read-only
        # this is apparently used internally, to keep track of tool chagnes
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool",
                        translate("PathProfile", "The current tool in use"))
        obj.ToolNumber = (0, 0, 1000, 1)
        obj.setEditorMode('ToolNumber', 1)  # make this read only

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def sort_jobs(self, jobs):
        """ sort holes by the nearest neighbor method """
        from Queue import PriorityQueue

        def sqdist(a, b):
            """ square Euclidean distance """
            return (a['xc'] - b['xc']) ** 2 + (a['yc'] - b['yc']) ** 2

        def find_closest(job_list, job, dist):
            q = PriorityQueue()

            for j in job_list:
                q.put((dist(j, job) + job['xc'], j))

            prio, result = q.get()
            return result

        out = []
        zero = {'xc': 0, 'yc': 0}

        out.append(find_closest(jobs, zero, sqdist))
        jobs.remove(out[-1])

        while jobs:
            closest = find_closest(jobs, out[-1], sqdist)
            out.append(closest)
            jobs.remove(closest)

        return out

    def execute(self, obj):
        from Part import Circle, Cylinder, Plane
        from PathScripts import PathUtils
        from math import sqrt

        output = '(helix cut operation'
        if obj.Comment:
            output += ', ' + str(obj.Comment) + ')\n'
        else:
            output += ')\n'

        if obj.Features:
            if not obj.Active:
                obj.Path = Path.Path("(helix cut operation inactive)")
                if obj.ViewObject:
                    obj.ViewObject.Visibility = False
                return

            if not obj.ToolController:
                obj.ToolController = PathUtils.findToolController(obj)

            toolLoad = obj.ToolController

            if toolLoad is None or toolLoad.ToolNumber == 0:
                FreeCAD.Console.PrintError("PathHelix: No tool selected for helix cut operation, insert a tool change operation first\n")
                obj.Path = Path.Path("(ERROR: no tool selected for helix cut operation)")
                return

            tool = toolLoad.Proxy.getTool(toolLoad)

            zsafe = max(baseobj.Shape.BoundBox.ZMax for baseobj, features in obj.Features) + obj.Clearance.Value
            output += "G0 Z" + fmt(zsafe)

            drill_jobs = []

            for base, features in obj.Features:
                for center, by_radius in features_by_centers(base, features).items():
                    radii = sorted(by_radius.keys(), reverse=True)
                    cylinders = map(lambda radius: getattr(base.Shape, by_radius[radius]), radii)
                    zsafe = max(cyl.BoundBox.ZMax for cyl in cylinders) + obj.Clearance.Value
                    cur_z = cylinders[0].BoundBox.ZMax
                    jobs = []

                    for cylinder in cylinders:
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
                            # This is a closed hole if the face connected to
                            # the current cylinder at next_z has the cylinder's
                            # edge as its OuterWire
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

                            xc, yc, _ = cylinder.Surface.Center
                            jobs.append(dict(xc=xc, yc=yc,
                                             zmin=next_z, zmax=cur_z, zsafe=zsafe,
                                             r_out=r, r_in=0.0, closed=closed))

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
                            FreeCAD.Console.PrintError("PathHelix: Encountered cylinder with zero height\n")
                            break

                        cur_z = next_z

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

            drill_jobs = self.sort_jobs(drill_jobs)

            for job in drill_jobs:
                output += helix_cut((job["xc"], job["yc"]), job["r_out"], job["r_in"], obj.DeltaR.Value,
                                    job["zmax"], job["zmin"], obj.StepDown.Value,
                                    job["zsafe"], tool.Diameter,
                                    toolLoad.VertFeed.Value, toolLoad.HorizFeed.Value,
                                    obj.Direction, obj.StartSide)
                output += '\n'

        obj.Path = Path.Path(output)
        if obj.ViewObject:
            obj.ViewObject.Visibility = True


class ViewProviderPathHelix(object):
    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
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
        return {'Pixmap':  'Path-Helix',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathHelix", "PathHelix"),
                'ToolTip':  QtCore.QT_TRANSLATE_NOOP("PathHelix", "Creates a helix cut from selected circles")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        import FreeCADGui
        import Path
        from PathScripts import PathUtils

        FreeCAD.ActiveDocument.openTransaction(translate("PathHelix", "Create a helix cut"))
        FreeCADGui.addModule("PathScripts.PathHelix")

        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "PathHelix")
        ObjectPathHelix(obj)
        ViewProviderPathHelix(obj.ViewObject)

        obj.Features = cylinders_in_selection()
        obj.DeltaR = 1.0

        if not obj.ToolController:
            obj.ToolController = PathUtils.findToolController(obj)

        toolLoad = obj.ToolController

        if toolLoad is not None:
            obj.ToolNumber = toolLoad.ToolNumber
            tool = toolLoad.Proxy.getTool(toolLoad)
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

        PathUtils.addToJob(obj)

        obj.ViewObject.startEditing()

        FreeCAD.ActiveDocument.recompute()


def print_exceptions(func):
    from functools import wraps
    import traceback
    import sys

    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except:
            ex_type, ex, tb = sys.exc_info()
            FreeCAD.Console.PrintError("".join(traceback.format_exception(ex_type, ex, tb)) + "\n")
            raise

    return wrapper


def print_all_exceptions(cls):
    for entry in dir(cls):
        obj = getattr(cls, entry)
        if not entry.startswith("__") and hasattr(obj, "__call__"):
            setattr(cls, entry, print_exceptions(obj))
    return cls


@print_all_exceptions
class TaskPanel(object):

    def __init__(self, obj):
        from Units import Quantity
        from PathScripts import PathUtils

        self.obj = obj
        self.previous_value = {}
        self.form = QtGui.QToolBox()

        ui = FreeCADGui.UiLoader()

        grayed_out = "background-color: #d0d0d0;"

        def nextToolBoxItem(label, iconFile):
            widget = QtGui.QWidget()
            layout = QtGui.QGridLayout()
            widget.setLayout(layout)
            icon = QtGui.QIcon(iconFile)
            self.form.addItem(widget, icon, label)
            return layout

        def addFiller():
            row = layout.rowCount()
            widget = QtGui.QWidget()
            layout.addWidget(widget, row, 0, 1, 2)
            layout.setRowStretch(row, 1)

        layout = nextToolBoxItem("Geometry", ":/icons/PartDesign_InternalExternalGear.svg")

        def addWidget(widget):
            row = layout.rowCount()
            layout.addWidget(widget, row, 0, 1, 2)

        def addWidgets(widget1, widget2):
            row = layout.rowCount()
            layout.addWidget(widget1, row, 0)
            layout.addWidget(widget2, row, 1)

        def addQuantity(property, labelstring, activator=None, max=None):
            self.previous_value[property] = getattr(self.obj, property)
            widget = ui.createWidget("Gui::InputField")

            if activator:
                self.previous_value[activator] = getattr(self.obj, activator)
                currently_active = getattr(self.obj, activator)
                label = QtGui.QCheckBox(labelstring)

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
                label = QtGui.QLabel(labelstring)
                label.setToolTip(self.obj.getDocumentationOfProperty(property))

            quantity = getattr(self.obj, property)
            widget.setText(quantity.UserString)
            widget.setToolTip(self.obj.getDocumentationOfProperty(property))

            if max:
                # cannot use widget.setMaximum() as apparently ui.createWidget()
                # returns the object up-casted to QWidget.
                widget.setProperty("maximum", max)

            def change(quantity):
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

        self.featureTree = QtGui.QTreeWidget()
        self.featureTree.setMinimumHeight(200)
        self.featureTree.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        # self.featureTree.setDragDropMode(QtGui.QAbstractItemView.DragDrop)
        # self.featureTree.setDefaultDropAction(QtCore.Qt.MoveAction)
        self.fillFeatureTree()
        sm = self.featureTree.selectionModel()
        sm.selectionChanged.connect(self.selectFeatures)
        addWidget(self.featureTree)
        self.featureTree.expandAll()

        self.addButton = QtGui.QPushButton("Add holes")
        self.addButton.clicked.connect(self.addCylinders)

        self.delButton = QtGui.QPushButton("Delete")
        self.delButton.clicked.connect(self.delCylinders)

        addWidgets(self.addButton, self.delButton)

        # End of "Features" section

        layout = nextToolBoxItem("Drill parameters", ":/icons/Path-OperationB.svg")
        addCheckBox("Active", "Operation is active")

        toolLoad = PathUtils.findToolController(obj)
        tool = toolLoad and toolLoad.Proxy.getTool(toolLoad)

        if not tool:
            drmax = None
        else:
            drmax = tool.Diameter

        addQuantity("DeltaR", "Step in Radius", max=drmax)
        addQuantity("StepDown", "Step in Z")
        addEnumeration("Direction", "Cut direction",
                       [("Clockwise", "CW"), ("Counter-Clockwise", "CCW")])
        addEnumeration("StartSide", "Start Side",
                       [("Start from inside", "inside"), ("Start from outside", "outside")])

        # End of "Drill parameters" section
        addFiller()

        layout = nextToolBoxItem("Cutting Depths", ":/icons/Path-Depths.svg")
        addQuantity("Clearance", "Clearance Distance")
        addQuantity("StartDepth", "Absolute start height", "UseStartDepth")

        fdcheckbox, fdinput = addQuantity("FinalDepth", "Absolute final height", "UseFinalDepth")
        tdlabel, tdinput = addQuantity("ThroughDepth", "Extra drill depth\nfor open holes")

        # End of "Cutting Depths" section
        addFiller()

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

    def addCylinders(self):
        features_per_base = {}
        for base, features in self.obj.Features:
            features_per_base[base] = list(set(features))

        for base, features in cylinders_in_selection():
            for feature in features:
                if base in features_per_base:
                    if feature not in features_per_base[base]:
                        features_per_base[base].append(feature)
                else:
                    features_per_base[base] = [feature]

        self.obj.Features = list(features_per_base.items())
        self.featureTree.clear()
        self.fillFeatureTree()
        self.featureTree.expandAll()
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def delCylinders(self):
        del_features = []

        def delete_feature(item, base=None):
            kind, feature = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "feature")

            if base is None:
                base_item = item.parent().parent()
                _, base = base_item.data(0, QtCore.Qt.UserRole)

            del_features.append((base, feature))
            item.parent().takeChild(item.parent().indexOfChild(item))

        def delete_hole(item, base=None):
            kind, center = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "hole")

            if base is None:
                base_item = item.parent()
                _, base = base_item.data(0, QtCore.Qt.UserRole)

            for i in reversed(range(item.childCount())):
                delete_feature(item.child(i), base=base)
            item.parent().takeChild(item.parent().indexOfChild(item))

        def delete_base(item):
            kind, base = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "base")
            for i in reversed(range(item.childCount())):
                delete_hole(item.child(i), base=base)
            self.featureTree.takeTopLevelItem(self.featureTree.indexOfTopLevelItem(item))

        for item in self.featureTree.selectedItems():
            kind, info = item.data(0, QtCore.Qt.UserRole)
            if kind == "base":
                delete_base(item)
            elif kind == "hole":
                parent = item.parent()
                delete_hole(item)
                if parent.childCount() == 0:
                    self.featureTree.takeTopLevelItem(self.featureTree.indexOfTopLevelItem(parent))
            elif kind == "feature":
                parent = item.parent()
                delete_feature(item)
                if parent.childCount() == 0:
                    parent.parent().takeChild(parent.parent().indexOfChild(parent))
            else:
                raise Exception("No such item kind: {0}".format(kind))

        for base, features in cylinders_in_selection():
            for feature in features:
                del_features.append((base, feature))

        new_features = []
        for obj, features in self.obj.Features:
            for feature in features:
                if (obj, feature) not in del_features:
                    new_features.append((obj, feature))

        self.obj.Features = new_features
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def fillFeatureTree(self):
        for base, features in self.obj.Features:
            base_item = QtGui.QTreeWidgetItem()
            base_item.setText(0, base.Name)
            base_item.setData(0, QtCore.Qt.UserRole, ("base", base))
            self.featureTree.addTopLevelItem(base_item)
            for center, by_radius in features_by_centers(base, features).items():
                hole_item = QtGui.QTreeWidgetItem()
                hole_item.setText(0, "Hole at ({0[0]:.2f}, {0[1]:.2f})".format(center))
                hole_item.setData(0, QtCore.Qt.UserRole, ("hole", center))
                base_item.addChild(hole_item)
                for radius in sorted(by_radius.keys(), reverse=True):
                    feature = by_radius[radius]
                    cylinder = getattr(base.Shape, feature)
                    cyl_item = QtGui.QTreeWidgetItem()
                    cyl_item.setText(0, "Diameter {0:.2f}, {1}".format(
                                           2 * cylinder.Surface.Radius, feature))
                    cyl_item.setData(0, QtCore.Qt.UserRole, ("feature", feature))
                    hole_item.addChild(cyl_item)

    def selectFeatures(self, selected, deselected):
        FreeCADGui.Selection.clearSelection()

        def select_feature(item, base=None):
            kind, feature = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "feature")

            if base is None:
                base_item = item.parent().parent()
                _, base = base_item.data(0, QtCore.Qt.UserRole)

            FreeCADGui.Selection.addSelection(base, feature)

        def select_hole(item, base=None):
            kind, center = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "hole")

            if base is None:
                base_item = item.parent()
                _, base = base_item.data(0, QtCore.Qt.UserRole)

            for i in range(item.childCount()):
                select_feature(item.child(i), base=base)

        def select_base(item):
            kind, base = item.data(0, QtCore.Qt.UserRole)
            assert(kind == "base")

            for i in range(item.childCount()):
                select_hole(item.child(i), base=base)

        for item in self.featureTree.selectedItems():
            kind, info = item.data(0, QtCore.Qt.UserRole)

            if kind == "base":
                select_base(item)
            elif kind == "hole":
                select_hole(item)
            elif kind == "feature":
                select_feature(item)

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
    FreeCADGui.addCommand('Path_Helix', CommandPathHelix())
