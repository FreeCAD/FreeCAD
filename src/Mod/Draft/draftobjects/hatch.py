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
import FreeCAD as App
from draftutils.translate import translate, QT_TRANSLATE_NOOP
from draftgeoutils.general import geomType

from draftobjects.base import DraftObject


class Hatch(DraftObject):


    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
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

    def dumps(self):

        return None

    def loads(self,state):

        return None

    def execute(self,obj):

        if self.props_changed_placement_only(obj) \
                or not obj.Base \
                or not obj.File \
                or not obj.Pattern \
                or not obj.Scale \
                or not obj.Pattern in self.getPatterns(obj.File) \
                or not obj.Base.isDerivedFrom("Part::Feature") \
                or not obj.Base.Shape.Faces:
            self.props_changed_clear()
            return

        import Part
        import TechDraw

        shapes = []
        for face in obj.Base.Shape.Faces:
            if face.findPlane(): # Only planar faces.
                face = face.copy()
                if obj.Translate:
                    mtx = None
                    w = face.normalAt(0, 0)
                    # Try to base a matrix on the first straight edge with
                    # a reasonable length (> 0.001):
                    for e in face.Edges:
                        if geomType(e) == "Line":
                            sta = e.firstVertex().Point
                            end = e.lastVertex().Point
                            u = end.sub(sta)
                            if u.Length > 0.001:
                                u = u.normalize()
                                v = w.cross(u)
                                mtx = App.Matrix(u.x, v.x, w.x, sta.x,
                                                 u.y, v.y, w.y, sta.y,
                                                 u.z, v.z, w.z, sta.z,
                                                 0.0, 0.0, 0.0, 1.0)
                                break
                    # If no suitable straight edge was found use a default matrix:
                    if not mtx:
                        cen = face.CenterOfMass
                        rot = App.Rotation(App.Vector(0,0,1), w)
                        mtx = App.Placement(cen, rot).Matrix
                    face = face.transformGeometry(mtx.inverse()).Faces[0]
                if obj.Rotation.Value:
                    face.rotate(App.Vector(), App.Vector(0,0,1), -obj.Rotation)
                shape = TechDraw.makeGeomHatch(face, obj.Scale, obj.Pattern, obj.File)
                if obj.Rotation.Value:
                    shape.rotate(App.Vector(), App.Vector(0,0,1), obj.Rotation)
                if obj.Translate:
                    shape = shape.transformGeometry(mtx)
                shapes.append(shape)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
        self.props_changed_clear()

    def onChanged(self, obj, prop):

        self.props_changed_store(prop)

    def getPatterns(self,filename):

        """returns a list of pattern names found in a PAT file"""
        patterns = []
        if os.path.exists(filename):
            with open(filename) as patfile:
                for line in patfile:
                    if line.startswith("*"):
                        patterns.append(line.split(",")[0][1:])
        return patterns
