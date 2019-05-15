#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 Yorik van Havre <yorik@uncreated.net>              *
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

__title__= "FreeCAD Precast concrete module"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

"""This module contains tools to build basic precast concrete elements:
Beams, pillars, slabs and panels"""

import ArchCommands,ArchComponent,FreeCAD
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchPrecast
#  \ingroup ARCH
#  \brief Precast options for ArchStructure
#
#  This module provides additional presets for the Arch Structure
#  tool, to build a series of precast concrete elements

class _Precast(ArchComponent.Component):

    "The base Precast class"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        _Precast.setProperties(self,obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Length" in pl:
            obj.addProperty("App::PropertyDistance","Length","Structure",QT_TRANSLATE_NOOP("App::Property","The length of this element"))
        if not "Width" in pl:
            obj.addProperty("App::PropertyDistance","Width","Structure",QT_TRANSLATE_NOOP("App::Property","The width of this element"))
        if not "Height" in pl:
            obj.addProperty("App::PropertyDistance","Height","Structure",QT_TRANSLATE_NOOP("App::Property","The height of this element"))
        if not "Nodes" in pl:
            obj.addProperty("App::PropertyVectorList","Nodes","Structure",QT_TRANSLATE_NOOP("App::Property","The structural nodes of this element"))
        self.Type = "Precast"

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        _Precast.setProperties(self,obj)

    def execute(self,obj):

        if self.clone(obj):
            return


class _PrecastBeam(_Precast):

    "The Precast Beam"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Beam"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Chamfer" in pl:
            obj.addProperty("App::PropertyDistance","Chamfer","Beam",QT_TRANSLATE_NOOP("App::Property","The size of the chamfer of this element"))
        if not "DentLength" in pl:
            obj.addProperty("App::PropertyDistance","DentLength","Beam",QT_TRANSLATE_NOOP("App::Property","The dent length of this element"))
        if not "DentHeight" in pl:
            obj.addProperty("App::PropertyDistance","DentHeight","Beam",QT_TRANSLATE_NOOP("App::Property","The dent height of this element"))
        if not "Dents" in pl:
            obj.addProperty("App::PropertyStringList","Dents","Beam",QT_TRANSLATE_NOOP("App::Property","The dents of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        chamfer = obj.Chamfer.Value
        dentlength = obj.DentLength.Value
        dentheight = obj.DentHeight.Value
        dents = obj.Dents

        if (length == 0) or (width == 0) or (height == 0):
            return
        if (chamfer >= width/2) or (chamfer >= height/2):
            return

        import Part
        p = []
        if chamfer > 0:
            p.append(Vector(0,chamfer,0))
            p.append(Vector(0,width-chamfer,0))
            p.append(Vector(0,width,chamfer))
            p.append(Vector(0,width,height-chamfer))
            p.append(Vector(0,width-chamfer,height))
            p.append(Vector(0,chamfer,height))
            p.append(Vector(0,0,height-chamfer))
            p.append(Vector(0,0,chamfer))
        else:
            p.append(Vector(0,0,0))
            p.append(Vector(0,width,0))
            p.append(Vector(0,width,height))
            p.append(Vector(0,0,height))
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(length,0,0))
        if (dentlength > 0) and (dentheight > 0):
            p = []
            p.append(Vector(0,0,0))
            p.append(Vector(dentlength,0,0))
            p.append(Vector(dentlength,width,0))
            p.append(Vector(0,width,0))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            d1 = f.extrude(Vector(0,0,dentheight))
            d2 = d1.copy()
            d2.translate(Vector(length-dentlength,0,0))
            shape = shape.cut(d1)
            shape = shape.cut(d2)
        for dent in dents:
            dent = dent.split(";")
            if len(dent) == 7:
                dentlength = float(dent[0])
                dentwidth = float(dent[1])
                dentheight = float(dent[2])
                dentslant = float(dent[3])
                dentchamfer = chamfer
                dentlevel = float(dent[4])
                dentrotation = float(dent[5])
                dentoffset = float(dent[6])
                if (dentlength == 0) or (dentwidth == 0) or (dentheight == 0):
                    continue
                if dentslant >= dentheight:
                    continue
                p = []
                p.append(Vector(0-dentchamfer,0,0))
                p.append(Vector(dentlength,0,dentslant))
                p.append(Vector(dentlength,0,dentheight))
                p.append(Vector(0-dentchamfer,0,dentheight))
                p.append(p[0])
                p = Part.makePolygon(p)
                f = Part.Face(p)
                dentshape = f.extrude(Vector(0,dentwidth,0))
                dentshape.rotate(Vector(0,0,0),Vector(0,0,1),dentrotation)
                if dentrotation == 0:
                    dentshape.translate(Vector(length,dentoffset,0))
                elif dentrotation == 90:
                    dentshape.translate(Vector(length-dentoffset,width,0))
                elif dentrotation == 180:
                    dentshape.translate(Vector(0,width-dentoffset,0))
                elif dentrotation == 270:
                    dentshape.translate(Vector(dentoffset,0,0))
                dentshape.translate(Vector(0,0,dentlevel))
                shape = shape.fuse(dentshape)
        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _PrecastIbeam(_Precast):

    "The Precast Ibeam"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Beam"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Chamfer" in pl:
            obj.addProperty("App::PropertyDistance","Chamfer","Beam",QT_TRANSLATE_NOOP("App::Property","The chamfer length of this element"))
        if not "BeamBase" in pl:
            obj.addProperty("App::PropertyDistance","BeamBase","Beam",QT_TRANSLATE_NOOP("App::Property","The base length of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        base = obj.BeamBase.Value
        slant = obj.Chamfer.Value

        if (length == 0) or (width == 0) or (height == 0):
            return
        if (slant*2 >= width) or (base*2+slant*2 >= height):
            return

        import Part
        p = []
        p.append(Vector(0,0,0))
        p.append(Vector(0,0,base))
        p.append(Vector(0,slant,base+slant))
        p.append(Vector(0,slant,height-(base+slant)))
        p.append(Vector(0,0,height-base))
        p.append(Vector(0,0,height))
        p.append(Vector(0,width,height))
        p.append(Vector(0,width,height-base))
        p.append(Vector(0,width-slant,height-(base+slant)))
        p.append(Vector(0,width-slant,base+slant))
        p.append(Vector(0,width,base))
        p.append(Vector(0,width,0))
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(length,0,0))

        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _PrecastPillar(_Precast):

    "The Precast Pillar"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Column"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Chamfer" in pl:
            obj.addProperty("App::PropertyDistance","Chamfer","Column",QT_TRANSLATE_NOOP("App::Property","The size of the chamfer of this element"))
        if not "GrooveDepth" in pl:
            obj.addProperty("App::PropertyDistance","GrooveDepth","Column",QT_TRANSLATE_NOOP("App::Property","The groove depth of this element"))
        if not "GrooveHeight" in pl:
            obj.addProperty("App::PropertyDistance","GrooveHeight","Column",QT_TRANSLATE_NOOP("App::Property","The groove height of this element"))
        if not "GrooveSpacing" in pl:
            obj.addProperty("App::PropertyDistance","GrooveSpacing","Column",QT_TRANSLATE_NOOP("App::Property","The spacing between the grooves of this element"))
        if not "GrooveNumber" in pl:
            obj.addProperty("App::PropertyInteger","GrooveNumber","Column",QT_TRANSLATE_NOOP("App::Property","The number of grooves of this element"))
        if not "Dents" in pl:
            obj.addProperty("App::PropertyStringList","Dents","Column",QT_TRANSLATE_NOOP("App::Property","The dents of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        chamfer = obj.Chamfer.Value
        groovedepth = obj.GrooveDepth.Value
        grooveheight = obj.GrooveHeight.Value
        spacing = obj.GrooveSpacing.Value
        number = obj.GrooveNumber
        dents = obj.Dents

        if (length == 0) or (width == 0) or (height == 0):
            return
        if (chamfer >= width/2) or (chamfer >= length/2):
            return

        import Part
        p = []
        if chamfer > 0:
            p.append(Vector(chamfer,0,0))
            p.append(Vector(length-chamfer,0,0))
            p.append(Vector(length,chamfer,0))
            p.append(Vector(length,width-chamfer,0))
            p.append(Vector(length-chamfer,width,0))
            p.append(Vector(chamfer,width,0))
            p.append(Vector(0,width-chamfer,0))
            p.append(Vector(0,chamfer,0))
        else:
            p.append(Vector(0,0,0))
            p.append(Vector(length,0,0))
            p.append(Vector(length,width,0))
            p.append(Vector(0,width,0))
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(0,0,height))

        if (groovedepth > 0) and (grooveheight > 0) and (spacing > 0) and (number > 0) and (groovedepth < length/2) and (groovedepth < width/2):
            p1 = []
            p1.append(Vector(0,0,0))
            p1.append(Vector(length,0,0))
            p1.append(Vector(length,width,0))
            p1.append(Vector(0,width,0))
            p1.append(p1[0])
            p1 = Part.makePolygon(p1)
            f1 = Part.Face(p1)
            groove = f1.extrude(Vector(0,0,grooveheight))
            p2 = []
            p2.append(Vector(groovedepth,groovedepth,0))
            p2.append(Vector(length-groovedepth,groovedepth,0))
            p2.append(Vector(length-groovedepth,width-groovedepth,0))
            p2.append(Vector(groovedepth,width-groovedepth,0))
            p2.append(p2[0])
            p2 = Part.makePolygon(p2)
            f2 = Part.Face(p2)
            s = f2.extrude(Vector(0,0,grooveheight))
            groove = groove.cut(s)
            for i in range(number):
                g = groove.copy()
                g.translate(Vector(0,0,spacing + i*(spacing+grooveheight)))
                shape = shape.cut(g)

        for dent in dents:
            dent = dent.split(";")
            if len(dent) == 7:
                dentlength = float(dent[0])
                dentwidth = float(dent[1])
                dentheight = float(dent[2])
                dentslant = float(dent[3])
                dentchamfer = chamfer
                dentlevel = float(dent[4])
                dentrotation = float(dent[5])
                dentoffset = float(dent[6])
                if (dentlength == 0) or (dentwidth == 0) or (dentheight == 0):
                    continue
                if dentslant >= dentheight:
                    continue
                p = []
                p.append(Vector(0-dentchamfer,0,0))
                p.append(Vector(dentlength,0,dentslant))
                p.append(Vector(dentlength,0,dentheight))
                p.append(Vector(0-dentchamfer,0,dentheight))
                p.append(p[0])
                p = Part.makePolygon(p)
                f = Part.Face(p)
                dentshape = f.extrude(Vector(0,dentwidth,0))
                dentshape.rotate(Vector(0,0,0),Vector(0,0,1),dentrotation)
                if dentrotation == 0:
                    dentshape.translate(Vector(length,dentoffset,0))
                elif dentrotation == 90:
                    dentshape.translate(Vector(length-dentoffset,width,0))
                elif dentrotation == 180:
                    dentshape.translate(Vector(0,width-dentoffset,0))
                elif dentrotation == 270:
                    dentshape.translate(Vector(dentoffset,0,0))
                dentshape.translate(Vector(0,0,dentlevel))
                shape = shape.fuse(dentshape)

        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _PrecastPanel(_Precast):

    "The Precast Panel"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Plate"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Chamfer" in pl:
            obj.addProperty("App::PropertyDistance","Chamfer","Panel",QT_TRANSLATE_NOOP("App::Property","The size of the chamfer of this element"))
        if not "DentWidth" in pl:
            obj.addProperty("App::PropertyDistance","DentWidth","Panel",QT_TRANSLATE_NOOP("App::Property","The dent width of this element"))
        if not "DentHeight" in pl:
            obj.addProperty("App::PropertyDistance","DentHeight","Panel",QT_TRANSLATE_NOOP("App::Property","The dent height of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        chamfer = obj.Chamfer.Value
        dentheight = obj.DentHeight.Value
        dentwidth = obj.DentWidth.Value

        if (length == 0) or (width == 0) or (height == 0):
            return
        if ((chamfer+dentwidth) >= width) or (dentheight >= height):
            return

        import Part
        p = []
        p.append(Vector(0,0,0))
        p.append(Vector(length,0,0))
        p.append(Vector(length,width,0))
        p.append(Vector(0,width,0))
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(0,0,height))
        if chamfer > 0:
            p = []
            p.append(Vector(0,width-chamfer,0))
            p.append(Vector(chamfer,width,0))
            p.append(Vector(0,width,0))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            s = f.extrude(Vector(0,0,height))
            shape = shape.cut(s)
            p = []
            p.append(Vector(length,width-chamfer,0))
            p.append(Vector(length-chamfer,width,0))
            p.append(Vector(length,width,0))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            s = f.extrude(Vector(0,0,height))
            shape = shape.cut(s)
            p = []
            p.append(Vector(0,width-chamfer,0))
            p.append(Vector(0,width,chamfer))
            p.append(Vector(0,width,0))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            s = f.extrude(Vector(length,0,0))
            shape = shape.cut(s)
            p = []
            p.append(Vector(0,width-chamfer,height))
            p.append(Vector(0,width,height-chamfer))
            p.append(Vector(0,width,height))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            s = f.extrude(Vector(length,0,0))
            shape = shape.cut(s)
        if (dentheight > 0) and (dentwidth > 0):
            p = []
            p.append(Vector(0,((width-chamfer)-dentwidth)/2,0))
            p.append(Vector(0,((width-chamfer)-dentwidth)/2+dentwidth,0))
            p.append(Vector(0,((width-chamfer)-dentwidth)/2+dentwidth,dentheight))
            p.append(Vector(0,((width-chamfer)-dentwidth)/2,dentheight))
            p.append(p[0])
            p = Part.makePolygon(p)
            f = Part.Face(p)
            s = f.extrude(Vector(length,0,0))
            shape = shape.cut(s)
            s.translate(Vector(0,0,height))
            shape = shape.fuse(s)

        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _PrecastSlab(_Precast):

    "The Precast Slab"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Slab"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "SlabType" in pl:
            obj.addProperty("App::PropertyEnumeration","SlabType","Slab",QT_TRANSLATE_NOOP("App::Property","The type of this slab"))
            obj.SlabType = ["Champagne","Hat"]
        if not "SlabBase" in pl:
            obj.addProperty("App::PropertyDistance","SlabBase","Slab",QT_TRANSLATE_NOOP("App::Property","The size of the base of this element"))
        if not "HoleNumber" in pl:
            obj.addProperty("App::PropertyInteger","HoleNumber","Slab",QT_TRANSLATE_NOOP("App::Property","The number of holes in this element"))
        if not "HoleMajor" in pl:
            obj.addProperty("App::PropertyDistance","HoleMajor","Slab",QT_TRANSLATE_NOOP("App::Property","The major radius of the holes of this element"))
        if not "HoleMinor" in pl:
            obj.addProperty("App::PropertyDistance","HoleMinor","Slab",QT_TRANSLATE_NOOP("App::Property","The minor radius of the holes of this element"))
        if not "HoleSpacing" in pl:
            obj.addProperty("App::PropertyDistance","HoleSpacing","Slab",QT_TRANSLATE_NOOP("App::Property","The spacing between the holes of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        slabtype = obj.SlabType
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        base = obj.SlabBase.Value
        holenumber = obj.HoleNumber
        holemajor = obj.HoleMajor.Value
        holeminor = obj.HoleMinor.Value
        holespacing = obj.HoleSpacing.Value

        slant = (height-base) / 3 # this gives the inclination of the vertical walls

        if (length == 0) or (width == 0) or (height == 0):
            return
        if base >= height:
            return
        if height < (base*2):
            return
        if (holenumber > 0) and ( (holespacing == 0) or (holemajor == 0) or (holeminor == 0) ):
            return
        if holemajor < holeminor:
            return
        import Part

        p = []
        if slabtype == "Champagne":
            p.append(Vector(0,0,0))
            p.append(Vector(0,slant,height-base))
            p.append(Vector(0,0,height-base))
            p.append(Vector(0,0,height))
            p.append(Vector(0,width,height))
            p.append(Vector(0,width,height-base))
            p.append(Vector(0,width-slant,height-base))
            p.append(Vector(0,width,0))
        elif slabtype == "Hat":
            p.append(Vector(0,0,0))
            p.append(Vector(0,0,base))
            p.append(Vector(0,slant,base))
            p.append(Vector(0,slant*2,height))
            p.append(Vector(0,width-slant*2,height))
            p.append(Vector(0,width-slant,base))
            p.append(Vector(0,width,base))
            p.append(Vector(0,width,0))
        else:
            return None
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(length,0,0))

        if holenumber > 0:
            holespan = holenumber * holeminor + (holenumber - 1) * holespacing
            holestart = (width/2 - holespan/2) + holeminor/2
            if holeminor != holemajor:
                e = Part.Ellipse(Vector(0,0,0),Vector(0,holeminor/2,0),Vector(0,0,holemajor/2)).toShape()
                e.translate(Vector(0,0,-holemajor/2))
            else:
                e = Part.Circle(Vector(0,0,0),Vector(1,0,0),holemajor/2).toShape()
            w = Part.Wire([e])
            f = Part.Face(w)
            tube = f.extrude(Vector(length,0,0))
            for i in range(holenumber):
                x = holestart + i*(holeminor+holespacing)
                s = tube.copy()
                s.translate(Vector(0,x,height/2))
                shape = shape.cut(s)

        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _PrecastStairs(_Precast):

    "The Precast Stairs"

    def __init__(self,obj):

        _Precast.__init__(self,obj)
        self.setProperties(obj)
        obj.IfcType = "Stair"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "DownLength" in pl:
            obj.addProperty("App::PropertyDistance","DownLength","Stairs",QT_TRANSLATE_NOOP("App::Property","The length of the down floor of this element"))
        if not "RiserNumber" in pl:
            obj.addProperty("App::PropertyInteger","RiserNumber","Stairs",QT_TRANSLATE_NOOP("App::Property","The number of risers in this element"))
        if not "Riser" in pl:
            obj.addProperty("App::PropertyDistance","Riser","Stairs",QT_TRANSLATE_NOOP("App::Property","The riser height of this element"))
        if not "Tread" in pl:
            obj.addProperty("App::PropertyDistance","Tread","Stairs",QT_TRANSLATE_NOOP("App::Property","The tread depth of this element"))

    def onDocumentRestored(self,obj):

        _Precast.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):

        if self.clone(obj):
            return

        pl = obj.Placement
        length = obj.Length.Value
        width = obj.Width.Value
        height = obj.Height.Value
        downlength = obj.DownLength.Value
        steps = obj.RiserNumber
        riser = obj.Riser.Value
        tread = obj.Tread.Value

        if not width:
            return
        if not steps:
            return
        if not riser:
            return
        if not tread:
            return
        if not height:
            return
        if length < tread:
            length = tread # minimum

        import math,Part

        p = [Vector(0,0,0)] # relative moves
        if downlength:
            p.append(Vector(0,downlength,0))
        for i in range(steps-1):
            p.append(Vector(0,0,riser))
            p.append(Vector(0,tread,0))
        p.append(Vector(0,0,riser))
        p.append(Vector(0,length,0))
        ang1 = math.atan(riser/tread)
        ang2 = ang1/2
        rdist = math.tan(ang2)*height
        if length > (tread+rdist):
            p.append(Vector(0,0,-height))
            p.append(Vector(0,-(length-(tread+rdist))))
        else:
            rest = length-(tread+rdist)
            addh = math.tan(ang1)*rest
            p.append(Vector(0,0,-(height+addh)))
        # absolutize
        r = [p[0]]
        for m in p[1:]:
            r.append(r[-1].add(m))
        p = r
        if downlength:
            bdist = math.tan(ang1)*height
            p.append(Vector(0,downlength+bdist,-height))
            p.append(Vector(0,0,-height))
        else:
            hdist = height*(math.tan(ang1))
            p.append(Vector(0,hdist,0))
        p.append(p[0])
        p = Part.makePolygon(p)
        f = Part.Face(p)
        shape = f.extrude(Vector(width,0,0))

        shape = self.processSubShapes(obj,shape,pl)
        self.applyShape(obj,shape,pl)


class _ViewProviderPrecast(ArchComponent.ViewProviderComponent):

    "The View Provider of the Precast object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Structure")

    def getIcon(self):

        import Arch_rc
        if hasattr(self,"Object"):
            if self.Object.CloneOf:
                return ":/icons/Arch_Structure_Clone.svg"
        return ":/icons/Arch_Structure_Tree.svg"

    def setEdit(self,vobj,mode):

        if mode == 0:
            import FreeCADGui
            taskd = ArchComponent.ComponentTaskPanel()
            taskd.obj = self.Object
            taskd.update()
            if hasattr(self.Object,"Dents"):
                self.dentd = _DentsTaskPanel()
                self.dentd.form.show()
                self.dentd.fillDents(self.Object.Dents)
                taskd.form = [taskd.form,self.dentd.form]
            FreeCADGui.Control.showDialog(taskd)
            return True
        return False

    def unsetEdit(self,vobj,mode):

        import FreeCADGui
        if hasattr(self,"dentd"):
            self.Object.Dents = self.dentd.getValues()
            del self.dentd
        FreeCADGui.Control.closeDialog()
        return False


class _PrecastTaskPanel:

    '''The TaskPanel for precast creation'''

    def __init__(self):

        import FreeCADGui
        from PySide import QtCore,QtGui,QtSvg
        self.form = QtGui.QWidget()
        self.grid = QtGui.QGridLayout(self.form)
        self.PrecastTypes = ["Beam","I-Beam","Pillar","Panel","Slab","Stairs"]
        self.SlabTypes = ["Champagne","Hat"]

        # image display
        self.preview = QtSvg.QSvgWidget(":/ui/ParametersBeam.svg")
        self.preview.setMaximumWidth(200)
        self.preview.setMinimumHeight(120)
        self.grid.addWidget(self.preview,0,0,1,2)

        # parameters
        self.labelSlabType = QtGui.QLabel()
        self.valueSlabType = QtGui.QComboBox()
        self.valueSlabType.addItems(self.SlabTypes)
        self.valueSlabType.setCurrentIndex(0)
        self.grid.addWidget(self.labelSlabType,1,0,1,1)
        self.grid.addWidget(self.valueSlabType,1,1,1,1)

        self.labelChamfer = QtGui.QLabel()
        self.valueChamfer = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelChamfer,2,0,1,1)
        self.grid.addWidget(self.valueChamfer,2,1,1,1)

        self.labelDentLength = QtGui.QLabel()
        self.valueDentLength = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelDentLength,3,0,1,1)
        self.grid.addWidget(self.valueDentLength,3,1,1,1)

        self.labelDentWidth = QtGui.QLabel()
        self.valueDentWidth = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelDentWidth,4,0,1,1)
        self.grid.addWidget(self.valueDentWidth,4,1,1,1)

        self.labelDentHeight = QtGui.QLabel()
        self.valueDentHeight = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelDentHeight,5,0,1,1)
        self.grid.addWidget(self.valueDentHeight,5,1,1,1)

        self.labelBase = QtGui.QLabel()
        self.valueBase = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelBase,6,0,1,1)
        self.grid.addWidget(self.valueBase,6,1,1,1)

        self.labelHoleNumber = QtGui.QLabel()
        self.valueHoleNumber = QtGui.QSpinBox()
        self.grid.addWidget(self.labelHoleNumber,7,0,1,1)
        self.grid.addWidget(self.valueHoleNumber,7,1,1,1)

        self.labelHoleMajor = QtGui.QLabel()
        self.valueHoleMajor = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelHoleMajor,8,0,1,1)
        self.grid.addWidget(self.valueHoleMajor,8,1,1,1)

        self.labelHoleMinor = QtGui.QLabel()
        self.valueHoleMinor = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelHoleMinor,9,0,1,1)
        self.grid.addWidget(self.valueHoleMinor,9,1,1,1)

        self.labelHoleSpacing = QtGui.QLabel()
        self.valueHoleSpacing = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelHoleSpacing,10,0,1,1)
        self.grid.addWidget(self.valueHoleSpacing,10,1,1,1)

        self.labelGrooveNumber = QtGui.QLabel()
        self.valueGrooveNumber = QtGui.QSpinBox()
        self.grid.addWidget(self.labelGrooveNumber,11,0,1,1)
        self.grid.addWidget(self.valueGrooveNumber,11,1,1,1)

        self.labelGrooveDepth = QtGui.QLabel()
        self.valueGrooveDepth = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelGrooveDepth,12,0,1,1)
        self.grid.addWidget(self.valueGrooveDepth,12,1,1,1)

        self.labelGrooveHeight = QtGui.QLabel()
        self.valueGrooveHeight = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelGrooveHeight,13,0,1,1)
        self.grid.addWidget(self.valueGrooveHeight,13,1,1,1)

        self.labelGrooveSpacing = QtGui.QLabel()
        self.valueGrooveSpacing = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelGrooveSpacing,14,0,1,1)
        self.grid.addWidget(self.valueGrooveSpacing,14,1,1,1)

        self.labelRiserNumber = QtGui.QLabel()
        self.valueRiserNumber = QtGui.QSpinBox()
        self.grid.addWidget(self.labelRiserNumber,15,0,1,1)
        self.grid.addWidget(self.valueRiserNumber,15,1,1,1)

        self.labelDownLength = QtGui.QLabel()
        self.valueDownLength = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelDownLength,16,0,1,1)
        self.grid.addWidget(self.valueDownLength,16,1,1,1)

        self.labelRiser = QtGui.QLabel()
        self.valueRiser = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelRiser,17,0,1,1)
        self.grid.addWidget(self.valueRiser,17,1,1,1)

        self.labelTread = QtGui.QLabel()
        self.valueTread = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelTread,18,0,1,1)
        self.grid.addWidget(self.valueTread,18,1,1,1)

        # signals/slots
        QtCore.QObject.connect(self.valueChamfer,QtCore.SIGNAL("valueChanged(double)"),self.setChamfer)
        QtCore.QObject.connect(self.valueDentLength,QtCore.SIGNAL("valueChanged(double)"),self.setDentLength)
        QtCore.QObject.connect(self.valueDentWidth,QtCore.SIGNAL("valueChanged(double)"),self.setDentWidth)
        QtCore.QObject.connect(self.valueDentHeight,QtCore.SIGNAL("valueChanged(double)"),self.setDentHeight)
        QtCore.QObject.connect(self.valueBase,QtCore.SIGNAL("valueChanged(double)"),self.setBase)
        QtCore.QObject.connect(self.valueHoleMajor,QtCore.SIGNAL("valueChanged(double)"),self.setHoleMajor)
        QtCore.QObject.connect(self.valueHoleMinor,QtCore.SIGNAL("valueChanged(double)"),self.setHoleMinor)
        QtCore.QObject.connect(self.valueHoleSpacing,QtCore.SIGNAL("valueChanged(double)"),self.setHoleSpacing)
        QtCore.QObject.connect(self.valueGrooveDepth,QtCore.SIGNAL("valueChanged(double)"),self.setGrooveDepth)
        QtCore.QObject.connect(self.valueGrooveHeight,QtCore.SIGNAL("valueChanged(double)"),self.setGrooveHeight)
        QtCore.QObject.connect(self.valueGrooveSpacing,QtCore.SIGNAL("valueChanged(double)"),self.setGrooveSpacing)
        QtCore.QObject.connect(self.valueDownLength,QtCore.SIGNAL("valueChanged(double)"),self.setDownLength)
        QtCore.QObject.connect(self.valueRiser,QtCore.SIGNAL("valueChanged(double)"),self.setRiser)
        QtCore.QObject.connect(self.valueTread,QtCore.SIGNAL("valueChanged(double)"),self.setTread)
        self.retranslateUi(self.form)
        self.form.hide()

    def getValues(self):
        d = {}
        d["SlabType"] = self.SlabTypes[self.valueSlabType.currentIndex()]
        d["Chamfer"] = self.Chamfer
        d["DentLength"] = self.DentLength
        d["DentWidth"] = self.DentWidth
        d["DentHeight"] = self.DentHeight
        d["Base"] = self.Base
        d["HoleNumber"] = self.valueHoleNumber.value()
        d["HoleMajor"] = self.HoleMajor
        d["HoleMinor"] = self.HoleMinor
        d["HoleSpacing"] = self.HoleSpacing
        d["GrooveNumber"] = self.valueGrooveNumber.value()
        d["GrooveDepth"] = self.GrooveDepth
        d["GrooveHeight"] = self.GrooveHeight
        d["GrooveSpacing"] = self.GrooveSpacing
        d["RiserNumber"] = self.valueRiserNumber.value()
        d["DownLength"] = self.DownLength
        d["Riser"] = self.Riser
        d["Tread"] = self.Tread
        if hasattr(self,"Dents"):
            d["Dents"] = self.Dents.getValues()
        return d

    def setChamfer(self,value):
        self.Chamfer = value

    def setDentLength(self,value):
        self.DentLength = value

    def setDentWidth(self,value):
        self.DentWidth = value

    def setDentHeight(self,value):
        self.DentHeight = value

    def setBase(self,value):
        self.Base = value

    def setHoleMajor(self,value):
        self.HoleMajor = value

    def setHoleMinor(self,value):
        self.HoleMinor = value

    def setHoleSpacing(self,value):
        self.HoleSpacing = value

    def setGrooveDepth(self,value):
        self.GrooveDepth = value

    def setGrooveHeight(self,value):
        self.GrooveHeight = value

    def setGrooveSpacing(self,value):
        self.GrooveSpacing = value

    def setDownLength(self,value):
        self.DownLength = value

    def setRiser(self,value):
        self.Riser = value

    def setTread(self,value):
        self.Tread = value

    def retranslateUi(self, dialog):
        from PySide import QtGui
        self.form.setWindowTitle(QtGui.QApplication.translate("Arch", "Precast elements", None))
        self.labelSlabType.setText(QtGui.QApplication.translate("Arch", "Slab type", None))
        self.labelChamfer.setText(QtGui.QApplication.translate("Arch", "Chamfer", None))
        self.labelDentLength.setText(QtGui.QApplication.translate("Arch", "Dent length", None))
        self.labelDentWidth.setText(QtGui.QApplication.translate("Arch", "Dent width", None))
        self.labelDentHeight.setText(QtGui.QApplication.translate("Arch", "Dent height", None))
        self.labelBase.setText(QtGui.QApplication.translate("Arch", "Slab base", None))
        self.labelHoleNumber.setText(QtGui.QApplication.translate("Arch", "Number of holes", None))
        self.labelHoleMajor.setText(QtGui.QApplication.translate("Arch", "Major diameter of holes", None))
        self.labelHoleMinor.setText(QtGui.QApplication.translate("Arch", "Minor diameter of holes", None))
        self.labelHoleSpacing.setText(QtGui.QApplication.translate("Arch", "Spacing between holes", None))
        self.labelGrooveNumber.setText(QtGui.QApplication.translate("Arch", "Number of grooves", None))
        self.labelGrooveDepth.setText(QtGui.QApplication.translate("Arch", "Depth of grooves", None))
        self.labelGrooveHeight.setText(QtGui.QApplication.translate("Arch", "Height of grooves", None))
        self.labelGrooveSpacing.setText(QtGui.QApplication.translate("Arch", "Spacing between grooves", None))
        self.labelRiserNumber.setText(QtGui.QApplication.translate("Arch", "Number of risers", None))
        self.labelDownLength.setText(QtGui.QApplication.translate("Arch", "Length of down floor", None))
        self.labelRiser.setText(QtGui.QApplication.translate("Arch", "Height of risers", None))
        self.labelTread.setText(QtGui.QApplication.translate("Arch", "Depth of treads", None))

    def setPreset(self,preset):
        self.preview.hide()
        if preset == "Beam":
            self.preview.load(":/ui/ParametersBeam.svg")
            self.labelSlabType.hide()
            self.valueSlabType.hide()
            self.labelChamfer.show()
            self.valueChamfer.show()
            self.labelDentLength.show()
            self.valueDentLength.show()
            self.labelDentWidth.hide()
            self.valueDentWidth.hide()
            self.labelDentHeight.show()
            self.valueDentHeight.show()
            self.labelBase.hide()
            self.valueBase.hide()
            self.labelHoleNumber.hide()
            self.valueHoleNumber.hide()
            self.labelHoleMajor.hide()
            self.valueHoleMajor.hide()
            self.labelHoleMinor.hide()
            self.valueHoleMinor.hide()
            self.labelHoleSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelGrooveNumber.hide()
            self.valueGrooveNumber.hide()
            self.labelGrooveDepth.hide()
            self.valueGrooveDepth.hide()
            self.labelGrooveHeight.hide()
            self.valueGrooveHeight.hide()
            self.labelGrooveSpacing.hide()
            self.valueGrooveSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelRiserNumber.hide()
            self.valueRiserNumber.hide()
            self.labelDownLength.hide()
            self.valueDownLength.hide()
            self.labelRiser.hide()
            self.valueRiser.hide()
            self.labelTread.hide()
            self.valueTread.hide()
        elif preset == "Pillar":
            self.preview.load(":/ui/ParametersPillar.svg")
            self.labelSlabType.hide()
            self.valueSlabType.hide()
            self.labelChamfer.show()
            self.valueChamfer.show()
            self.labelDentLength.hide()
            self.valueDentLength.hide()
            self.labelDentWidth.hide()
            self.valueDentWidth.hide()
            self.labelDentHeight.hide()
            self.valueDentHeight.hide()
            self.labelBase.hide()
            self.valueBase.hide()
            self.labelHoleNumber.hide()
            self.valueHoleNumber.hide()
            self.labelHoleMajor.hide()
            self.valueHoleMajor.hide()
            self.labelHoleMinor.hide()
            self.valueHoleMinor.hide()
            self.labelHoleSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelGrooveNumber.show()
            self.valueGrooveNumber.show()
            self.labelGrooveDepth.show()
            self.valueGrooveDepth.show()
            self.labelGrooveHeight.show()
            self.valueGrooveHeight.show()
            self.labelGrooveSpacing.show()
            self.valueGrooveSpacing.show()
            self.labelRiserNumber.hide()
            self.valueRiserNumber.hide()
            self.labelDownLength.hide()
            self.valueDownLength.hide()
            self.labelRiser.hide()
            self.valueRiser.hide()
            self.labelTread.hide()
            self.valueTread.hide()
        elif preset == "Panel":
            self.preview.load(":/ui/ParametersPanel.svg")
            self.labelSlabType.hide()
            self.valueSlabType.hide()
            self.labelChamfer.show()
            self.valueChamfer.show()
            self.labelDentLength.hide()
            self.valueDentLength.hide()
            self.labelDentWidth.show()
            self.valueDentWidth.show()
            self.labelDentHeight.show()
            self.valueDentHeight.show()
            self.labelBase.hide()
            self.valueBase.hide()
            self.labelHoleNumber.hide()
            self.valueHoleNumber.hide()
            self.labelHoleMajor.hide()
            self.valueHoleMajor.hide()
            self.labelHoleMinor.hide()
            self.valueHoleMinor.hide()
            self.labelHoleSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelGrooveNumber.hide()
            self.valueGrooveNumber.hide()
            self.labelGrooveDepth.hide()
            self.valueGrooveDepth.hide()
            self.labelGrooveHeight.hide()
            self.valueGrooveHeight.hide()
            self.labelGrooveSpacing.hide()
            self.valueGrooveSpacing.hide()
            self.labelRiserNumber.hide()
            self.valueRiserNumber.hide()
            self.labelDownLength.hide()
            self.valueDownLength.hide()
            self.labelRiser.hide()
            self.valueRiser.hide()
            self.labelTread.hide()
            self.valueTread.hide()
        elif preset == "Slab":
            self.preview.load(":/ui/ParametersSlab.svg")
            self.labelSlabType.show()
            self.valueSlabType.show()
            self.labelChamfer.hide()
            self.valueChamfer.hide()
            self.labelDentLength.hide()
            self.valueDentLength.hide()
            self.labelDentWidth.hide()
            self.valueDentWidth.hide()
            self.labelDentHeight.hide()
            self.valueDentHeight.hide()
            self.labelBase.show()
            self.valueBase.show()
            self.labelHoleNumber.show()
            self.valueHoleNumber.show()
            self.labelHoleMajor.show()
            self.valueHoleMajor.show()
            self.labelHoleMinor.show()
            self.valueHoleMinor.show()
            self.labelHoleSpacing.show()
            self.valueHoleSpacing.show()
            self.labelGrooveNumber.hide()
            self.valueGrooveNumber.hide()
            self.labelGrooveDepth.hide()
            self.valueGrooveDepth.hide()
            self.labelGrooveHeight.hide()
            self.valueGrooveHeight.hide()
            self.labelGrooveSpacing.hide()
            self.valueGrooveSpacing.hide()
            self.labelRiserNumber.hide()
            self.valueRiserNumber.hide()
            self.labelDownLength.hide()
            self.valueDownLength.hide()
            self.labelRiser.hide()
            self.valueRiser.hide()
            self.labelTread.hide()
            self.valueTread.hide()
        elif preset == "I-Beam":
            self.preview.load(":/ui/ParametersIbeam.svg")
            self.labelSlabType.hide()
            self.valueSlabType.hide()
            self.labelChamfer.show()
            self.valueChamfer.show()
            self.labelDentLength.hide()
            self.valueDentLength.hide()
            self.labelDentWidth.hide()
            self.valueDentWidth.hide()
            self.labelDentHeight.hide()
            self.valueDentHeight.hide()
            self.labelBase.show()
            self.valueBase.show()
            self.labelHoleNumber.hide()
            self.valueHoleNumber.hide()
            self.labelHoleMajor.hide()
            self.valueHoleMajor.hide()
            self.labelHoleMinor.hide()
            self.valueHoleMinor.hide()
            self.labelHoleSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelGrooveNumber.hide()
            self.valueGrooveNumber.hide()
            self.labelGrooveDepth.hide()
            self.valueGrooveDepth.hide()
            self.labelGrooveHeight.hide()
            self.valueGrooveHeight.hide()
            self.labelGrooveSpacing.hide()
            self.valueGrooveSpacing.hide()
            self.labelRiserNumber.hide()
            self.valueRiserNumber.hide()
            self.labelDownLength.hide()
            self.valueDownLength.hide()
            self.labelRiser.hide()
            self.valueRiser.hide()
            self.labelTread.hide()
            self.valueTread.hide()
        elif preset == "Stairs":
            self.preview.load(":/ui/ParametersStairs.svg")
            self.labelSlabType.hide()
            self.valueSlabType.hide()
            self.labelChamfer.hide()
            self.valueChamfer.hide()
            self.labelDentLength.hide()
            self.valueDentLength.hide()
            self.labelDentWidth.hide()
            self.valueDentWidth.hide()
            self.labelDentHeight.hide()
            self.valueDentHeight.hide()
            self.labelBase.hide()
            self.valueBase.hide()
            self.labelHoleNumber.hide()
            self.valueHoleNumber.hide()
            self.labelHoleMajor.hide()
            self.valueHoleMajor.hide()
            self.labelHoleMinor.hide()
            self.valueHoleMinor.hide()
            self.labelHoleSpacing.hide()
            self.valueHoleSpacing.hide()
            self.labelGrooveNumber.hide()
            self.valueGrooveNumber.hide()
            self.labelGrooveDepth.hide()
            self.valueGrooveDepth.hide()
            self.labelGrooveHeight.hide()
            self.valueGrooveHeight.hide()
            self.labelGrooveSpacing.hide()
            self.valueGrooveSpacing.hide()
            self.labelRiserNumber.show()
            self.valueRiserNumber.show()
            self.labelDownLength.show()
            self.valueDownLength.show()
            self.labelRiser.show()
            self.valueRiser.show()
            self.labelTread.show()
            self.valueTread.show()
        self.preview.show()


