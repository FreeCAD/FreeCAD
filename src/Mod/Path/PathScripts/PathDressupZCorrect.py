# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathUtils as PathUtils
from bisect import bisect_left

from PySide import QtCore

"""Z Depth Correction Dressup.  This dressup takes a probe file as input and does bilinear interpolation of the Zdepths to correct for a surface which is not parallel to the milling table/bed.  The probe file should conform to the format specified by the linuxcnc G38 probe logging: 9-number coordinate consisting of XYZABCUVW http://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g38
"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

movecommands = ['G1', 'G01', 'G2', 'G02', 'G3', 'G03']
rapidcommands = ['G0', 'G00']
arccommands = ['G2', 'G3', 'G02', 'G03']

class ObjectDressup:
    x_index = 0
    y_index = 0
    values = None
    x_length = 0
    y_length = 0 

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupAxisMap", "The base path to modify"))
        obj.addProperty("App::PropertyFile", "probefile", "Path", QtCore.QT_TRANSLATE_NOOP("Path_DressupZCorrect", "The point file from the surface probing."))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, fp, prop):
        if str(prop) == "probefile":
            self._loadFile(fp.probefile)

    def _bilinearInterpolate(self, x, y):
        # local lookups
        x_index, y_index, values = self.x_index, self.y_index, self.values

        i = bisect_left(x_index, x) - 1
        j = bisect_left(y_index, y) - 1

        # fix x index
        if i == -1:
            x_slice = slice(None, 2)
        elif i == self.x_length - 1:
            x_slice = slice(-2, None)
        else:
            x_slice = slice(i, i + 2)
        # fix y index
        if j == -1:
            j = 0
            y_slice = slice(None, 2)
        elif j == self.y_length - 1:
            j = -2
            y_slice = slice(-2, None)
        else:
            y_slice = slice(j, j + 2)

        x1, x2 = x_index[x_slice]
        y1, y2 = y_index[y_slice]
        z11, z12 = values[j][x_slice]
        z21, z22 = values[j + 1][x_slice]

        return (z11 * (x2 - x) * (y2 - y) +
                z21 * (x - x1) * (y2 - y) +
                z12 * (x2 - x) * (y - y1) +
                z22 * (x - x1) * (y - y1)) / ((x2 - x1) * (y2 - y1))


    def _loadFile(self, filename):
	f1 = open(filename, 'r')

	pointlist = []
	for line in f1.readlines():
            w = line.split()
            xval = round(float(w[0]), 3)
       	    yval = round(float(w[1]), 3)
            zval = round(float(w[2]), 3)
            pointlist.append((xval, yval,zval))
        cols = list(zip(*pointlist))

        xcolpos = list(sorted(set(cols[0])))
        #ycolpos = list(sorted(set(cols[1])))
        zdict = {x:[] for x in xcolpos}

        for (x, y, z) in pointlist:
            zdict[x].append(z)

        self.values = tuple(tuple(x) for x in [zdict[x] for x in sorted(xcolpos)])
        self.x_index = tuple(sorted(set(cols[0])))
        self.y_index = tuple(sorted(set(cols[1])))

        # sanity check
        x_length = len(self.x_index)
        y_length = len(self.y_index)

        if x_length < 2 or y_length < 2:
            raise ValueError("Probe grid must be at least 2x2.")
        if y_length != len(self.values):
            raise ValueError("Probe grid data must have equal number of rows to y_index.")
        if any(x2 - x1 <= 0 for x1, x2 in zip(self.x_index, self.x_index[1:])):
            raise ValueError("x_index must be in strictly ascending order!")
        if any(y2 - y1 <= 0 for y1, y2 in zip(self.y_index, self.y_index[1:])):
            raise ValueError("y_index must be in strictly ascending order!")

        self.x_length = x_length
        self.y_length = y_length


    def execute(self, obj):
        if self.values is None: #No valid probe data.  return unchanged path
            obj.Path = obj.Base.Path
            return

        if obj.Base:
            if obj.Base.isDerivedFrom("Path::Feature"):
                if obj.Base.Path:
                    if obj.Base.Path.Commands:
                        pp = obj.Base.Path.Commands
			# process the path

                        pathlist = pp
                        newcommandlist = []
                        currLocation = {'X':0,'Y':0,'Z':0, 'F': 0}

                        for c in pathlist: #obj.Base.Path.Commands:
                            newparams = dict(c.Parameters)
                            currLocation.update(newparams)
                            remapvar = newparams.pop("Z", None)
                            if remapvar is not None:
                                offset = self._bilinearInterpolate(currLocation['X'], currLocation['Y'])
                                newparams["Z"] = remapvar + offset
                                newcommand = Path.Command(c.Name, newparams)
                                newcommandlist.append(newcommand)
                                currLocation.update(newparams)
                            else:
                                newcommandlist.append(c)

                        path = Path.Path(newcommandlist)
                        obj.Path = path


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
            # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return

    def claimChildren(self):
        return [self.obj.Base]

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        job = PathUtils.findParentJob(arg1.Object)
        job.Proxy.addOperation(arg1.Object.Base)
        arg1.Object.Base = None
        return True

class CommandPathDressup:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_DressupZCorrect", "Z Depth Correction Dress-up"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_DressupZCorrect", "Use Probe Map to correct Z depth")}

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
            FreeCAD.Console.PrintError(translate("Path_Dressup", "Please select one path object\n"))
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("Path_Dressup", "The selected object is not a path\n"))
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Path_Dressup", "Please select a Path object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Path_DressupZCorrect", "Create Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupZCorrect")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "ZCorrectDressup")')
        FreeCADGui.doCommand('PathScripts.PathDressupZCorrect.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupZCorrect.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_DressupZCorrect', CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
