# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
'''used to create material stock around a machined part- for visualization '''

import Draft,Part
import FreeCAD, FreeCADGui
from FreeCAD import Vector
from PySide import QtCore, QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class Stock:
    def __init__(self, obj):
        "Make stock"
        obj.addProperty("App::PropertyFloat","Length_Allowance","Stock",translate("Length Allowance","extra allownace from part width")).Length_Allowance = 1.0
        obj.addProperty("App::PropertyFloat","Width_Allowance","Stock",translate("Width Allowance","extra allownace from part width")).Width_Allowance = 1.0
        obj.addProperty("App::PropertyFloat","Height_Allowance","Stock",translate("Height Allowance","extra allownace from part width")).Height_Allowance = 1.0
        obj.addProperty("App::PropertyLink","Base","Base",
                        "The base object this represents")
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self, obj):
        self.Xmin = obj.Base.Shape.BoundBox.XMin
        self.Xmax = obj.Base.Shape.BoundBox.XMax

        self.Ymin = obj.Base.Shape.BoundBox.YMin
        self.Ymax = obj.Base.Shape.BoundBox.YMax

        self.Zmin = obj.Base.Shape.BoundBox.ZMin
        self.Zmax = obj.Base.Shape.BoundBox.ZMax

        self.length = self.Xmax -self.Xmin+obj.Length_Allowance*2.0
        self.width  = self.Ymax - self.Ymin+obj.Width_Allowance*2.0
        self.height = self.Zmax - self.Zmin+obj.Height_Allowance*2.0
        self.pnt = Vector(self.Xmin-obj.Length_Allowance , self.Ymin-obj.Width_Allowance, self.Zmin-obj.Height_Allowance)

        obj.Shape = Part.makeBox(self.length,self.width,self.height,self.pnt)

class _ViewProviderStock:

    def __init__(self,obj): #mandatory
#        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Stock.svg"

    def attach(self, vobj): #optional
        self.Object = vobj.Object



class CommandPathStock:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Stock',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathStock","Stock"),
                'Accel': "P, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathStock","Creates a 3D object to represent raw stock to mill the part out of")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("PathStock","Creates a 3D object to represent raw stock to mill the part out of"))
        FreeCADGui.addModule("PathScripts.PathStock")
        snippet = '''
import FreeCADGui
if len(FreeCADGui.Selection.getSelection())>0:
    sel=FreeCADGui.Selection.getSelection()
    o = sel[0]
    if "Shape" in o.PropertiesList:
        obj =FreeCAD.ActiveDocument.addObject('Part::FeaturePython',sel[0].Name+('_Stock'))
        PathScripts.PathStock.Stock(obj)
        PathScripts.PathStock._ViewProviderStock(obj.ViewObject)
        PathScripts.PathUtils.addToProject(obj)
        baseobj = sel[0]
        obj.Base = baseobj
        FreeCADGui.ActiveDocument.getObject(sel[0].Name+("_Stock")).ShapeColor = (0.3333,0.6667,1.0000)
        FreeCADGui.ActiveDocument.getObject(sel[0].Name+("_Stock")).Transparency = 75
        FreeCAD.ActiveDocument.recompute()
    else:
        FreeCAD.Console.PrintMessage("Select a Solid object and try again.\\n")
else:
    FreeCAD.Console.PrintMessage("Select the object you want to show stock for and try again.\\n")
        '''
        FreeCADGui.doCommand(snippet)

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Stock',CommandPathStock())

FreeCAD.Console.PrintLog("Loading PathStock... done\n")

