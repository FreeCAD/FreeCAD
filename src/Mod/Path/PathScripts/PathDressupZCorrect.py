# -*- coding: utf-8 -*
# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
# *   Bilinear interpolation code modified heavily from the interpolation   *
# *   library https://github.com/pmav99/interpolation                      *
# *   Copyright (c) 2013 by Panagiotis Mavrogiorgos                         *
# *                                                                         *
# ***************************************************************************
import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
from PathScripts.PathGeom import CmdMoveArc, CmdMoveStraight

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

"""Z Depth Correction Dressup.  This dressup takes a probe file as input and does bilinear interpolation of the Zdepths to correct for a surface which is not parallel to the milling table/bed.  The probe file should conform to the format specified by the linuxcnc G38 probe logging: 9-number coordinate consisting of XYZABCUVW http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g38
"""

LOGLEVEL = False

LOG_MODULE = PathLog.thisModule()

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyFile",
            "probefile",
            "ProbeData",
            QT_TRANSLATE_NOOP(
                "App::Property", "The point file from the surface probing."
            ),
        )
        obj.Proxy = self
        obj.addProperty("Part::PropertyPartShape", "interpSurface", "Path")
        obj.addProperty(
            "App::PropertyDistance",
            "ArcInterpolate",
            "Interpolate",
            QT_TRANSLATE_NOOP(
                "App::Property", "Deflection distance for arc interpolation"
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "SegInterpolate",
            "Interpolate",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "break segments into smaller segments of this length.",
            ),
        )
        obj.ArcInterpolate = 0.1
        obj.SegInterpolate = 1.0

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, fp, prop):
        if str(prop) == "probefile":
            self._loadFile(fp, fp.probefile)

    def _bilinearInterpolate(self, surface, x, y):

        p1 = FreeCAD.Vector(x, y, 100.0)
        p2 = FreeCAD.Vector(x, y, -100.0)

        vertical_line = Part.Line(p1, p2)
        points, curves = vertical_line.intersectCS(surface)
        return points[0].Z

    def _loadFile(self, obj, filename):
        if filename == "":
            return

        f1 = open(filename, "r")

        try:
            pointlist = []
            for line in f1.readlines():
                if line == "\n":
                    continue
                w = line.split()
                xval = round(float(w[0]), 2)
                yval = round(float(w[1]), 2)
                zval = round(float(w[2]), 2)

                pointlist.append([xval, yval, zval])
            PathLog.debug(pointlist)

            cols = list(zip(*pointlist))
            PathLog.debug("cols: {}".format(cols))
            yindex = list(sorted(set(cols[1])))
            PathLog.debug("yindex: {}".format(yindex))

            array = []
            for y in yindex:
                points = sorted([p for p in pointlist if p[1] == y])
                inner = []
                for p in points:
                    inner.append(FreeCAD.Vector(p[0], p[1], p[2]))
                array.append(inner)

            intSurf = Part.BSplineSurface()
            intSurf.interpolate(array)

            obj.interpSurface = intSurf.toShape()
        except Exception:
            raise ValueError("File does not contain appropriate point data")

    def execute(self, obj):

        sampleD = obj.SegInterpolate.Value
        curveD = obj.ArcInterpolate.Value

        if obj.interpSurface.isNull():  # No valid probe data.  return unchanged path
            obj.Path = obj.Base.Path
            return

        surface = obj.interpSurface.toNurbs().Faces[0].Surface

        if obj.Base:
            if obj.Base.isDerivedFrom("Path::Feature"):
                if obj.Base.Path:
                    if obj.Base.Path.Commands:
                        pathlist = obj.Base.Path.Commands

                        newcommandlist = []
                        currLocation = {"X": 0, "Y": 0, "Z": 0, "F": 0}

                        for c in pathlist:
                            PathLog.debug(c)
                            PathLog.debug("     curLoc:{}".format(currLocation))
                            newparams = dict(c.Parameters)
                            zval = newparams.get("Z", currLocation["Z"])
                            if c.Name in CmdMoveStraight + CmdMoveArc:
                                curVec = FreeCAD.Vector(
                                    currLocation["X"],
                                    currLocation["Y"],
                                    currLocation["Z"],
                                )
                                arcwire = PathGeom.edgeForCmd(c, curVec)
                                if arcwire is None:
                                    continue
                                if c.Name in CmdMoveArc:
                                    pointlist = arcwire.discretize(Deflection=curveD)
                                else:
                                    disc_number = int(arcwire.Length / sampleD)
                                    if disc_number > 1:
                                        pointlist = arcwire.discretize(
                                            Number=int(arcwire.Length / sampleD)
                                        )
                                    else:
                                        pointlist = [v.Point for v in arcwire.Vertexes]
                                for point in pointlist:
                                    offset = self._bilinearInterpolate(
                                        surface, point.x, point.y
                                    )
                                    newcommand = Path.Command(
                                        "G1",
                                        {
                                            "X": point.x,
                                            "Y": point.y,
                                            "Z": point.z + offset,
                                        },
                                    )
                                    newcommandlist.append(newcommand)
                                    currLocation.update(newcommand.Parameters)
                                    currLocation["Z"] = zval

                            else:
                                # Non Feed Command
                                newcommandlist.append(c)
                                currLocation.update(c.Parameters)
                        path = Path.Path(newcommandlist)
                        obj.Path = path


