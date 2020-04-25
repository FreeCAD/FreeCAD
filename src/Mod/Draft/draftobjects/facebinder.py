# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""This module provides the object code for Draft Facebinder.
"""
## @package facebinder
# \ingroup DRAFT
# \brief This module provides the object code for Draft Facebinder.

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

from draftobjects.base import DraftObject


class Facebinder(DraftObject):
    """The Draft Facebinder object"""
    def __init__(self,obj):
        super(Facebinder, self).__init__(obj, "Facebinder")

        _tip = "Linked faces"
        obj.addProperty("App::PropertyLinkSubList", "Faces",
                        "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "Specifies if splitter lines must be removed"
        obj.addProperty("App::PropertyBool","RemoveSplitter",
                        "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "An optional extrusion value to be applied to all faces"
        obj.addProperty("App::PropertyDistance","Extrusion",
                        "Draft" ,QT_TRANSLATE_NOOP("App::Property", _tip))

        _tip = "This specifies if the shapes sew"
        obj.addProperty("App::PropertyBool","Sew",
                        "Draft", QT_TRANSLATE_NOOP("App::Property", _tip))


    def execute(self,obj):
        import Part
        pl = obj.Placement
        if not obj.Faces:
            return
        faces = []
        for sel in obj.Faces:
            for f in sel[1]:
                if "Face" in f:
                    try:
                        fnum = int(f[4:])-1
                        faces.append(sel[0].Shape.Faces[fnum])
                    except(IndexError, Part.OCCError):
                        print("Draft: wrong face index")
                        return
        if not faces:
            return
        try:
            if len(faces) > 1:
                sh = None
                if hasattr(obj, "Extrusion"):
                    if obj.Extrusion.Value:
                        for f in faces:
                            f = f.extrude(f.normalAt(0,0).multiply(obj.Extrusion.Value))
                            if sh:
                                sh = sh.fuse(f)
                            else:
                                sh = f
                if not sh:
                    sh = faces.pop()
                    sh = sh.multiFuse(faces)
                if hasattr(obj, "Sew"):
                    if obj.Sew:
                        sh = sh.copy()
                        sh.sewShape()
                if hasattr(obj, "RemoveSplitter"):
                    if obj.RemoveSplitter:
                        sh = sh.removeSplitter()
                else:
                    sh = sh.removeSplitter()
            else:
                sh = faces[0]
                if hasattr(obj, "Extrusion"):
                    if obj.Extrusion.Value:
                        sh = sh.extrude(sh.normalAt(0,0).multiply(obj.Extrusion.Value))
                sh.transformShape(sh.Matrix, True)
        except Part.OCCError:
            print("Draft: error building facebinder")
            return
        obj.Shape = sh
        obj.Placement = pl

    def addSubobjects(self,obj,facelinks):
        """adds facelinks to this facebinder"""
        objs = obj.Faces
        for o in facelinks:
            if isinstance(o, tuple) or isinstance(o, list):
                if o[0].Name != obj.Name:
                    objs.append(tuple(o))
            else:
                for el in o.SubElementNames:
                    if "Face" in el:
                        if o.Object.Name != obj.Name:
                            objs.append((o.Object, el))
        obj.Faces = objs
        self.execute(obj)


_Facebinder = Facebinder