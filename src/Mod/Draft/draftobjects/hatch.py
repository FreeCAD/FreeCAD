#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *
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


"""This module contains FreeCAD commands for the Draft workbench"""

import os
import FreeCAD
from draftutils.translate import translate, QT_TRANSLATE_NOOP




class Draft_Hatch_Object:


    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Placement" in pl:
            obj.addProperty("App::PropertyPlacement","Placement","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The placement of this object"))
        if not "Shape" in pl:
            obj.addProperty("Part::PropertyPartShape","Shape","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The shape of this object"))
        if not "Base" in pl:
            obj.addProperty("App::PropertyLink","Base","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The base object used by this object"))
        if not "File" in pl:
            obj.addProperty("App::PropertyFile","File","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The PAT file used by this object"))
        if not "Pattern" in pl:
            obj.addProperty("App::PropertyString","Pattern","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The pattern name used by this object"))
        if not "Scale" in pl:
            obj.addProperty("App::PropertyFloat","Scale","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The pattern scale used by this object"))
        if not "Rotation" in pl:
            obj.addProperty("App::PropertyAngle","Rotation","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","The pattern rotation used by this object"))
        if not "Translate" in pl:
            obj.addProperty("App::PropertyBool","Translate","Hatch",
                            QT_TRANSLATE_NOOP("App::Property","If set to False, hatch is applied as is to the faces, without translation (this might give wrong results for non-XY faces)"))
            obj.Translate = True
        self.Type = "Hatch"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def __getstate__(self):

        return None

    def __setstate__(self,state):

        return None

    def execute(self,obj):

        import Part
        import TechDraw

        if not obj.Base:
            return
        if not obj.File:
            return
        if not obj.Pattern:
            return
        if not obj.Scale:
            return
        if not obj.Pattern in self.getPatterns(obj.File):
            return
        if not obj.Base.isDerivedFrom("Part::Feature"):
            return
        if not obj.Base.Shape.Faces:
            return

        pla = obj.Placement
        shapes = []
        for face in obj.Base.Shape.Faces:
            face = face.copy()
            if obj.Translate:
                bpoint = face.CenterOfMass
                norm = face.normalAt(0,0)
                fpla = FreeCAD.Placement(bpoint,FreeCAD.Rotation(FreeCAD.Vector(0,0,1),norm))
                face.Placement = face.Placement.multiply(fpla.inverse())
            if obj.Rotation:
                face.rotate(FreeCAD.Vector(),FreeCAD.Vector(0,0,1),obj.Rotation)
            shape = TechDraw.makeGeomHatch(face,obj.Scale,obj.Pattern,obj.File)
            if obj.Rotation:
                shape.rotate(FreeCAD.Vector(),FreeCAD.Vector(0,0,1),-obj.Rotation)
            if obj.Translate:
                shape.Placement = shape.Placement.multiply(fpla)
            shapes.append(shape)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pla

    def getPatterns(self,filename):

        """returns a list of pattern names found in a PAT file"""
        patterns = []
        if os.path.exists(filename):
            with open(filename) as patfile:
                for line in patfile:
                    if line.startswith("*"):
                        patterns.append(line.split(",")[0][1:])
        return patterns