class _DentsTaskPanel:

    '''The TaskPanel for dent creation'''

    def __init__(self):

        import FreeCADGui
        from PySide import QtCore,QtGui,QtSvg
        self.form = QtGui.QWidget()
        self.grid = QtGui.QGridLayout(self.form)
        self.Rotations = ["N","S","E","O"]
        self.RotationAngles = [90,270,0,180]

        # dents list
        self.labelDents = QtGui.QLabel()
        self.listDents = QtGui.QListWidget()
        self.grid.addWidget(self.labelDents,0,0,1,2)
        self.grid.addWidget(self.listDents,1,0,1,2)

        # buttons
        self.buttonAdd = QtGui.QPushButton()
        self.buttonRemove = QtGui.QPushButton()
        self.grid.addWidget(self.buttonAdd,2,0,1,1)
        self.grid.addWidget(self.buttonRemove,2,1,1,1)

        # image display
        self.preview = QtSvg.QSvgWidget(":/ui/ParametersDent.svg")
        self.preview.setMaximumWidth(200)
        self.preview.setMinimumHeight(120)
        self.grid.addWidget(self.preview,3,0,1,2)

        # parameters
        self.labelLength = QtGui.QLabel()
        self.valueLength = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelLength,4,0,1,1)
        self.grid.addWidget(self.valueLength,4,1,1,1)

        self.labelWidth = QtGui.QLabel()
        self.valueWidth = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelWidth,5,0,1,1)
        self.grid.addWidget(self.valueWidth,5,1,1,1)

        self.labelHeight = QtGui.QLabel()
        self.valueHeight = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelHeight,6,0,1,1)
        self.grid.addWidget(self.valueHeight,6,1,1,1)

        self.labelSlant = QtGui.QLabel()
        self.valueSlant = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelSlant,7,0,1,1)
        self.grid.addWidget(self.valueSlant,7,1,1,1)

        self.labelLevel = QtGui.QLabel()
        self.valueLevel = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelLevel,8,0,1,1)
        self.grid.addWidget(self.valueLevel,8,1,1,1)

        self.labelRotation = QtGui.QLabel()
        self.valueRotation = QtGui.QComboBox()
        self.valueRotation.addItems(self.Rotations)
        self.valueRotation.setCurrentIndex(0)
        self.grid.addWidget(self.labelRotation,9,0,1,1)
        self.grid.addWidget(self.valueRotation,9,1,1,1)

        self.labelOffset = QtGui.QLabel()
        self.valueOffset = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.grid.addWidget(self.labelOffset,10,0,1,1)
        self.grid.addWidget(self.valueOffset,10,1,1,1)

        # signals/slots
        QtCore.QObject.connect(self.valueLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.valueWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.valueHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(self.valueSlant,QtCore.SIGNAL("valueChanged(double)"),self.setSlant)
        QtCore.QObject.connect(self.valueLevel,QtCore.SIGNAL("valueChanged(double)"),self.setLevel)
        QtCore.QObject.connect(self.valueRotation,QtCore.SIGNAL("currentIndexChanged(int)"),self.setDent)
        QtCore.QObject.connect(self.valueOffset,QtCore.SIGNAL("valueChanged(double)"),self.setOffset)
        QtCore.QObject.connect(self.buttonAdd,QtCore.SIGNAL("clicked()"),self.addDent)
        QtCore.QObject.connect(self.buttonRemove,QtCore.SIGNAL("clicked()"),self.removeDent)
        QtCore.QObject.connect(self.listDents,QtCore.SIGNAL("itemClicked(QListWidgetItem*)"),self.editDent)
        self.retranslateUi(self.form)
        self.form.hide()

    def setLength(self,value):
        self.Length = value
        self.setDent()

    def setWidth(self,value):
        self.Width = value
        self.setDent()

    def setHeight(self,value):
        self.Height = value
        self.setDent()

    def setSlant(self,value):
        self.Slant = value
        self.setDent()

    def setLevel(self,value):
        self.Level = value
        self.setDent()

    def setOffset(self,value):
        self.Offset = value
        self.setDent()

    def fillDents(self,dents):
        self.listDents.clear()
        i = 1
        for d in dents:
            s = "Dent "+str(i)+" :"+d
            self.listDents.addItem(s)
            i += 1

    def setDent(self,i=0):
        if self.listDents.currentItem():
            num = str(self.listDents.currentRow()+1)
            rot = self.RotationAngles[self.valueRotation.currentIndex()]
            s = "Dent "+num+" :"+str(self.Length)+";"+str(self.Width)+";"+str(self.Height)+";"+str(self.Slant)+";"+str(self.Level)+";"+str(rot)+";"+str(self.Offset)
            self.listDents.currentItem().setText(s)

    def addDent(self):
        num = str(self.listDents.count()+1)
        rot = self.RotationAngles[self.valueRotation.currentIndex()]
        s = "Dent "+num+" :"+str(self.Length)+";"+str(self.Width)+";"+str(self.Height)+";"+str(self.Slant)+";"+str(self.Level)+";"+str(rot)+";"+str(self.Offset)
        self.listDents.addItem(s)
        self.listDents.setCurrentRow(self.listDents.count()-1)
        self.editDent()

    def removeDent(self):
        if self.listDents.currentItem():
            self.listDents.takeItem(self.listDents.currentRow())

    def editDent(self,item=None):
        if self.listDents.currentItem():
            s = self.listDents.currentItem().text()
            s = s.split(":")[1]
            s = s.split(";")
            self.valueLength.setText(FreeCAD.Units.Quantity(float(s[0]),FreeCAD.Units.Length).UserString)
            self.valueWidth.setText(FreeCAD.Units.Quantity(float(s[1]),FreeCAD.Units.Length).UserString)
            self.valueHeight.setText(FreeCAD.Units.Quantity(float(s[2]),FreeCAD.Units.Length).UserString)
            self.valueSlant.setText(FreeCAD.Units.Quantity(float(s[3]),FreeCAD.Units.Length).UserString)
            self.valueLevel.setText(FreeCAD.Units.Quantity(float(s[4]),FreeCAD.Units.Length).UserString)
            self.valueRotation.setCurrentIndex(self.RotationAngles.index(int(s[5])))
            self.valueOffset.setText(FreeCAD.Units.Quantity(float(s[6]),FreeCAD.Units.Length).UserString)

    def retranslateUi(self, dialog):
        from PySide import QtGui
        self.form.setWindowTitle(QtGui.QApplication.translate("Arch", "Precast options", None))
        self.labelDents.setText(QtGui.QApplication.translate("Arch", "Dents list", None))
        self.buttonAdd.setText(QtGui.QApplication.translate("Arch", "Add dent", None))
        self.buttonRemove.setText(QtGui.QApplication.translate("Arch", "Remove dent", None))
        self.labelLength.setText(QtGui.QApplication.translate("Arch", "Length", None))
        self.labelWidth.setText(QtGui.QApplication.translate("Arch", "Width", None))
        self.labelHeight.setText(QtGui.QApplication.translate("Arch", "Height", None))
        self.labelSlant.setText(QtGui.QApplication.translate("Arch", "Slant", None))
        self.labelLevel.setText(QtGui.QApplication.translate("Arch", "Level", None))
        self.labelRotation.setText(QtGui.QApplication.translate("Arch", "Rotation", None))
        self.labelOffset.setText(QtGui.QApplication.translate("Arch", "Offset", None))

    def getValues(self):
        l = []
        for i in range(self.listDents.count()):
            s = self.listDents.item(i).text()
            l.append(s.split(":")[1])
        return l


def makePrecast(precasttype=None,length=0,width=0,height=0,slabtype="",chamfer=0,dentlength=0,dentwidth=0,dentheight=0,dents=[],base=0,holenumber=0,holemajor=0,holeminor=0,holespacing=0,groovenumber=0,groovedepth=0,grooveheight=0,groovespacing=0,risernumber=0,downlength=0,riser=0,tread=0):

    "Creates one of the precast objects in the current document"

    if precasttype == "Beam":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Beam")
        _PrecastBeam(obj)
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.Chamfer = chamfer
        obj.Dents = dents
        obj.DentLength = dentlength
        obj.DentHeight = dentheight
    elif precasttype == "Pillar":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Pillar")
        _PrecastPillar(obj)
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.Chamfer = chamfer
        obj.Dents = dents
        obj.GrooveNumber = groovenumber
        obj.GrooveDepth = groovedepth
        obj.GrooveHeight = grooveheight
        obj.GrooveSpacing = groovespacing
    elif precasttype == "Panel":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Panel")
        _PrecastPanel(obj)
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.Chamfer = chamfer
        obj.DentWidth = dentwidth
        obj.DentHeight = dentheight
    elif precasttype == "Slab":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Slab")
        _PrecastSlab(obj)
        obj.SlabType = slabtype
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.SlabBase = base
        obj.HoleNumber = holenumber
        obj.HoleMajor = holemajor
        obj.HoleMinor = holeminor
        obj.HoleSpacing = holespacing
    elif precasttype == "I-Beam":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Beam")
        _PrecastIbeam(obj)
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.Chamfer = chamfer
        obj.BeamBase = base
    elif precasttype == "Stairs":
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Stairs")
        _PrecastStairs(obj)
        obj.Length = length
        obj.Width = width
        obj.Height = height
        obj.RiserNumber = risernumber
        obj.DownLength = downlength
        obj.Riser = riser
        obj.Tread = tread
    else:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Precast")
        _Precast(obj)
    if FreeCAD.GuiUp:
        _ViewProviderPrecast(obj.ViewObject)
    return obj