class TaskPanel:
    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ZCorrectEdit.ui")
        FreeCAD.ActiveDocument.openTransaction("Edit Z Correction Dress-up")
        self.interpshape = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "InterpolationSurface"
        )
        self.interpshape.Shape = obj.interpSurface
        self.interpshape.ViewObject.Transparency = 60
        self.interpshape.ViewObject.ShapeColor = (1.00000, 1.00000, 0.01961)
        self.interpshape.ViewObject.Selectable = False
        stock = PathUtils.findParentJob(obj).Stock
        self.interpshape.Placement.Base.z = stock.Shape.BoundBox.ZMax

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.removeObject(self.interpshape.Name)
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.obj.Proxy.execute(self.obj)

    def updateUI(self):

        if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith("Shape"):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)
            print("object name %s" % self.obj.Name)
            if hasattr(self.obj.Proxy, "shapes"):
                PathLog.info("showing shapes attribute")
                for shapes in self.obj.Proxy.shapes.itervalues():
                    for shape in shapes:
                        Part.show(shape)
            else:
                PathLog.info("no shapes attribute found")

    def updateModel(self):
        self.getFields()
        self.updateUI()
        FreeCAD.ActiveDocument.recompute()

    def setFields(self):
        self.form.ProbePointFileName.setText(self.obj.probefile)

        self.updateUI()

    def open(self):
        pass

    def setupUi(self):
        self.setFields()
        # now that the form is filled, setup the signal handlers
        self.form.ProbePointFileName.editingFinished.connect(self.updateModel)
        self.form.SetProbePointFileName.clicked.connect(self.SetProbePointFileName)

    def SetProbePointFileName(self):
        filename = QtGui.QFileDialog.getOpenFileName(
            self.form,
            translate("Path_Probe", "Select Probe Point File"),
            None,
            translate("Path_Probe", "All Files (*.*)"),
        )
        if filename and filename[0]:
            self.obj.probefile = str(filename[0])
            self.setFields()


class ViewProviderDressup:
    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
        return

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        job = PathUtils.findParentJob(arg1.Object)
        job.Proxy.addOperation(arg1.Object.Base)
        arg1.Object.Base = None
        return True


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "Path_Dressup",
            "MenuText": QT_TRANSLATE_NOOP(
                "Path_DressupZCorrect", "Z Depth Correction Dress-up"
            ),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_DressupZCorrect", "Use Probe Map to correct Z depth"
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("Path_Dressup", "Please select one path object\n")
            )
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("Path_Dressup", "The selected object is not a path\n")
            )
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(
                translate("Path_Dressup", "Please select a Path object")
            )
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("PathScripts.PathDressupZCorrect")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "ZCorrectDressup")'
        )
        FreeCADGui.doCommand("PathScripts.PathDressupZCorrect.ObjectDressup(obj)")
        FreeCADGui.doCommand("obj.Base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand(
            "PathScripts.PathDressupZCorrect.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCADGui.doCommand(
            "Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False"
        )
        FreeCADGui.doCommand("obj.ViewObject.Document.setEdit(obj.ViewObject, 0)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_DressupZCorrect", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
