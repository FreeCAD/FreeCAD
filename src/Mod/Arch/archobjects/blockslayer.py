#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

"""This module provides tools to build Wall objects.  Walls are simple
objects, usually vertical, typically obtained by giving a thickness to a base
line, then extruding it vertically.

Examples
--------
TODO put examples here.

"""

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands,math
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import draftguitools.gui_trackers as DraftTrackers
else:
    # \cond
    def translate(ctxt,txt, utf8_decode=False):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

## @package ArchWall
#  \ingroup ARCH
#  \brief The Wall object and tools
#
#  This module provides tools to build Wall objects.  Walls are simple objects,
#  usually vertical, typically obtained by giving a thickness to a base line,
#  then extruding it vertically.

__title__="FreeCAD Wall"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


class BlocksLayer(ArchComponent.Component):
    """The BlocksLayer object. 

    Turns a <App::FeaturePython> into a wall object, then uses a
    <Part::Feature> to create the wall's shape.

    Walls are simple objects, usually vertical, typically obtained by giving a
    thickness to a base line, then extruding it vertically.

    Parameters
    ----------
    obj: <App::FeaturePython>
        The object to turn into a wall. Note that this is not the object that
        forms the basis for the new wall's shape. That is given later.
    """

    def __init__(self, obj):
        ArchComponent.Component.__init__(self, obj)
        self.setProperties(obj)
        obj.IfcType = "Wall"

    def setProperties(self, obj):
        """Give the wall its wall specific properties, such as its alignment.

        You can learn more about properties here:
        https://wiki.freecadweb.org/property

        parameters
        ----------
        obj: <part::featurepython>
            The object to turn into a wall.
        """

        lp = obj.PropertiesList
        if not "Length" in lp:
            obj.addProperty("App::PropertyLength","Length","Wall",QT_TRANSLATE_NOOP("App::Property","The length of this wall. Not used if this wall is based on an underlying object"))
        
        if not "Width" in lp:
            obj.addProperty("App::PropertyLength","Width","Wall",QT_TRANSLATE_NOOP("App::Property","The width of this wall. Not used if this wall is based on a face"))

        if not "Height" in lp:
            obj.addProperty("App::PropertyLength","Height","Wall",QT_TRANSLATE_NOOP("App::Property","The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid"))

        if not "MakeBlocks" in lp:
            obj.addProperty("App::PropertyBool","MakeBlocks","Blocks",QT_TRANSLATE_NOOP("App::Property","Enable this to make the wall generate blocks"))
        if not "BlockLength" in lp:
            obj.addProperty("App::PropertyLength","BlockLength","Blocks",QT_TRANSLATE_NOOP("App::Property","The length of each block"))
        if not "BlockHeight" in lp:
            obj.addProperty("App::PropertyLength","BlockHeight","Blocks",QT_TRANSLATE_NOOP("App::Property","The height of each block"))
        if not "OffsetFirst" in lp:
            obj.addProperty("App::PropertyLength","OffsetFirst","Blocks",QT_TRANSLATE_NOOP("App::Property","The horizontal offset of the first line of blocks"))
        if not "OffsetSecond" in lp:
            obj.addProperty("App::PropertyLength","OffsetSecond","Blocks",QT_TRANSLATE_NOOP("App::Property","The horizontal offset of the second line of blocks"))
        if not "Joint" in lp:
            obj.addProperty("App::PropertyLength","Joint","Blocks",QT_TRANSLATE_NOOP("App::Property","The size of the joints between each block"))
        if not "CountEntire" in lp:
            obj.addProperty("App::PropertyInteger","CountEntire","Blocks",QT_TRANSLATE_NOOP("App::Property","The number of entire blocks"))
            obj.setEditorMode("CountEntire",1)
        if not "CountBroken" in lp:
            obj.addProperty("App::PropertyInteger","CountBroken","Blocks",QT_TRANSLATE_NOOP("App::Property","The number of broken blocks"))
            obj.setEditorMode("CountBroken",1)

        self.Type = "BlocksLayer"

    def onDocumentRestored(self,obj):
        """Method run when the document is restored. Re-adds the Arch component, and Arch wall properties."""

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def execute(self,obj):
        """Method run when the object is recomputed.

        Extrude the wall from the Base shape if possible. Processe additions
        and subtractions. Assign the resulting shape as the shape of the wall.

        Add blocks if the MakeBlocks property is assigned. If the Base shape is
        a mesh, just copy the mesh.
        """

        if self.clone(obj):
            return

        import Part, DraftGeomUtils
        base = None
        pl = obj.Placement
        extdata = self.getExtrusionData(obj)

        if obj.Base.Shape.isNull():
            return
        if not obj.Base.Shape.isValid():
            if not obj.Base.Shape.Solids:
                # let pass invalid objects if they have solids...
                return
        elif obj.Base.Shape.Solids:
            base = obj.Base.Shape.copy()

        # blocks calculation
        if hasattr(obj,"MakeBlocks") and hasattr(self,"basewires"):
            if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                #print "calculating blocks"
                if len(self.basewires) == 1:
                    blocks = []
                    n = App.Vector(extv)
                    n.normalize()
                    cuts1 = []
                    cuts2 = []
                    if obj.BlockLength.Value:
                        for i in range(2):
                            if i == 0:
                                offset = obj.OffsetFirst.Value
                            else:
                                offset = obj.OffsetSecond.Value
                            for edge in self.basewires[0].Edges:
                                while offset < (edge.Length-obj.Joint.Value):
                                    #print i," Edge ",edge," : ",edge.Length," - ",offset
                                    if offset:
                                        t = edge.tangentAt(offset)
                                        p = t.cross(n)
                                        p.multiply(1.1*obj.Width.Value+obj.Offset.Value)
                                        p1 = edge.valueAt(offset).add(p)
                                        p2 = edge.valueAt(offset).add(p.negative())
                                        sh = Part.LineSegment(p1,p2).toShape()
                                        if obj.Joint.Value:
                                            sh = sh.extrude(t.multiply(obj.Joint.Value))
                                        sh = sh.extrude(n)
                                        if i == 0:
                                            cuts1.append(sh)
                                        else:
                                            cuts2.append(sh)
                                    offset += (obj.BlockLength.Value + obj.Joint.Value)
                                else:
                                    offset -= (edge.Length - obj.Joint.Value)

                    if isinstance(bplates,list):
                        bplates = bplates[0]
                    if obj.BlockHeight.Value:
                        fsize = obj.BlockHeight.Value + obj.Joint.Value
                        bh = obj.BlockHeight.Value
                    else:
                        fsize = obj.Height.Value
                        bh = obj.Height.Value
                    bvec = FreeCAD.Vector(n)
                    bvec.multiply(bh)
                    svec = FreeCAD.Vector(n)
                    svec.multiply(fsize)
                    if cuts1:
                        plate1 = bplates.cut(cuts1).Faces
                    else:
                        plate1 = bplates.Faces
                    blocks1 = Part.makeCompound([f.extrude(bvec) for f in plate1])
                    if cuts2:
                        plate2 = bplates.cut(cuts2).Faces
                    else:
                        plate2 = bplates.Faces
                    blocks2 = Part.makeCompound([f.extrude(bvec) for f in plate2])
                    interval = extv.Length/(fsize)
                    entires = int(interval)
                    rest = (interval - entires)
                    for i in range(entires):
                        if i % 2: # odd
                            b = blocks2.copy()
                        else:
                            b = blocks1.copy()
                        if i:
                            t = FreeCAD.Vector(svec)
                            t.multiply(i)
                            b.translate(t)
                        blocks.append(b)
                    if rest:
                        rest = extv.Length-(entires*fsize)
                        rvec = FreeCAD.Vector(n)
                        rvec.multiply(rest)
                        if entires % 2:
                            b = Part.makeCompound([f.extrude(rvec) for f in plate2])
                        else:
                            b = Part.makeCompound([f.extrude(rvec) for f in plate1])
                        t = FreeCAD.Vector(svec)
                        t.multiply(entires)
                        b.translate(t)
                        blocks.append(b)
                    if blocks:
                        base = Part.makeCompound(blocks)

                else:
                    FreeCAD.Console.PrintWarning(translate("Arch","Cannot compute blocks for wall")+obj.Label+"\n")


        base = self.processSubShapes(obj,base,pl)

        self.applyShape(obj,base,pl)

        # count blocks
        if hasattr(obj,"MakeBlocks"):
            if obj.MakeBlocks:
                fvol = obj.BlockLength.Value * obj.BlockHeight.Value * obj.Width.Value
                if fvol:
                    #print("base volume:",fvol)
                    #for s in base.Solids:
                        #print(abs(s.Volume - fvol))
                    ents = [s for s in base.Solids if abs(s.Volume - fvol) < 1]
                    obj.CountEntire = len(ents)
                    obj.CountBroken = len(base.Solids) - len(ents)
                else:
                    obj.CountEntire = 0
                    obj.CountBroken = 0

        # set the length property
        if obj.Base:
            if hasattr(obj.Base,'Shape'):
                if obj.Base.Shape.Edges:
                    if not obj.Base.Shape.Faces:
                        if hasattr(obj.Base.Shape,"Length"):
                            l = obj.Base.Shape.Length
                            if obj.Length.Value != l:
                                obj.Length = l
                                self.oldLength = None # delete the stored value to prevent triggering base change below

        # set the Area property
        obj.Area = obj.Length.Value * obj.Height.Value

    def onBeforeChange(self,obj,prop):
        """Method called before the object has a property changed. 

        Specifically, this method is called before the value changes.

        If "Length" has changed, record the old length so that .onChanged() can
        be sure that the base needs to be changed.

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "Length":
            self.oldLength = obj.Length.Value

    def onChanged(self, obj, prop):
        """Method called when the object has a property changed.

        If length has changed, extend the length of the Base object, if the
        Base object only has a single edge to extend.

        Also hide subobjects.

        Also call ArchComponent.Component.onChanged().

        Parameters
        ----------
        prop: string
            The name of the property that has changed.
        """

        if prop == "Length":
            if (obj.Base and obj.Length.Value 
                    and hasattr(self,"oldLength") and (self.oldLength is not None) 
                    and (self.oldLength != obj.Length.Value)):

                if hasattr(obj.Base,'Shape'):
                    if len(obj.Base.Shape.Edges) == 1:
                        import DraftGeomUtils
                        e = obj.Base.Shape.Edges[0]
                        if DraftGeomUtils.geomType(e) == "Line":
                            if e.Length != obj.Length.Value:
                                v = e.Vertexes[-1].Point.sub(e.Vertexes[0].Point)
                                v.normalize()
                                v.multiply(obj.Length.Value)
                                p2 = e.Vertexes[0].Point.add(v)
                                if Draft.getType(obj.Base) == "Wire":
                                    #print "modifying p2"
                                    obj.Base.End = p2
                                elif Draft.getType(obj.Base) == "Sketch":
                                    try:
                                        obj.Base.movePoint(0,2,p2,0)
                                    except:
                                        print("Debug: The base sketch of this wall could not be changed, because the sketch has not been edited yet in this session (this is a bug in FreeCAD). Try entering and exiting edit mode in this sketch first, and then changing the wall length should work.")
                                else:
                                    FreeCAD.Console.PrintError(translate("Arch","Error: Unable to modify the base object of this wall")+"\n")

        self.hideSubobjects(obj,prop)
        ArchComponent.Component.onChanged(self,obj,prop)
