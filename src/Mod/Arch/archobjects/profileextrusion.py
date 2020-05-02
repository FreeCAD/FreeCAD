#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 Carlo Pavan                                        *
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
"""Provide the object code for Arch Profile Extrusion object."""
## @package wall
# \ingroup ARCH
# \brief Provide the object code for Arch Profile Extrusion object. This
#        was part of the former Arch Wall, and was split to separate the 
#        geometry object from its semantic meaning. (TO BE COMPLETED)

import FreeCAD as App

import Part
import DraftGeomUtils, DraftVecUtils

from PySide.QtCore import QT_TRANSLATE_NOOP


def make_profile_extrusion(obj, width, height):
    """
    """
    pass


class ProfileExtrusion:

    "The Profile Extrusion Arch object"

    def __init__(self, obj):

        self.setProperties(obj)

        #obj.IfcType = "Wall"


    def setProperties(self, obj):

        lp = obj.PropertiesList

        # general properties
        if not "Width" in lp:
            obj.addProperty("App::PropertyLength","Width","Geometry",QT_TRANSLATE_NOOP("App::Property","The width of this wall. Not used if this wall is based on a face"))
        
        if not "Height" in lp:
            obj.addProperty("App::PropertyLength","Height","Geometry",QT_TRANSLATE_NOOP("App::Property","The height of this wall. Keep 0 for automatic. Not used if this wall is based on a solid"))

        if not "Align" in lp:
            obj.addProperty("App::PropertyEnumeration","Align","Geometry",QT_TRANSLATE_NOOP("App::Property","The alignment of this wall on its base object, if applicable"))
            obj.Align = ['Left','Right','Center']

        if not "Offset" in lp:
            obj.addProperty("App::PropertyDistance","Offset","Geometry",QT_TRANSLATE_NOOP("App::Property","The offset between this wall and its baseline (only for left and right alignments)"))

        if not "Normal" in lp:
            obj.addProperty("App::PropertyVector","Normal","Geometry",QT_TRANSLATE_NOOP("App::Property","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))

        if not "Area" in lp:
            obj.addProperty("App::PropertyArea","Area","Geometry",QT_TRANSLATE_NOOP("App::Property","The area of this wall as a simple Height * Length calculation"))
            obj.setEditorMode("Area", 1)

        # Segment individual properties
        if not "OverrideWidth" in lp: # To be combined into Width when PropertyLengthList is available
            _tip = "Override Width attribute to set width for each segment of the extrusion.\n" \
                   "Ignored if Base object provides Widths information, with getWidths() method.\n"\
                   "(The 1st value override 'Width' attribute for 1st segment of wall;\n" \
                   "if a value is zero, 1st value of 'OverrideWidth' will be followed)"
            obj.addProperty("App::PropertyFloatList","OverrideWidth",
                            "Segment individual properties",
                            QT_TRANSLATE_NOOP("App::Property", _tip))
            # see DraftGeomUtils.offsetwire()

        if not "OverrideAlign" in lp:
            _tip = "Override Align attribute to set Align for each segment of the extrusion.\n"\
                   "Ignored if Base object provides Aligns information, with getAligns() method.\n"\
                   "(The 1st value override 'Align' attribute for 1st segment of wall;\n"\
                   "if a value is not 'Left, Right, Center', 1st value of 'OverrideAlign'\n"\
                   "will be followed)"
            obj.addProperty("App::PropertyStringList", "OverrideAlign",
                            "Segment individual properties",
                            QT_TRANSLATE_NOOP("App::Property", _tip))
            # see DraftGeomUtils.offsetwire()

        self.Type = "Wall"


    def onDocumentRestored(self, obj):
        self.setProperties(obj)


    def execute(self, obj):

        "builds the wall shape"

        if self.clone(obj):
            return

        base = None
        pl = obj.Placement
        extdata = self.getExtrusionData(obj)
        if extdata:
            bplates = extdata[0]
            extv = extdata[2].Rotation.multVec(extdata[1])
            if isinstance(bplates,list):
                shps = []
                # Test : if base is Sketch, then fuse all solid; otherwise, makeCompound
                sketchBaseToFuse = obj.Base.getLinkedObject().isDerivedFrom("Sketcher::SketchObject")
                # but turn this off if we have layers, otherwise layers get merged
                if hasattr(obj,"Material") and obj.Material \
                and hasattr(obj.Material,"Materials") and obj.Material.Materials:
                    sketchBaseToFuse = False
                for b in bplates:
                    b.Placement = extdata[2].multiply(b.Placement)
                    b = b.extrude(extv)

                    # See getExtrusionData() - not fusing baseplates there - fuse solids here
                    # Remarks - If solids are fused, but exportIFC.py use underlying baseplates w/o fuse, the result in ifc look slightly different from in FC.

                    if sketchBaseToFuse:
                        if shps:
                            shps = shps.fuse(b) #shps.fuse(b)
                        else:
                            shps=b
                    else:
                        shps.append(b)
                    # TODO - To let user to select whether to fuse (slower) or to do a compound (faster) only ?

                if sketchBaseToFuse:
                    base = shps
                else:
                    base = Part.makeCompound(shps)
            else:
                bplates.Placement = extdata[2].multiply(bplates.Placement)
                base = bplates.extrude(extv)

        if obj.Base:

            if not hasattr(obj.Base,'Shape'):
                return

            if obj.Base.Shape.isNull():
                return

            if not obj.Base.Shape.isValid():
                if not obj.Base.Shape.Solids:
                    # let pass invalid objects if they have solids...
                    return
            elif obj.Base.Shape.Solids:
                base = obj.Base.Shape.copy()

            # blocks calculation
            elif hasattr(obj,"MakeBlocks") and hasattr(self,"basewires"):
                if obj.MakeBlocks and self.basewires and extdata and obj.Width and obj.Height:
                    #print("calculating blocks")
                    self.make_blocks(obj, extdata)

        if not base:
            App.Console.PrintError(translate("Arch","Error: Invalid base object")+"\n")
            return

        base = self.processSubShapes(obj,base,pl)

        self.applyShape(obj,base,pl)

        # count blocks
        if hasattr(obj,"MakeBlocks"):
            if obj.MakeBlocks:
                self.count_blocks(obj)

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


    def make_blocks(self, obj):
        """this was split from execute()"""

        if len(self.basewires) != 1:
            App.Console.PrintWarning(translate("Arch","Cannot compute blocks for wall")+obj.Label+"\n")
            return

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
        bvec = App.Vector(n)
        bvec.multiply(bh)
        svec = App.Vector(n)
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
                t = App.Vector(svec)
                t.multiply(i)
                b.translate(t)
            blocks.append(b)

        if rest:
            rest = extv.Length-(entires*fsize)
            rvec = App.Vector(n)
            rvec.multiply(rest)
            if entires % 2:
                b = Part.makeCompound([f.extrude(rvec) for f in plate2])
            else:
                b = Part.makeCompound([f.extrude(rvec) for f in plate1])
            t = App.Vector(svec)
            t.multiply(entires)
            b.translate(t)
            blocks.append(b)

        if blocks:
            base = Part.makeCompound(blocks)


    def count_blocks(self, obj):
        """returns the number of blocks inside the profile extrusion"""
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


    def onBeforeChange(self, obj, prop):
        if prop == "Length":
            self.oldLength = obj.Length.Value


    def onChanged(self, obj, prop):
        if prop == "Length":
            if obj.Base and obj.Length.Value and hasattr(self,"oldLength") and (self.oldLength != None) and (self.oldLength != obj.Length.Value):
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
                                    App.Console.PrintError(translate("Arch","Error: Unable to modify the base object of this wall")+"\n")
        self.hideSubobjects(obj,prop)
        ArchComponent.Component.onChanged(self,obj,prop)


    def getFootprint(self,obj):

        faces = []
        if obj.Shape:
            for f in obj.Shape.Faces:
                if f.normalAt(0,0).getAngle(App.Vector(0,0,-1)) < 0.01:
                    if abs(abs(f.CenterOfMass.z) - abs(obj.Shape.BoundBox.ZMin)) < 0.001:
                        faces.append(f)
        return faces


    def getExtrusionData(self, obj):

        """returns (shape, extrusion vector, placement) or None"""
        import Part,DraftGeomUtils
        data = ArchComponent.Component.getExtrusionData(self,obj)
        if data:
            if not isinstance(data[0],list):
                # multifuses not considered here
                return data
        length  = obj.Length.Value

        # TODO currently layers were not supported when len(basewires) > 0	##( or 1 ? )
        width = 0

        # Get width of each edge segment from Base Objects if they store it (Adding support in SketchFeaturePython, DWire...)
        widths = []  # [] or None are both False
        if obj.Base:
            if hasattr(obj.Base, 'Proxy'):
                if hasattr(obj.Base.Proxy, 'getWidths'):
                    widths = obj.Base.Proxy.getWidths(obj.Base)  # return a list of Width corresponds to indexes of sorted edges of Sketch

        # Get width of each edge/wall segment from ArchWall.OverrideWidth if Base Object does not provide it

        if not widths:

            if obj.OverrideWidth:
                if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when ArchSketch.getWidth() not implemented yet):-
                    # sort the width list in OverrrideWidth to correspond to indexes of sorted edges of Sketch
                    try:
                        import ArchSketchObject
                    except:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        widths = ArchSketchObject.sortSketchWidth(obj.Base, obj.OverrideWidth)
                    except:
                        widths = obj.OverrideWidth
                else:
                    # If Base Object is not Sketch, but e.g. DWire, the width list in OverrrideWidth just correspond to sequential order of edges
                    widths = obj.OverrideWidth
            elif obj.Width:
                widths = [obj.Width.Value]
            else:
                print ("Width & OverrideWidth & base.getWidths() should not be all 0 or None or [] empty list ")
                return None

        # set 'default' width - for filling in any item in the list == 0 or None
        if obj.Width.Value:
            width = obj.Width.Value
        else:
            width = 200  # 'Default' width value

        # Get align of each edge segment from Base Objects if they store it (Adding support in SketchFeaturePython, DWire...)
        aligns = []
        if obj.Base:
            if hasattr(obj.Base, 'Proxy'):
                if hasattr(obj.Base.Proxy, 'getAligns'):
                    aligns = obj.Base.Proxy.getAligns(obj.Base)  # return a list of Align corresponds to indexes of sorted edges of Sketch

        # Get align of each edge/wall segment from ArchWall.OverrideAlign if Base Object does not provide it
        if not aligns:
            if obj.OverrideAlign:
                if obj.Base.isDerivedFrom("Sketcher::SketchObject"):
                    # If Base Object is ordinary Sketch (or when ArchSketch.getAligns() not implemented yet):-
                    # sort the align list in OverrideAlign to correspond to indexes of sorted edges of Sketch
                    try:
                        import ArchSketchObject
                    except:
                        print("ArchSketchObject add-on module is not installed yet")
                    try:
                        aligns = ArchSketchObject.sortSketchAlign(obj.Base, obj.OverrideAlign)
                    except:
                        aligns = obj.OverrideAlign
                else:
                    # If Base Object is not Sketch, but e.g. DWire, the align list in OverrideAlign just correspond to sequential order of edges
                    aligns = obj.OverrideAlign
            else:
                aligns = [obj.Align]

        # set 'default' align - for filling in any item in the list == 0 or None
        align = obj.Align  # or aligns[0]

        height = obj.Height.Value
        if not height:
            height = self.getParentHeight(obj)
        if not height:
            return None
        if obj.Normal == Vector(0,0,0):
            normal = Vector(0,0,1)
        else:
            normal = Vector(obj.Normal)
        base = None
        placement = None
        self.basewires = None
        # build wall layers
        layers = []
        if hasattr(obj,"Material"):
            if obj.Material:
                if hasattr(obj.Material,"Materials"):
                    thicknesses = [abs(t) for t in obj.Material.Thicknesses]
                    # multimaterials
                    varwidth = 0
                    restwidth = width - sum(thicknesses)
                    if restwidth > 0:
                        varwidth = [t for t in thicknesses if t == 0]
                        if varwidth:
                            varwidth = restwidth/len(varwidth)
                    for t in obj.Material.Thicknesses:
                        if t:
                            layers.append(t)
                        elif varwidth:
                            layers.append(varwidth)
        if obj.Base:
            self.get_extrusion_data_from_base(obj)

        else:
            if layers:
                totalwidth = sum([abs(l) for l in layers])
                offset = 0
                base = []
                for l in layers:
                    if l > 0:
                        l2 = length/2 or 0.5
                        w1 = -totalwidth/2 + offset
                        w2 = w1 + l
                        v1 = Vector(-l2,w1,0)
                        v2 = Vector(l2,w1,0)
                        v3 = Vector(l2,w2,0)
                        v4 = Vector(-l2,w2,0)
                        base.append(Part.Face(Part.makePolygon([v1,v2,v3,v4,v1])))
                    offset += abs(l)
            else:
                l2 = length/2 or 0.5
                w2 = width/2 or 0.5
                v1 = Vector(-l2,-w2,0)
                v2 = Vector(l2,-w2,0)
                v3 = Vector(l2,w2,0)
                v4 = Vector(-l2,w2,0)
                base = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
            placement = App.Placement()
        if base and placement:
            extrusion = normal.multiply(height)
            if placement.Rotation.Angle > 0:
                extrusion = placement.inverse().Rotation.multVec(extrusion)
            return (base,extrusion,placement)

        return None

    def get_extrusion_data_from_base(self, obj, layers):
        """ get extrusion data from base object """
        if not hasattr(obj.Base, 'Shape'):
            return None

        if obj.Base.Shape:
            if obj.Base.Shape.Solids:
                return None

        # COLLECT EDGES IN self.basewires

        if len(obj.Base.Shape.Edges) == 1:
            self.basewires = [Part.Wire(obj.Base.Shape.Edges)]

        # Sort Sketch edges consistently with below procedures without using Sketch.Shape.Edges - found the latter order in some corner case != getSortedClusters()
        elif obj.Base.isDerivedFrom("Sketcher::SketchObject"):
            self.basewires = []
            skGeom = obj.Base.Geometry
            skGeomEdges = []
            skPlacement = obj.Base.Placement  # Get Sketch's placement to restore later
            for i in skGeom:
                if not i.Construction:
                    skGeomEdgesI = i.toShape()
                    skGeomEdges.append(skGeomEdgesI)
            for cluster in Part.getSortedClusters(skGeomEdges):
                clusterTransformed = []
                for edge in cluster:
                    edge.Placement = edge.Placement.multiply(skPlacement)  ## TODO add attribute to skip Transform...
                    clusterTransformed.append(edge)
                self.basewires.append(clusterTransformed)  # Only use cluster of edges rather than turning into wire
            # Use Sketch's Normal for all edges/wires generated from sketch for consistency
            # Discussion on checking normal of sketch.Placement vs sketch.getGlobalPlacement() - https://forum.freecadweb.org/viewtopic.php?f=22&t=39341&p=334275#p334275
            # normal = obj.Base.Placement.Rotation.multVec(App.Vector(0,0,1))
            normal = obj.Base.getGlobalPlacement().Rotation.multVec(App.Vector(0,0,1))

        else:
            # in every other case, try to sort edges and collect them
            # self.basewires = obj.Base.Shape.Wires
            self.basewires = []
            for cluster in Part.getSortedClusters(obj.Base.Shape.Edges):
                for c in Part.sortEdges(cluster):
                    self.basewires.append(Part.Wire(c))
            # if not sketch, e.g. Dwire, can have wire which is 3d so not on the placement's working plane - below applied to Sketch not applicable here
            #normal = obj.Base.getGlobalPlacement().Rotation.multVec(App.Vector(0,0,1))  #normal = obj.Base.Placement.Rotation.multVec(App.Vector(0,0,1))

        # PROCEED WITH THE FUNCTION

        if not self.basewires: # and width: # width already tested earlier...
            return None

        if (len(self.basewires) == 1) and layers:
            self.basewires = [self.basewires[0] for l in layers]

        layeroffset = 0
        baseface = None

        for i,wire in enumerate(self.basewires):

            # Check number of edges per 'wire' and get the 1st edge
            if isinstance(wire,Part.Wire):
                edgeNum = len(wire.Edges)
                e = wire.Edges[0]
            elif isinstance(wire[0],Part.Edge):
                edgeNum = len(wire)
                e = wire[0]

            for n in range(0,edgeNum,1):  # why these not work - range(edgeNum), range(0,edgeNum) ...

                # Fill the aligns list with ArchWall's default align entry and with same number of items as number of edges
                try:
                    if aligns[n] not in ['Left', 'Right', 'Center']:
                        aligns[n] = align
                except:
                    aligns.append(align)

                # Fill the widths List with ArchWall's default width entry and with same number of items as number of edges
                try:
                    if not widths[n]:
                        widths[n] = width
                except:
                    widths.append(width)

            if isinstance(e.Curve,Part.Circle):
                dvec = e.Vertexes[0].Point.sub(e.Curve.Center)
            else:
                #dvec = DraftGeomUtils.vec(wire.Edges[0]).cross(normal)
                dvec = DraftGeomUtils.vec(e).cross(normal)

            if not DraftVecUtils.isNull(dvec):
                dvec.normalize()
            sh = None

            curAligns = aligns[0]
            if curAligns == "Left":
                off = obj.Offset.Value
                if layers:
                    off = off+layeroffset
                    dvec.multiply(abs(layers[i]))
                    layeroffset += abs(layers[i])
                else:
                    dvec.multiply(width)

                # Now DraftGeomUtils.offsetWire() support similar effect as ArchWall Offset
                #
                #if off:
                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)

                # Get the 'offseted' wire taking into account of Width and Align of each edge, and overall Offset
                w2 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,None,aligns,normal,off)

                # Get the 'base' wire taking into account of width and align of each edge
                w1 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,"BasewireMode",aligns,normal,off)
                sh = DraftGeomUtils.bind(w1,w2)

            elif curAligns == "Right":
                dvec = dvec.negative()
                off = obj.Offset.Value
                if layers:
                    off = off+layeroffset
                    dvec.multiply(abs(layers[i]))
                    layeroffset += abs(layers[i])
                else:
                    dvec.multiply(width)

                # Now DraftGeomUtils.offsetWire() support similar effect as ArchWall Offset
                #
                #if off:
                #    dvec2 = DraftVecUtils.scaleTo(dvec,off)
                #    wire = DraftGeomUtils.offsetWire(wire,dvec2)

                w2 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,None,aligns,normal,off)
                w1 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,"BasewireMode",aligns,normal,off)
                sh = DraftGeomUtils.bind(w1,w2)

            #elif obj.Align == "Center":
            elif curAligns == "Center":
                if layers:
                    off = width/2-layeroffset
                    d1 = Vector(dvec).multiply(off)
                    w1 = DraftGeomUtils.offsetWire(wire,d1)
                    layeroffset += abs(layers[i])
                    off = width/2-layeroffset
                    d1 = Vector(dvec).multiply(off)
                    w2 = DraftGeomUtils.offsetWire(wire,d1)
                else:
                    dvec.multiply(width)
                    w2 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,None,aligns,normal)
                    w1 = DraftGeomUtils.offsetWire(wire,dvec,False,False,widths,"BasewireMode",aligns,normal)
                sh = DraftGeomUtils.bind(w1,w2)

            del widths[0:edgeNum]
            del aligns[0:edgeNum]
            if sh:

                sh.fix(0.1,0,1) # fixes self-intersecting wires

                f = Part.Face(sh)
                if baseface:

                    # To allow exportIFC.py to work properly on sketch, which use only 1st face / wire, do not fuse baseface here
                    # So for a sketch with multiple wires, each returns individual face (rather than fusing together) for exportIFC.py to work properly
                    # "ArchWall - Based on Sketch Issues" - https://forum.freecadweb.org/viewtopic.php?f=39&t=31235

                    # "Bug #2408: [PartDesign] .fuse is splitting edges it should not"
                    # - https://forum.freecadweb.org/viewtopic.php?f=10&t=20349&p=346237#p346237
                    # - bugtracker - https://freecadweb.org/tracker/view.php?id=2408

                    # Try Part.Shell before removeSplitter
                    # - https://forum.freecadweb.org/viewtopic.php?f=10&t=20349&start=10
                    # - 1st finding : if a rectangle + 1 line, can't removesSplitter properly...
                    # - 2nd finding : if 2 faces do not touch, can't form a shell; then, subsequently for remaining faces even though touch each faces, can't form a shell

                    baseface.append(f)
                    # The above make Refine methods below (in else) useless, regardless removeSpitters yet to be improved for cases do not work well
                    '''  Whether layers or not, all baseface.append(f) '''

                else:
                    baseface = [f]

                    '''  Whether layers or not, all baseface = [f] '''