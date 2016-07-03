# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic  <shopinthewoods@gmail.com>              *
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

import FreeCAD
import Path
from PathScripts import PathUtils

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt, txt):
        return txt

__title__ = "Path Surface Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path surface object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectSurface:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", "The base geometry of this toolpath")
        obj.addProperty("App::PropertyBool", "Active", "Path", "Make False, to prevent operation from generating code")
        obj.addProperty("App::PropertyString", "Comment", "Path", "An optional comment for this profile")
        obj.addProperty("App::PropertyString", "UserLabel", "Path", "User Assigned Label")

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm", "The library to use to generate the path")
        obj.Algorithm = ['OCL Dropcutter', 'OCL Waterline']

        # Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool", "The tool number in use")
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", "The description of the tool ")
        obj.setEditorMode('ToolDescription', 1)  # make this read onlyt

        # Surface Properties
        obj.addProperty("App::PropertyFloatConstraint", "SampleInterval", "Surface", "The Sample Interval.  Small values cause long wait")
        obj.SampleInterval = (0, 0, 1, 0)

        # Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", "The height needed to clear clamps and obstructions")
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", "Rapid Safety Height between locations.")
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", "Incremental Step Down of Tool")
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", "Starting Depth of Tool- first cut depth in Z")
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", "Final Depth of Tool- lowest value in Z")
        obj.addProperty("App::PropertyDistance", "FinishDepth", "Depth", "Maximum material removed on final pass.")

        obj.Proxy = self

    def addsurfacebase(self, obj, ss, sub=""):
        baselist = obj.Base
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                subobj = ss.Shape.getElement(sub)
                fbb = subobj.BoundBox  # feature boundbox
                obj.StartDepth = bb.ZMax
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight = bb.ZMax + 3.0

                if fbb.ZMax < bb.ZMax:
                    obj.FinalDepth = fbb.ZMax
                else:
                    obj.FinalDepth = bb.ZMin
            except:
                obj.StartDepth = 5.0
                obj.ClearanceHeight = 10.0
                obj.SafeHeight = 8.0

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning(
                "this object already in the list" + "\n")
        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

    def _waterline(self, obj, s, bb):
        import ocl
        from PathScripts.PathUtils import depth_params, fmt
        import time

        def drawLoops(loops):
            nloop = 0
            waterlinestring = ""
            waterlinestring += "(waterline begin)"
            for loop in loops:
                p = loop[0]
                loopstring = "(loop begin)" + "\n"
                loopstring += "G0 Z" + str(obj.SafeHeight.Value) + "\n"
                loopstring += "G0 X" + \
                    str(fmt(p.x)) + " Y" + str(fmt(p.y)) + "\n"
                loopstring += "G1 Z" + str(fmt(p.z)) + "\n"
                for p in loop[1:]:
                    loopstring += "G1 X" + \
                        str(fmt(p.x)) + " Y" + str(fmt(p.y)) + \
                        " Z" + str(fmt(p.z)) + "\n"
                    zheight = p.z
                p = loop[0]
                loopstring += "G1 X" + \
                    str(fmt(p.x)) + " Y" + str(fmt(p.y)) + \
                    " Z" + str(fmt(zheight)) + "\n"
                loopstring += "(loop end)" + "\n"
                print "    loop ", nloop, " with ", len(loop), " points"
                nloop = nloop + 1
                waterlinestring += loopstring
            waterlinestring += "(waterline end)" + "\n"
            return waterlinestring

        depthparams = depth_params(obj.ClearanceHeight.Value, obj.SafeHeight.Value,
                                   obj.StartDepth.Value, obj.StepDown, obj.FinishDepth.Value, obj.FinalDepth.Value)
        # stlfile = "../../stl/gnu_tux_mod.stl"
        # surface = STLSurfaceSource(stlfile)
        surface = s

        t_before = time.time()
        zheights = depthparams.get_depths()
        wl = ocl.Waterline()
        # wl = ocl.AdaptiveWaterline() # this is slower, ca 60 seconds on i7
        # CPU
        wl.setSTL(surface)
        diam = 0.5
        length = 10.0
        # any ocl MillingCutter class should work here
        cutter = ocl.BallCutter(diam, length)
        wl.setCutter(cutter)
        # this should be smaller than the smallest details in the STL file
        wl.setSampling(obj.SampleInterval)
        # AdaptiveWaterline() also has settings for minimum sampling interval
        # (see c++ code)
        all_loops = []
        for zh in zheights:
            print "calculating Waterline at z= ", zh
            wl.reset()
            wl.setZ(zh)  # height for this waterline
            wl.run()
            all_loops.append(wl.getLoops())
        t_after = time.time()
        calctime = t_after - t_before
        n = 0
        output = ""
        for loops in all_loops:  # at each z-height, we may get many loops
            print "  %d/%d:" % (n, len(all_loops))
            output += drawLoops(loops)
            n = n + 1
        print "(" + str(calctime) + ")"
        return output

    def _dropcutter(self, obj, s, bb):
        import ocl
        import time

        cutter = ocl.CylCutter(self.radius * 2, 5)
        pdc = ocl.PathDropCutter()   # create a pdc
        pdc.setSTL(s)
        pdc.setCutter(cutter)
        pdc.minimumZ = 0.25
        pdc.setSampling(obj.SampleInterval)

        # some parameters for this "zigzig" pattern
        xmin = bb.XMin - cutter.getDiameter()
        xmax = bb.XMax + cutter.getDiameter()
        ymin = bb.YMin - cutter.getDiameter()
        ymax = bb.YMax + cutter.getDiameter()

        # number of lines in the y-direction
        Ny = int(bb.YLength / cutter.getDiameter())
        dy = float(ymax - ymin) / Ny  # the y step-over

        path = ocl.Path()                   # create an empty path object

        # add Line objects to the path in this loop
        for n in xrange(0, Ny):
            y = ymin + n * dy
            p1 = ocl.Point(xmin, y, 0)   # start-point of line
            p2 = ocl.Point(xmax, y, 0)   # end-point of line
            if (n % 2 == 0):  # even
                l = ocl.Line(p1, p2)     # line-object
            else:  # odd
                l = ocl.Line(p2, p1)     # line-object

            path.append(l)        # add the line to the path

        pdc.setPath(path)

        # run drop-cutter on the path
        t_before = time.time()
        pdc.run()
        t_after = time.time()
        print "calculation took ", t_after - t_before, " s"

        # retrieve the points
        clp = pdc.getCLPoints()
        print "points received: " + str(len(clp))

        # generate the path commands
        output = ""
        output += "G0 Z" + str(obj.ClearanceHeight.Value) + "\n"
        output += "G0 X" + str(clp[0].x) + " Y" + str(clp[0].y) + "\n"
        output += "G1 Z" + str(clp[0].z) + " F" + str(self.vertFeed) + "\n"

        for c in clp:
            output += "G1 X" + str(c.x) + " Y" + \
                str(c.y) + " Z" + str(c.z) + "\n"

        return output

    def execute(self, obj):
        import MeshPart
        FreeCAD.Console.PrintWarning(
            translate("PathSurface", "Hold on.  This might take a minute.\n"))
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'

        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.vertFeed = 100
            self.horizFeed = 100
            self.radius = 0.25
            obj.ToolNumber = 0
            obj.ToolDescription = "UNDEFINED"
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            self.radius = tool.Diameter/2
            obj.ToolNumber = toolLoad.ToolNumber
            obj.ToolDescription = toolLoad.Name

        if obj.UserLabel == "":
            obj.Label = obj.Name + " :" + obj.ToolDescription
        else:
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

        output += "(" + obj.Label + ")"
        output += "(Compensated Tool Path: " + str(self.radius) + ")"

        if obj.Base:
            for b in obj.Base:

                if obj.Algorithm in ['OCL Dropcutter', 'OCL Waterline']:
                    try:
                        import ocl
                    except:
                        FreeCAD.Console.PrintError(translate(
                            "PathSurface", "This operation requires OpenCamLib to be installed.\n"))
                        return

                mesh = b[0]
                if mesh.TypeId.startswith('Mesh'):
                    mesh = mesh.Mesh
                    bb = mesh.BoundBox
                else:
                    bb = mesh.Shape.BoundBox
                    mesh = MeshPart.meshFromShape(mesh.Shape, MaxLength=2)

                s = ocl.STLSurf()
                for f in mesh.Facets:
                    p = f.Points[0]
                    q = f.Points[1]
                    r = f.Points[2]
                    t = ocl.Triangle(ocl.Point(p[0], p[1], p[2]), ocl.Point(
                        q[0], q[1], q[2]), ocl.Point(r[0], r[1], r[2]))
                    s.addTriangle(t)

                if obj.Algorithm == 'OCL Dropcutter':
                    output = self._dropcutter(obj, s, bb)
                elif obj.Algorithm == 'OCL Waterline':
                    output = self._waterline(obj, s, bb)

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False

