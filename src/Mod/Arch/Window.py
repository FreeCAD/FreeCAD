#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

import FreeCAD,FreeCADGui,Part,Draft,Component
from draftlibs import fcgeo,fcvec
from FreeCAD import Vector
from PyQt4 import QtCore

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

def makeWindow(baseobj=None,name="Window"):
    '''makeWindow(obj,[name]): creates a window based on the
    given object'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Window(obj)
    _ViewProviderWindow(obj.ViewObject)
    if baseobj: obj.Base = baseobj
    if obj.Base: obj.Base.ViewObject.hide()
    #p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    #c = p.GetUnsigned("WindowColor")
    #r = float((c>>24)&0xFF)/255.0
    #g = float((c>>16)&0xFF)/255.0
    #b = float((c>>8)&0xFF)/255.0
    #obj.ViewObject.ShapeColor = (r,g,b,1.0)
    return obj

class _CommandWindow:
    "the Arch Window command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Window',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Window","Window"),
                'Accel': "W, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Window","Creates a window object from scratch or from a selected object (wire, rectangle or sketch)")}
        
    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        FreeCAD.ActiveDocument.openTransaction("Window")
        if sel:
            for obj in sel:
                makeWindow(obj)
        else:
            rect = Draft.makeRectangle(1,1)
            makeWindow(rect)
        FreeCAD.ActiveDocument.commitTransaction()
       
class _Window(Component.Component):
    "The Window object"
    def __init__(self,obj):
        Component.Component.__init__(self,obj)
        obj.addProperty("App::PropertyFloat","Thickness","Base",
                        "the thickness to be associated with the wire of the Base object")
        #obj.addProperty("App::PropertyLink","Support","Base",
        #                "The support object (wall usually) of this window")
        #obj.addProperty("App::PropertyLength","Jamb","Base",
        #                "The distance between the window and the exterior face of its supporting object")
        #obj.addProperty("Part::PropertyPartShape","Subvolume","Base",
        #                "the volume to be subtracted when inserting this window into its support")
        self.Type = "Window"
        obj.Thickness = .1
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Base","Thickness"]:
            self.createGeometry(obj)

    def createGeometry(self,obj):
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.Wires:
                    basewire = obj.Base.Shape.Wires[0]
                    if obj.Thickness:
                        offwire = basewire.makeOffset(-obj.Thickness)
                        f1 = Part.Face(basewire)
                        f2 = Part.Face(offwire)
                        bf = f1.cut(f2)
                        sh = bf.extrude(Vector(0,0,obj.Thickness))
                        obj.Shape = sh
                        f1.translate(Vector(0,0,-.5))
                        svol = f1.extrude(Vector(0,0,1))
                        self.Subvolume = svol
        if not fcgeo.isNull(pl):
            obj.Placement = pl
            self.Subvolume.Placement = pl

class _ViewProviderWindow(Component.ViewProviderComponent):
    "A View Provider for the Window object"

    def __init__(self,vobj):
        Component.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):          
        return """
            /* XPM */
            static char * Arch_Window_xpm[] = {
            "16 16 5 1",
            " 	c None",
            ".	c #0E0A00",
            "+	c #9B8A61",
            "@	c #C6B287",
            "#	c #FEFFF8",
            "                ",
            " ......         ",
            " ####+......    ",
            " #+. #####..... ",
            " #+.  @#@####++ ",
            " #+.   ##   #++ ",
            " #.....##   #++ ",
            " ####+.#....#+@ ",
            " #+. +###+..++@ ",
            " #+.  @#+@###+@ ",
            " #...  ##   #+@ ",
            " #+....#+   #+@ ",
            "  ####+#....#+@ ",
            "     @####..@+@ ",
            "         @###++ ",
            "             .  "};
            """

FreeCADGui.addCommand('Arch_Window',_CommandWindow())
