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
        if self.props_changed_placement_only():
            obj.positionBySupport()
            self.props_changed_clear()
            return

        if obj.String and obj.FontFile:
            import Part

            if obj.Placement:
                plm = obj.Placement

            # test a simple letter to know if we have a sticky font or not
            sticky = False
            testWire = Part.makeWireString("L", obj.FontFile, obj.Size, obj.Tracking)[0][0]
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

            CharList = Part.makeWireString(obj.String, obj.FontFile, obj.Size, obj.Tracking)
            SSChars = []

            for char in CharList:
                if sticky or (not fill):
                    SSChars.extend(char)
                elif char:
                    SSChars.extend(self.makeFaces(char))
            if SSChars:
                shape = Part.Compound(SSChars)
                obj.Shape = shape
            else:
                App.Console.PrintWarning(translate("draft", "ShapeString: string has no wires")+"\n")

            if plm:
                obj.Placement = plm

        obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)

    def makeFaces(self, wireChar):
        import Part

        wrn = translate("draft", "ShapeString: face creation failed for one character") + "\n"

        wirelist = []
        for w in wireChar:
            compEdges = Part.Compound(w.Edges)
            compEdges = compEdges.connectEdgesToWires()
            if compEdges.Wires[0].isClosed():
                wirelist.append(compEdges.Wires[0])

        if not wirelist:
            App.Console.PrintWarning(wrn)
            return []

        # Some test fonts:
        # https://raw.githubusercontent.com/FreeCAD/FPA/main/images/freecad_logo_official.svg
        #     https://evolventa.github.io/
        #     not a problem font, but it is used by FreeCAD
        # https://forum.freecad.org/viewtopic.php?t=57774
        #     https://www.dafont.com/mutlu-ornamental.font
        # https://forum.freecad.org/viewtopic.php?t=65110&p=559810#p559886
        #     http://www.atelier-des-fougeres.fr/Cambam/Aide/Plugins/stickfonts.html

        # 1CamBam_Stick_0.ttf is actually not a stick font.

        # FaceMakerBullseye:
        #     1CamBam_Stick_0.ttf face validation problem with A, E, F, H, K, R, Y and y.
        # FaceMakerCheese:
        #     1CamBam_Stick_0.ttf face creation problem with: A, E, F, H, Q, R, e and y.
        #     All fonts: face creation problem in case of double-nested wires f.e. with: ©.
        # FaceMakerSimple:
        #     All fonts: overlapping faces in case of nested wires f.e. with: O.
        try:
            # print("try Bullseye")
            faces = Part.makeFace(wirelist, "Part::FaceMakerBullseye").Faces
            for face in faces:
                face.validate()
        except Part.OCCError:
            try:
                # print("try Cheese")
                faces = Part.makeFace(wirelist, "Part::FaceMakerCheese").Faces
                for face in faces:
                    face.validate()
            except Part.OCCError:
                try:
                    # print("try Simple")
                    faces = Part.makeFace(wirelist, "Part::FaceMakerSimple").Faces
                    for face in faces:
                        face.validate()
                except Part.OCCError:
                    App.Console.PrintWarning(wrn)
                    return []

        for face in faces:
            try:
                # some fonts fail here
                if face.Surface.Axis.z < 0.0: # Does not seem to occur for FaceMakerBullseye.
                    face.reverse()
            except Exception:
                pass

        return faces

    def makeGlyph(self, facelist):
        ''' turn list of simple contour faces into a compound shape representing a glyph '''
        ''' remove cuts, fuse overlapping contours, retain islands '''
        import Part
        if len(facelist) == 1:
            return facelist[0]

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