class ViewProviderSurface:

    def __init__(self, obj):  # mandatory
        #        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/Path-Surfacing.svg"

    def onChanged(self, obj, prop):  # optional
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self, obj, prop):  # optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def unsetEdit(self, vobj, mode):  # optional
        # this is executed when the user cancels or terminates edit mode
        pass


class CommandPathSurfacing:

    def GetResources(self):
        return {'Pixmap': 'Path-3DSurface',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Surface", "Surfacing"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Surface", "Creates a Path Surfacing object")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):

        ztop = 10
        zbottom = 0

        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Surfacing", "Create Surface"))
        FreeCADGui.addModule("PathScripts.PathSurface")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Surface")')
        FreeCADGui.doCommand('PathScripts.PathSurface.ObjectSurface(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand(
            'PathScripts.PathSurface.ViewProviderSurface(obj.ViewObject)')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StepDown = ' + str((ztop - zbottom) / 8))
        FreeCADGui.doCommand('obj.SampleInterval = 0.4')

        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:

    def __init__(self):
        # self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/SurfaceEdit.ui")
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/SurfaceEdit.ui")
        FreeCAD.Console.PrintWarning("Surface calculations can be slow.  Don't Panic.\n")

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = self.form.startDepth.text()
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = self.form.finalDepth.text()
            if hasattr(self.obj, "FinishDepth"):
                self.obj.FinishDepth = self.form.finishDepth.text()
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = self.form.stepDown.value()

            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = self.form.safeHeight.text()
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = self.form.clearanceHeight.text()
            if hasattr(self.obj, "Algorithm"):
                self.obj.Algorithm = str(
                    self.form.algorithmSelect.currentText())

        self.obj.Proxy.execute(self.obj)

    def setFields(self):
        self.form.startDepth.setText(str(self.obj.StartDepth.Value))
        self.form.finalDepth.setText(str(self.obj.FinalDepth.Value))
        self.form.finishDepth.setText(str(self.obj.FinishDepth.Value))
        self.form.stepDown.setValue(self.obj.StepDown)

        self.form.safeHeight.setText(str(self.obj.SafeHeight.Value))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight.Value))

        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

        index = self.form.algorithmSelect.findText(
                self.obj.Algorithm, QtCore.Qt.MatchFixedString)
        if index >= 0:
            self.form.algorithmSelect.setCurrentIndex(index)

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate(
                "PathSurface", "Please select a single solid object from the project tree\n"))
            return

        if not len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate(
                "PathSurface", "Please select a single solid object from the project tree\n"))
            return

        sel = selection[0].Object
        # get type of object
        if sel.TypeId.startswith('Mesh'):
            # it is a mesh already
            print 'was already mesh'

        elif sel.TypeId.startswith('Part') and \
                (sel.Shape.BoundBox.XLength > 0) and \
                (sel.Shape.BoundBox.YLength > 0) and \
                (sel.Shape.BoundBox.ZLength > 0):
            print 'this is a solid Part object'

        else:
            FreeCAD.Console.PrintError(
                translate("PathSurface", "Cannot work with this object\n"))
            return

        self.obj.Proxy.addsurfacebase(self.obj, sel)

        self.setFields()  # defaults may have changed.  Reload.
        self.form.baseList.clear()
        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text():
                    newlist.append(i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            o = FreeCAD.ActiveDocument.getObject(i.text())
            FreeCADGui.Selection.addSelection(o)
        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            obj = FreeCAD.ActiveDocument.getObject(s)
            newlist.append(obj)
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):

        # Connect Signals and Slots

        #Base Geometry
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)

        # Depths
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.finishDepth.editingFinished.connect(self.getFields)
        self.form.stepDown.editingFinished.connect(self.getFields)

        # Heights
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        # Operation
        self.form.algorithmSelect.currentIndexChanged.connect(self.getFields)

        sel = FreeCADGui.Selection.getSelectionEx()
        self.setFields()

        if len(sel) != 0:
            self.addBase()


class SelObserver:

    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.surfaceselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):               # Selection object
        FreeCADGui.doCommand(
            'Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Surfacing', CommandPathSurfacing())

FreeCAD.Console.PrintLog("Loading PathSurfacing... done\n")
