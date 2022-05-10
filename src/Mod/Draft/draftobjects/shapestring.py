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
"""Provides the object code for the ShapeString object."""
## @package shapestring
# \ingroup draftobjects
# \brief Provides the object code for the ShapeString object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftutils.utils as utils

from draftutils.translate import translate
from draftobjects.base import DraftObject


class ShapeString(DraftObject):
    """The ShapeString object"""

    def __init__(self, obj):
        super(ShapeString, self).__init__(obj, "ShapeString")

        _tip = QT_TRANSLATE_NOOP("App::Property", "Text string")
        obj.addProperty("App::PropertyString", "String", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Font file name")
        obj.addProperty("App::PropertyFile", "FontFile", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Height of text")
        obj.addProperty("App::PropertyLength", "Size", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Inter-character spacing")
        obj.addProperty("App::PropertyLength", "Tracking", "Draft", _tip)

        _tip = QT_TRANSLATE_NOOP("App::Property", "Fill letters with faces")
        obj.addProperty("App::PropertyBool", "MakeFace", "Draft", _tip).MakeFace = True

    def execute(self, obj):
        import Part
        # import OpenSCAD2Dgeom
        if obj.String and obj.FontFile:
            if obj.Placement:
                plm = obj.Placement

            CharList = Part.makeWireString(obj.String,obj.FontFile,obj.Size,obj.Tracking)
            if len(CharList) == 0:
                App.Console.PrintWarning(translate("draft","ShapeString: string has no wires")+"\n")
                return
            SSChars = []

            # test a simple letter to know if we have a sticky font or not
            sticky = False
            testWire = Part.makeWireString("L",obj.FontFile,obj.Size,obj.Tracking)[0][0]
            if testWire.isClosed:
                try:
                    testFace = Part.Face(testWire)
                except Part.OCCError:
                    sticky = True
                else:
                    if not testFace.isValid():
                        sticky = True
            else:
                sticky = True

            fill = True
            if hasattr(obj, "MakeFace"):
                fill = obj.MakeFace

            for char in CharList:
                if sticky or (not fill):
                    for CWire in char:
                        SSChars.append(CWire)
                else:
                    CharFaces = []
                    for CWire in char:
                        f = Part.Face(CWire)
                        if f:
                            CharFaces.append(f)
                    # whitespace (ex: ' ') has no faces. This breaks OpenSCAD2Dgeom...
                    if CharFaces:
                        # s = OpenSCAD2Dgeom.Overlappingfaces(CharFaces).makeshape()
                        # s = self.makeGlyph(CharFaces)
                        s = self.makeFaces(char)
                        SSChars.append(s)
            shape = Part.Compound(SSChars)
            obj.Shape = shape
            if plm:
                obj.Placement = plm
        obj.positionBySupport()

    def makeFaces(self, wireChar):
        import Part
        compFaces=[]
        allEdges = [] # unused variable?
        wirelist=sorted(wireChar,key=(lambda shape: shape.BoundBox.DiagonalLength),reverse=True)
        fixedwire = []
        for w in wirelist:
            compEdges = Part.Compound(w.Edges)
            compEdges = compEdges.connectEdgesToWires()
            fixedwire.append(compEdges.Wires[0])
        wirelist = fixedwire
        sep_wirelist = []
        while len(wirelist) > 0:
            wire2Face = [wirelist[0]]
            face = Part.Face(wirelist[0])
            for w in wirelist[1:]:
                p = w.Vertexes[0].Point
                u,v = face.Surface.parameter(p)
                if face.isPartOfDomain(u,v):
                    f = Part.Face(w)
                    if face.Orientation == f.Orientation:
                        if f.Surface.Axis * face.Surface.Axis < 0:
                            w.reverse()
                    else:
                        if f.Surface.Axis * face.Surface.Axis > 0:
                            w.reverse()
                    wire2Face.append(w)
                else:
                    sep_wirelist.append(w)
            wirelist = sep_wirelist
            sep_wirelist = []
            face = Part.Face(wire2Face)
            face.validate()
            try:
                # some fonts fail here
                if face.Surface.Axis.z < 0.0:
                    face.reverse()
            except Exception:
                pass
            compFaces.append(face)
        ret = Part.Compound(compFaces)
        return ret

    def makeGlyph(self, facelist):
        ''' turn list of simple contour faces into a compound shape representing a glyph '''
        ''' remove cuts, fuse overlapping contours, retain islands '''
        import Part
        if len(facelist) == 1:
            return(facelist[0])

        sortedfaces = sorted(facelist,key=(lambda shape: shape.Area),reverse=True)

        biggest = sortedfaces[0]
        result = biggest
        islands =[]
        for face in sortedfaces[1:]:
            bcfA = biggest.common(face).Area
            fA = face.Area
            difA = abs(bcfA - fA)
            eps = utils.epsilon()
            # if biggest.common(face).Area == face.Area:
            if difA <= eps:                              # close enough to zero
                # biggest completely overlaps current face ==> cut
                result = result.cut(face)
            # elif biggest.common(face).Area == 0:
            elif bcfA <= eps:
                # island
                islands.append(face)
            else:
                # partial overlap - (font designer error?)
                result = result.fuse(face)
        #glyphfaces = [result]
        wl = result.Wires
        for w in wl:
            w.fixWire()
        glyphfaces = [Part.Face(wl)]
        glyphfaces.extend(islands)
        ret = Part.Compound(glyphfaces) # should we fuse these instead of making compound?
        return ret


# Alias for compatibility with v0.18 and earlier
_ShapeString = ShapeString

## @}
