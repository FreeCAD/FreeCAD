# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__= "FreeCAD Arch Stairs"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"

## @package ArchStairs
#  \ingroup ARCH
#  \brief The Stairs object and tools
#
#  This module provides tools to build Stairs objects.

import math

import FreeCAD
import ArchComponent
import ArchPipe
import Draft
import DraftVecUtils
import DraftGeomUtils
import Part

from FreeCAD import Vector
from draftutils import params

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

zeroMM = FreeCAD.Units.Quantity('0mm')


class _Stairs(ArchComponent.Component):

    "A stairs object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.Type = "Stairs"
        self.setProperties(obj)
        obj.IfcType = "Stair"

    def setProperties(self,obj):

        # http://en.wikipedia.org/wiki/Stairs

        pl = obj.PropertiesList

        # base properties
        if not "Length" in pl:
            obj.addProperty("App::PropertyLength","Length","Stairs",QT_TRANSLATE_NOOP("App::Property","The length of these stairs, if no baseline is defined"), locked=True)
        if not "Width" in pl:
            obj.addProperty("App::PropertyLength","Width","Stairs",QT_TRANSLATE_NOOP("App::Property","The width of these stairs"), locked=True)
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","Stairs",QT_TRANSLATE_NOOP("App::Property","The total height of these stairs"), locked=True)
        if not "Align" in pl:
            obj.addProperty("App::PropertyEnumeration","Align","Stairs",QT_TRANSLATE_NOOP("App::Property","The alignment of these stairs on their baseline, if applicable"), locked=True)
            obj.Align = ['Left','Right','Center']

        # TODO - To be combined into Width when PropertyLengthList is available
        if not "WidthOfLanding" in pl:
            obj.addProperty("App::PropertyFloatList","WidthOfLanding","Stairs",QT_TRANSLATE_NOOP("App::Property","The width of a Landing (Second edge and after - First edge follows Width property)"), locked=True)

        # steps and risers properties

        if not "NumberOfSteps" in pl:
            obj.addProperty("App::PropertyInteger","NumberOfSteps","Steps",QT_TRANSLATE_NOOP("App::Property","The number of risers in these stairs"), locked=True)
        if not "TreadDepth" in pl:
            obj.addProperty("App::PropertyLength","TreadDepth","Steps",QT_TRANSLATE_NOOP("App::Property","The depth of the treads of these stairs"), locked=True)
            obj.setEditorMode("TreadDepth",1)
        if not "RiserHeight" in pl:
            obj.addProperty("App::PropertyLength","RiserHeight","Steps",QT_TRANSLATE_NOOP("App::Property","The height of the risers of these stairs"), locked=True)
            obj.setEditorMode("RiserHeight",1)
        if not "Nosing" in pl:
            obj.addProperty("App::PropertyLength","Nosing","Steps",QT_TRANSLATE_NOOP("App::Property","The size of the nosing"), locked=True)
        if not "TreadThickness" in pl:
            obj.addProperty("App::PropertyLength","TreadThickness","Steps",QT_TRANSLATE_NOOP("App::Property","The thickness of the treads"), locked=True)
        if not "BlondelRatio" in pl:
            obj.addProperty("App::PropertyFloat","BlondelRatio","Steps",QT_TRANSLATE_NOOP("App::Property","The Blondel ratio indicates comfortable stairs and should be between 62 and 64cm or 24.5 and 25.5in"), locked=True)
            obj.setEditorMode("BlondelRatio",1)

        if not "RiserThickness" in pl:
            obj.addProperty("App::PropertyLength","RiserThickness","Steps",QT_TRANSLATE_NOOP("App::Property","The thickness of the risers"), locked=True)

        if not hasattr(obj,"LandingDepth"):
            obj.addProperty("App::PropertyLength","LandingDepth","Steps",QT_TRANSLATE_NOOP("App::Property","The depth of the landing of these stairs"), locked=True)

        if not hasattr(obj,"TreadDepthEnforce"):
            obj.addProperty("App::PropertyLength","TreadDepthEnforce","Steps",QT_TRANSLATE_NOOP("App::Property","The depth of the treads of these stairs - Enforced regardless of Length or edge's Length"), locked=True)
        if not hasattr(obj,"RiserHeightEnforce"):
            obj.addProperty("App::PropertyLength","RiserHeightEnforce","Steps",QT_TRANSLATE_NOOP("App::Property","The height of the risers of these stairs - Enforced regardless of Height or edge's Height"), locked=True)

        if not hasattr(obj,"Flight"):
            obj.addProperty("App::PropertyEnumeration","Flight","Structure",QT_TRANSLATE_NOOP("App::Property","The direction of flight after landing"), locked=True)
            obj.Flight = ["Straight","HalfTurnLeft","HalfTurnRight"]

        # Segment and Parts properties
        if not hasattr(obj,"LastSegment"):
            obj.addProperty("App::PropertyLink","LastSegment","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Last Segment (Flight or Landing) of Arch Stairs connecting to This Segment"), locked=True)
        if not hasattr(obj,"AbsTop"):
            obj.addProperty("App::PropertyVector","AbsTop","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'absolute' top level of a flight of stairs leads to"), locked=True)
            obj.setEditorMode("AbsTop",1)
        if not hasattr(obj,"OutlineLeft"):
            obj.addProperty("App::PropertyVectorList","OutlineLeft","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'left outline' of stairs"), locked=True) # Used for Outline of Railing
            obj.setEditorMode("OutlineLeft",1)
        if not hasattr(obj,"OutlineRight"):
            obj.addProperty("App::PropertyVectorList","OutlineRight","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'left outline' of stairs"), locked=True)
            obj.setEditorMode("OutlineRight",1)

        # Can't accept 'None' in list, need NaN
        #if not hasattr(obj,"OutlineRailArcLeft"):
            #obj.addProperty("App::PropertyVectorList","OutlineRailArcLeft","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'left outline' 'arc points' of stairs railing"), locked=True)
            #obj.setEditorMode("OutlineRailArcLeft",1)
        #if not hasattr(obj,"OutlineRailArcRight"):
            #obj.addProperty("App::PropertyVectorList","OutlineRailArcRight","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'right outline' 'arc points of stairs railing"), locked=True)
            #obj.setEditorMode("OutlineRailArcRight",1)
        if not hasattr(self,"OutlineRailArcLeft"):
            self.OutlineRailArcLeft = []
        if not hasattr(self,"OutlineRailArcRight"):
            self.OutlineRailArcRight = []

        if not hasattr(obj,"RailingLeft"):
            obj.addProperty("App::PropertyLinkHidden","RailingLeft","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Name of Railing object (left) created"), locked=True)
        if not hasattr(obj,"RailingRight"):
            obj.addProperty("App::PropertyLinkHidden","RailingRight","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Name of Railing object (right) created"), locked=True)

        if not hasattr(obj,"OutlineLeftAll"):
            obj.addProperty("App::PropertyVectorList","OutlineLeftAll","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'left outline' of all segments of stairs"), locked=True)
            obj.setEditorMode("OutlineLeftAll",1) # Used for Outline of Railing
        if not hasattr(obj,"OutlineRightAll"):
            obj.addProperty("App::PropertyVectorList","OutlineRightAll","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'right outline' of all segments of stairs"), locked=True)
            obj.setEditorMode("OutlineRightAll",1)

        # Can't accept 'None' in list, need NaN
        #if not hasattr(obj,"OutlineRailArcLeftAll"):
            #obj.addProperty("App::PropertyVectorList","OutlineRailArcLeftAll","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'left outline' 'arc points' of all segments of stairs railing"), locked=True)
            #obj.setEditorMode("OutlineRailArcLeftAll",1) # Used for Outline of Railing
        #if not hasattr(obj,"OutlineRailArcRightAll"):
            #obj.addProperty("App::PropertyVectorList","OutlineRailArcRightAll","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","The 'right outline' 'arc points' of all segments of stairs railing"), locked=True)
            #obj.setEditorMode("OutlineRailArcRightAll",1)
        if not hasattr(self,"OutlineRailArcLeftAll"):
            self.OutlineRailArcLeftAll = []
        if not hasattr(self,"OutlineRailArcRightAll"):
            self.OutlineRailArcRightAll = []

        if not hasattr(obj,"RailingHeightLeft"):
            obj.addProperty("App::PropertyLength","RailingHeightLeft","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Height of Railing on Left hand side from Stairs or Landing"), locked=True)
        if not hasattr(obj,"RailingHeightRight"):
            obj.addProperty("App::PropertyLength","RailingHeightRight","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Height of Railing on Right hand side from Stairs or Landing"), locked=True)
        if not hasattr(obj,"RailingOffsetLeft"):
            obj.addProperty("App::PropertyLength","RailingOffsetLeft","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Offset of Railing on Left hand side from stairs or landing Edge"), locked=True)
        if not hasattr(obj,"RailingOffsetRight"):
            obj.addProperty("App::PropertyLength","RailingOffsetRight","Segment and Parts",QT_TRANSLATE_NOOP("App::Property","Offset of Railing on Right hand side from stairs or landing Edge"), locked=True)

        # structural properties
        if not "Landings" in pl:
            obj.addProperty("App::PropertyEnumeration","Landings","Structure",QT_TRANSLATE_NOOP("App::Property","The type of landings of these stairs"), locked=True)
            obj.Landings = ["None","At center","At each corner"]
        if not "Winders" in pl:
            obj.addProperty("App::PropertyEnumeration","Winders","Structure",QT_TRANSLATE_NOOP("App::Property","The type of winders in these stairs"), locked=True)
            obj.Winders = ["None","All","Corners strict","Corners relaxed"]
        if not "Structure" in pl:
            obj.addProperty("App::PropertyEnumeration","Structure","Structure",QT_TRANSLATE_NOOP("App::Property","The type of structure of these stairs"), locked=True)
            obj.Structure = ["None","Massive","One stringer","Two stringers"]
        if not "StructureThickness" in pl:
            obj.addProperty("App::PropertyLength","StructureThickness","Structure",QT_TRANSLATE_NOOP("App::Property","The thickness of the massive structure or of the stringers"), locked=True)
        if not "StringerWidth" in pl:
            obj.addProperty("App::PropertyLength","StringerWidth","Structure",QT_TRANSLATE_NOOP("App::Property","The width of the stringers"), locked=True)
        if not "StructureOffset" in pl:
            obj.addProperty("App::PropertyLength","StructureOffset","Structure",QT_TRANSLATE_NOOP("App::Property","The offset between the border of the stairs and the structure"), locked=True)
        if not "StringerOverlap" in pl:
            obj.addProperty("App::PropertyLength","StringerOverlap","Structure",QT_TRANSLATE_NOOP("App::Property","The overlap of the stringers above the bottom of the treads"), locked=True)
        if not "DownSlabThickness" in pl:
            obj.addProperty("App::PropertyLength","DownSlabThickness","Structure",QT_TRANSLATE_NOOP("App::Property","The thickness of the lower floor slab"), locked=True)
        if not "UpSlabThickness" in pl:
            obj.addProperty("App::PropertyLength","UpSlabThickness","Structure",QT_TRANSLATE_NOOP("App::Property","The thickness of the upper floor slab"), locked=True)
        if not "ConnectionDownStartStairs" in pl:
            obj.addProperty("App::PropertyEnumeration","ConnectionDownStartStairs","Structure",QT_TRANSLATE_NOOP("App::Property","The type of connection between the lower floor slab and the start of the stairs"), locked=True)
            obj.ConnectionDownStartStairs = ["HorizontalCut","VerticalCut","HorizontalVerticalCut"]
        if not "ConnectionEndStairsUp" in pl:
            obj.addProperty("App::PropertyEnumeration","ConnectionEndStairsUp","Structure",QT_TRANSLATE_NOOP("App::Property","The type of connection between the end of the stairs and the upper floor slab"), locked=True)
            obj.ConnectionEndStairsUp = ["toFlightThickness","toSlabThickness"]

        # additional stairs properties
        if not "ArchSketchData" in pl:
            obj.addProperty("App::PropertyBool","ArchSketchData","Stairs",QT_TRANSLATE_NOOP("App::Property","Use Base ArchSketch (if used) data (e.g. selected edge, widths, aligns) instead of Stairs' properties"), locked=True)
            obj.ArchSketchData = True
        # TODO Consider other properties for flight, landing etc.
        if not "ArchSketchEdges" in pl:
            obj.addProperty("App::PropertyStringList","ArchSketchEdges","Stairs",QT_TRANSLATE_NOOP("App::Property","Selected edges of the base Sketch/ArchSketch, to use in creating the shape (flight) of this Arch Stairs (instead of using all the Base ArchSketch's edges by default).  Input are index numbers of edges.  Disabled and ignored if Base object (ArchSketch) provides selected edges (as Flight Axis) information, with getStairsBaseShapeEdgesInfo() method.  [ENHANCEMENT by ArchSketch] GUI 'Edit Stairs' Tool is provided in external SketchArch Add-on to let users to (de)select the edges interactively.  'Toponaming-Tolerant' if ArchSketch is used in Base (and SketchArch Add-on is installed).  Warning : Not 'Toponaming-Tolerant' if just Sketch is used."), locked=True)
        if not hasattr(obj,"ArchSketchPropertySet"):
            obj.addProperty("App::PropertyEnumeration","ArchSketchPropertySet","Stairs",QT_TRANSLATE_NOOP("App::Property","Select User Defined PropertySet to use in creating variant shape, with same ArchSketch "), locked=True)
            obj.ArchSketchPropertySet = ['Default']
        if not hasattr(self,"ArchSkPropSetPickedUuid"):
            self.ArchSkPropSetPickedUuid = ''
        if not hasattr(self,"ArchSkPropSetListPrev"):
            self.ArchSkPropSetListPrev = []


    def dumps(self):  # Supercede Arch.Component.dumps()
        dump = super().dumps()
        if not isinstance(dump, tuple):
           dump = (dump,)  #Python Tuple With One Item
        dump = dump + (self.ArchSkPropSetPickedUuid, self.ArchSkPropSetListPrev)
        return dump


    def loads(self,state):
        super().loads(state)  # do nothing as of 2024.11.28
        if state == None:
            return
        elif state[0] == 'S':  # state[1] == 't', behaviour before 2024.11.28
            return
        elif state[0] == 'Stairs':
            self.ArchSkPropSetPickedUuid = state[1]
            self.ArchSkPropSetListPrev = state[2]
        elif state[0] != 'Stairs':  # model before merging super.dumps/loads()
            self.ArchSkPropSetPickedUuid = state[0]
            self.ArchSkPropSetListPrev = state[1]
        self.Type = "Stairs"


    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

        if hasattr(obj,"OutlineWireLeft"):
            self.update_properties_0v18_to_0v20(obj)

        if obj.getTypeIdOfProperty("RailingLeft") == "App::PropertyString":
            self.update_properties_0v19_to_0v20(obj)

        if (hasattr(obj,"ArchSketchData") and obj.ArchSketchData and
                Draft.getType(obj.Base) == "ArchSketch"):
            if hasattr(obj,"ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj,"ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
        else:
            if hasattr(obj,"ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj,"ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])


    def update_properties_0v18_to_0v20(self, obj):
        doc = FreeCAD.ActiveDocument
        outlineWireLeftObject = doc.getObject(obj.OutlineWireLeft)
        outlineWireRightObject = doc.getObject(obj.OutlineWireRight)
        try:
            obj.RailingLeft = outlineWireLeftObject.InList[0]
        except Exception:
            pass
        try:
            obj.RailingRight = outlineWireRightObject.InList[0]
        except Exception:
            pass
        obj.removeProperty("OutlineWireLeft")
        obj.removeProperty("OutlineWireRight")
        self.update_properties_to_0v20(obj)
        from draftutils.messages import _wrn
        _wrn("v0.20.3, " + obj.Label + ", "
             + translate("Arch", "removed properties 'OutlineWireLeft' and 'OutlineWireRight', and added properties 'RailingLeft' and 'RailingRight'"))

    def update_properties_0v19_to_0v20(self, obj):
        doc = FreeCAD.ActiveDocument
        railingLeftObject = doc.getObject(obj.RailingLeft)
        railingRightObject = doc.getObject(obj.RailingRight)
        obj.removeProperty("RailingLeft")
        obj.removeProperty("RailingRight")
        self.setProperties(obj)
        obj.RailingLeft = railingLeftObject
        obj.RailingRight = railingRightObject
        self.update_properties_to_0v20(obj)
        from draftutils.messages import _wrn
        _wrn("v0.20.3, " + obj.Label + ", "
             + translate("Arch", "changed the type of properties 'RailingLeft' and 'RailingRight'"))

    def update_properties_to_0v20(self, obj):
        additions = obj.Additions
        if obj.RailingLeft in additions:
            additions.remove(obj.RailingLeft)
        if obj.RailingRight in additions:
            additions.remove(obj.RailingRight)
        obj.Additions = additions
        if obj.RailingLeft is not None:
            obj.RailingLeft.Visibility = True
        if obj.RailingRight is not None:
            obj.RailingRight.Visibility = True

    def execute(self,obj):

        "constructs the shape of the stairs"

        if self.clone(obj):
            return

        # Stairs can do without Base.  Base validity is tested in code below.
        # Remarked out ensureBase() below
        #if not self.ensureBase(obj):
        #    return

        self.steps = []
        self.risers = []

        self.pseudosteps = []
        self.pseudorisers = []

        self.structures = []
        pl = obj.Placement
        landings = 0 # TODO Any use? 2018.7.15
        base = None
        baseProxy = None

        # PropertySet support
        propSetPickedUuidPrev = self.ArchSkPropSetPickedUuid
        propSetListPrev = self.ArchSkPropSetListPrev
        propSetSelectedNamePrev = obj.ArchSketchPropertySet
        propSetSelectedNameCur = None
        propSetListCur = None
        if Draft.getType(obj.Base) == "ArchSketch":
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy,"getPropertySet"):
                # get full list of PropertySet
                propSetListCur = baseProxy.getPropertySet(obj.Base)
                # get updated name (if any) of the selected PropertySet
                propSetSelectedNameCur = baseProxy.getPropertySet(obj.Base,
                                         propSetUuid=propSetPickedUuidPrev)
        if propSetSelectedNameCur:  # True if selection is not deleted
            if propSetListPrev != propSetListCur:
                obj.ArchSketchPropertySet = propSetListCur
                obj.ArchSketchPropertySet = propSetSelectedNameCur
                self.ArchSkPropSetListPrev = propSetListCur
            #elif propSetListPrev == propSetListCur:
                #pass  #nothing to do in this case
            # but if below, though (propSetListPrev == propSetListCur)
            elif propSetSelectedNamePrev != propSetSelectedNameCur:
                obj.ArchSketchPropertySet = propSetSelectedNameCur
        else:  # True if selection is deleted
            if propSetListCur:
                if propSetListPrev != propSetListCur:
                    obj.ArchSketchPropertySet = propSetListCur
                    obj.ArchSketchPropertySet = 'Default'
                #else:  # Seems no need ...
                    #obj.PropertySet = 'Default'

        # Get ArchSketchData if enabled
        #   e.g. flightAxis, floorHeight, landingAxis
        flightAxis = landingAxis = strucInfo = []  # None
        # baseProxy if obj.base is ArchSketch above
        if baseProxy and obj.ArchSketchData and \
                hasattr(baseProxy, 'getStairsBaseShapeEdgesInfo'):
            propSetUuid = self.ArchSkPropSetPickedUuid
            info = baseProxy.getStairsBaseShapeEdgesInfo(obj.Base,
                   propSetUuid=propSetUuid)
            if info:
                flightAxis = info.get('flightAxis')
                strucInfo = info.get('strucInfo, None')
                #floorHeight = info.get('floorHeight') TODO To support floorHeight
            # Still fallback to check if Base has Solids e.g. is another Stairs,
            # do not clear shape.
            #if not strucInfo and not flightAxis:
                #obj.Shape = Part.Shape()
                #return  #if ArchSketchData but no baseShapeEdge, no fallback
                #pass

        # Creation of base Stairs shape

        # TODO Fix:  Why below is repeated in next if routine?  Seems this is
        # just part of which further down below.  To add missing codes here?
        # If ArchSketch not provides info, but there is obj.Base e.g. a Stairs
        if (not flightAxis and not strucInfo) and obj.Base:
            if hasattr(obj.Base,"Shape"):
                if obj.Base.Shape:
                    if obj.Base.Shape.Solids:
                        base = Part.Shape(obj.Base.Shape)

        # special case NumberOfSteps = 1 : multi-edges landing
        # TODO to Support individual width, height etc. for each flight/ landing
        if (not base) and obj.Width.Value and obj.Height.Value and \
                (obj.NumberOfSteps > 0):
            # Check if there is obj.Base and its validity to proceed
            if self.ensureBase(obj):
                if not hasattr(obj.Base,'Shape'):
                    return
                # TODO See todo remarks above, below to be further reviewed.
                if obj.Base.Shape.Solids:
                    obj.Shape = obj.Base.Shape.copy()
                    obj.Placement = FreeCAD.Placement(obj.Base.Placement).multiply(pl)
                    obj.TreadDepth = 0.0
                    obj.RiserHeight = 0.0
                    return
                if not obj.Base.Shape.Edges:
                    return
                if obj.Base.Shape.Faces:
                    return

                if baseProxy and obj.ArchSketchData and not flightAxis :
                    obj.Shape = Part.Shape()
                    # TODO Clear Railings also?
                    return
                elif flightAxis:
                    edgeL = flightAxis  #[flightAxis]
                # If Base is Sketch and has input in ArchSketchEdges
                elif obj.Base.isDerivedFrom("Sketcher::SketchObject") and obj.ArchSketchEdges:
                    edgeL = []
                    baseGeom = obj.Base.Geometry
                    lg =len(baseGeom)
                    for e in obj.ArchSketchEdges:
                        ie = int(e)
                        if lg >= (ie+1) :
                            if isinstance(baseGeom[ie], Part.LineSegment):
                                edgeL.append(baseGeom[ie].toShape())
                #if not ArchSketchData and obj.ArchSketchEdges (whether stock Sketch or ArchSketch)
                elif len(obj.Base.Shape.Edges) == 1:  #elif not obj.ArchSketchData and len(obj.Base.Shape.Edges) == 1:
                    edgeL = [obj.Base.Shape.Edges[0]]
                elif len(obj.Base.Shape.Edges) > 1:  #elif not obj.ArchSketchData and len(obj.Base.Shape.Edges) > 1:  # >= 1
                    #if obj.NumberOfSteps == 1:
                    # Sort the edges so each vertex tested of its tangent direction in order
                    # TODO - Found Part.sortEdges() occasionally return less edges then 'input'
                    edgeL = [Part.sortEdges(obj.Base.Shape.Edges)[0]]
                else:  # Should not happen?
                    edgeL = []
                #lenAxis = len(flightAxis) + len(landingAxis)

            # Build Stairs if there is no obj.Base or even obj.Base is not valid
            else:
                if not obj.Length.Value:
                    obj.Shape = Part.Shape()
                    # TODO clear Railings also
                    return
                edgeL = [Part.LineSegment(Vector(0,0,0),Vector(obj.Length.Value,0,0)).toShape()]

            for edge in edgeL:
                if not isinstance(edge, list):  # i.e. a single edges
                    if isinstance(edge.Curve,(Part.LineSegment,Part.Line)):
                        # preparing for multi-edges landing / segment staircase
                        if obj.NumberOfSteps > 1:
                            # if the edge has a delta Z the start point must have lowest Z:
                            if edge.Vertexes[0].Point.z > edge.Vertexes[1].Point.z:
                                edge = Part.LineSegment(edge.Vertexes[1].Point,edge.Vertexes[0].Point).toShape()
                            self.makeStraightStairsWithLanding(obj,edge)    # all cases use makeStraightStairsWithLanding()
                        elif obj.NumberOfSteps == 1:
                            # TODO - All use self.makeMultiEdgesLanding(obj,edges) ?
                            self.makeStraightLanding(obj,edge)
                        elif obj.NumberOfSteps == 0:
                            pass  # TODO Should delete the whole shape

                    else:  # TODO Not implemented yet
                        if obj.Landings == "At center":
                            landings = 1
                            self.makeCurvedStairsWithLanding(obj,edge)
                        else:
                            self.makeCurvedStairs(obj,edge)
                #elif len(edge.Edges) >= 1:
                else:  # i.e. isinstance(edge, list), or elif len(edge) >= 1:
                    #edges = Part.sortEdges(obj.Base.Shape.Edges)[0]
                    #self.makeMultiEdgesLanding(obj,edges)
                    self.makeMultiEdgesLanding(obj,edge)

        steps = risers = None
        if self.steps:
            steps = self.steps
        elif self.pseudosteps:
            steps = self.pseudosteps
        if self.risers:
            risers = self.risers
        elif self.pseudorisers:
            risers = self.pseudorisers

        if self.structures or self.steps or self.risers:
            base = Part.makeCompound(self.structures + self.steps + self.risers)
        elif self.pseudosteps:
            shape = Part.makeCompound(self.pseudosteps)
            obj.Shape = shape
            obj.Placement = pl
            return
            # TODO No Railing etc?
        elif base:  # i.e. obj.Base has Shape e.g. it is another Stairs
            pass
        else:
            #base = None
            obj.Shape = Part.Shape()
            # TODO Clear Railings also
            # return ?



        base = self.processSubShapes(obj,base,pl)
        if base:
            if not base.isNull():
                obj.Shape = base
                obj.Placement = pl

        railingLeftObject, railWireL = None, None
        railingRightObject, railWireR = None, None
        doc = FreeCAD.ActiveDocument

        if obj.RailingLeft:
            railingLeftObject = obj.RailingLeft
            if obj.OutlineLeftAll:
                railWireL, NU = _Stairs.returnOutlineWireFace(obj.OutlineLeftAll, self.OutlineRailArcLeftAll, mode = "notFaceAlso")
            elif obj.OutlineLeft:
                railWireL, NU = _Stairs.returnOutlineWireFace(obj.OutlineLeft, self.OutlineRailArcLeft, mode = "notFaceAlso")
            else:
                print (" No obj.OutlineLeftAll or obj.OutlineLeft")

            if railWireL:
                if Draft.getType(railingLeftObject.Base) != "Part::Feature": # Base can have wrong type or be None.
                    if railingLeftObject.Base:
                        doc.removeObject(railingLeftObject.Base.Name)
                    railingLeftWireObject = doc.addObject("Part::Feature","RailingWire")
                    if FreeCAD.GuiUp:
                        railingLeftWireObject.ViewObject.hide()
                    railingLeftObject.Base = railingLeftWireObject
                # update the Base object shape
                railingLeftObject.Base.Shape = railWireL
            else:
                print (" No railWireL created ")

        if obj.RailingRight:
            railingRightObject = obj.RailingRight
            if obj.OutlineRightAll:
                railWireR, NU = _Stairs.returnOutlineWireFace(obj.OutlineRightAll, self.OutlineRailArcRightAll, mode = "notFaceAlso")
            elif obj.OutlineLeft:
                railWireR, NU = _Stairs.returnOutlineWireFace(obj.OutlineLeft, self.OutlineRailArcRight, mode = "notFaceAlso")
            else:
                print (" No obj.OutlineRightAll or obj.OutlineLeft")

            if railWireR:
                if Draft.getType(railingRightObject.Base) != "Part::Feature": # Base can have wrong type or be None.
                    if railingRightObject.Base:
                        doc.removeObject(railingRightObject.Base.Name)
                    railingRightWireObject = doc.addObject("Part::Feature","RailingWire")
                    if FreeCAD.GuiUp:
                        railingRightWireObject.ViewObject.hide()
                    railingRightObject.Base = railingRightWireObject
                # update the Base object shape
                railingRightObject.Base.Shape = railWireR
            else:
                print (" No railWireL created ")

        # compute step data
        #if obj.NumberOfSteps > 1:
        if False: # TODO Fix this - BlondelRatio is not computed
            l = obj.Length.Value
            h = obj.Height.Value
            if obj.Base:
                if hasattr(obj.Base,'Shape'):
                    l = obj.Base.Shape.Length
                    if obj.Base.Shape.BoundBox.ZLength:
                        h = obj.Base.Shape.BoundBox.ZLength
            if obj.LandingDepth:
                obj.TreadDepth = float(l-(landings*obj.LandingDepth.Value))/(obj.NumberOfSteps-(1+landings))
            else:
                obj.TreadDepth = float(l-(landings*obj.Width.Value))/(obj.NumberOfSteps-(1+landings))
            obj.RiserHeight = float(h)/obj.NumberOfSteps
            obj.BlondelRatio = obj.RiserHeight.Value*2+obj.TreadDepth.Value


    def onChanged(self,obj,prop):

        ArchComponent.Component.onChanged(self,obj,prop)

        if (prop == "ArchSketchPropertySet"
            and Draft.getType(obj.Base) == "ArchSketch"):
            baseProxy = obj.Base.Proxy
            if hasattr(baseProxy,"getPropertySet"):
                uuid = baseProxy.getPropertySet(obj,
                                 propSetName=obj.ArchSketchPropertySet)
                self.ArchSkPropSetPickedUuid = uuid
        if (hasattr(obj,"ArchSketchData") and obj.ArchSketchData
            and Draft.getType(obj.Base) == "ArchSketch"):
            if hasattr(obj,"ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", ["ReadOnly"])
            if hasattr(obj,"ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", 0)
        else:
            if hasattr(obj,"ArchSketchEdges"):
                obj.setEditorMode("ArchSketchEdges", 0)
            if hasattr(obj,"ArchSketchPropertySet"):
                obj.setEditorMode("ArchSketchPropertySet", ["ReadOnly"])


    @staticmethod
    def align(basepoint,align,widthvec):

        "moves a given basepoint according to the alignment"
        if align == "Center":
            basepoint = basepoint.add(DraftVecUtils.scale(widthvec,-0.5))
        elif align == "Right":
            basepoint = basepoint.add(DraftVecUtils.scale(widthvec,-1))
        return basepoint


    # TODO
    def makeMultiEdgesLanding(self,obj,edges):

        "builds a 'multi-edges' landing from edges" # 'copying' from makeStraightLanding()

        outline, outlineL, outlineR, vBase1, outlineP1P2ClosedNU, outlineP3P4ClosedNU, NU, pArc, pArcL, pArcR = self.returnOutlines(obj, edges, obj.Align, None, obj.Width, obj.WidthOfLanding,
                                                                                                                                    obj.TreadThickness, zeroMM, zeroMM, zeroMM, zeroMM, zeroMM, True)

        obj.AbsTop = vBase1[0]  # TODO
        stepWire, stepFace = _Stairs.returnOutlineWireFace(outline, pArc, mode = "faceAlso") #(outlinePoints, pArc, mode="wire or faceAlso")

        if obj.TreadThickness.Value:  # TODO
            step = stepFace.extrude(Vector(0,0,abs(obj.TreadThickness.Value)))
            self.steps.append(step)
        else:
            self.pseudosteps.append(stepFace)

        if obj.StructureThickness.Value:  # TODO
            landingFace = stepFace
            struct = landingFace.extrude(Vector(0,0,-abs(obj.StructureThickness.Value)))

        if struct:
            self.structures.append(struct)

        self.makeRailingOutline(obj,edges)


    # TODO
    def makeRailingOutline(self,obj,edges):

        "builds railing outline "

        outlineNotUsed, outlineRailL, outlineRailR, vBase2, outlineP1P2ClosedNU, outlineP3P4ClosedNU, NU, NU, pArcRailL, pArcRailR = self.returnOutlines(obj, edges, obj.Align, None, obj.Width,
                                                                                                                                                         obj.WidthOfLanding, obj.TreadThickness, zeroMM,
                                                                                                                                                         obj.RailingOffsetLeft, obj.RailingOffsetRight,
                                                                                                                                                         obj.RailingHeightLeft, obj.RailingHeightRight, True)
        self.connectRailingVector(obj,outlineRailL,outlineRailR, pArcRailL, pArcRailR)


    @staticmethod
    def returnOutlineWireFace(outlinePoints, pArc, mode="wire or faceAlso"):

        stepFace = None

        if not any(pArc): # i.e. no arc ... though any([0, '', False]):- is False
            stepWire = Part.makePolygon(outlinePoints)
            if mode == "faceAlso":
                stepFace = Part.Face(stepWire)
        else:
            edges = []
            enum_outlinePoints = enumerate(outlinePoints)
            lenoutlinePoints = len(outlinePoints)

            for k, a in enum_outlinePoints:
                if k < (lenoutlinePoints-1): # iterate to last but 1: [k], [k+1] ... len() is +1 over index
                    if pArc[k] is None:
                        edges.append(Part.LineSegment(outlinePoints[k],outlinePoints[k+1]).toShape())
                    else:
                        edges.append(Part.Arc(outlinePoints[k],pArc[k],outlinePoints[k+1]).toShape())

            stepWire = Part.Wire(edges)

            if mode == "faceAlso":
                stepFace = Part.Face(stepWire)

        return stepWire, stepFace


    @staticmethod                                # obj become stairsObj
    def returnOutlines(stairsObj, edges, align="Left", mode=None, widthFirstSegment=zeroMM, widthOtherSegments=[], treadThickness=zeroMM,
                       railStartRiser=zeroMM, offsetHLeft=zeroMM, offsetHRight=zeroMM, offsetVLeft=zeroMM, offsetVRight=zeroMM, widthFirstSegmentDefault=False):

        ''' Construct outline of stairs landing or the like from Edges - Side effect is vertexes are 'ordered' in series of findIntersection() functions '''

        ''' outlineP1P2Ordered seem no use at the moment '''

        #import DraftGeomUtils

        v, vLength, vWidth, vBase = [], [], [], []

        p1, p2, p3, p4, pArc, pArc1, pArc2 = [], [], [], [], [], [], []        # p1o, p2o - Not used
        outline, outlineP1P2, outlineP3P4, outlineP1P2Closed, outlineP3P4Closed, outlineP1P2Ordered = [], [], [], [], [], []

        if not isinstance(edges, list):
            edges = [edges]

        enum_edges = enumerate(edges)
        for i, edge in enum_edges:

            isLine = isinstance(edge.Curve,(Part.Line, Part.LineSegment))
            isArc = isinstance(edge.Curve,Part.Circle)                # why it is Part.Circle for an Arc Edge? - why Part.ArcOfCircle Not Working?

            ''' (1) append v (vec) '''
            v.append(DraftGeomUtils.vec(edge))    # TODO check all function below ok with curve?


            ''' (2) get netWidthI '''
            netWidthI = 0
            if i > 0:
                try:
                    if widthOtherSegments[i-1] > 0 or (not widthFirstSegmentDefault):
                        netWidthI = widthOtherSegments[i-1] - offsetHLeft.Value - offsetHRight.Value  #2*offsetH
                    else: # i.e. elif widthFirstSegmentDefault:
                        netWidthI = widthFirstSegment.Value - offsetHLeft.Value - offsetHRight.Value  #2*offsetH
                except Exception:
                    if widthFirstSegmentDefault:
                        netWidthI = widthFirstSegment.Value - offsetHLeft.Value - offsetHRight.Value  #2*offsetH

            else:
                netWidthI = widthFirstSegment.Value - offsetHLeft.Value - offsetHRight.Value  #2*offsetH


            ''' (3) append vBase '''
            vBase.append(edges[i].Vertexes[0].Point)
            if isArc:
                vBase1 = edge.Vertexes[1].Point
                vBase2 = (edge.valueAt((edge.LastParameter+edge.FirstParameter)/2))
                #vBase2vec = (vBase2-vBase[i]) # - would not be correct if Align is not Left

            ''' (1a) calc & append vLength - Need v (vec) '''
            vLength.append(Vector(v[i].x,v[i].y,v[i].z))    # TODO check all function below ok with curve? # TODO vLength in this f() is 3d


            ''' (1b, 2a) calc & append vWidth - Need vLength, netWidthI '''

            #vWidth.append(DraftVecUtils.scaleTo(vLength[i].cross(Vector(0,0,1)),netWidthI))

            if isLine:
                dvec = vLength[i].cross(Vector(0,0,1))

            elif isArc:
                #dvec = edge.Vertexes[0].Point.sub(edge.Curve.Center)        # TODO - how to determine direction? - Reference from ArchWall; used tangentAt instead
                #dvec1 = edge.Vertexes[1].Point.sub(edge.Curve.Center)
                dvec = edge.tangentAt(edge.FirstParameter).cross(Vector(0,0,1))
                dvec1 = edge.tangentAt(edge.LastParameter).cross(Vector(0,0,1))
                dvec2 = edge.tangentAt((edge.LastParameter+edge.FirstParameter)/2).cross(Vector(0,0,1))

            vWidth.append(DraftVecUtils.scaleTo(dvec,netWidthI))
            if isArc:
                vWidth1=DraftVecUtils.scaleTo(dvec1,netWidthI)
                vWidth2=DraftVecUtils.scaleTo(dvec2,netWidthI)

            ''' (3a) alter vBase '''
            if stairsObj:
                vBase[i] = stairsObj.Proxy.vbaseFollowLastSegment(stairsObj, vBase[i])
                if isArc:
                    vBase1 = stairsObj.Proxy.vbaseFollowLastSegment(stairsObj, vBase1)
                    vBase2 = stairsObj.Proxy.vbaseFollowLastSegment(stairsObj, vBase2)

            vBase[i] = vBase[i].add(Vector(0,0,offsetVLeft.Value))
            vBase[i] = vBase[i].add(Vector(0,0,railStartRiser.Value))

            if isArc:
                vBase1 = vBase1.add(Vector(0,0,offsetVLeft.Value))    # TODO - if arc is flight (sloping then), arc would be ellipse, so the following become incorrect?
                vBase1 = vBase1.add(Vector(0,0,railStartRiser.Value))
                vBase2 = vBase2.add(Vector(0,0,offsetVLeft.Value))
                vBase2 = vBase2.add(Vector(0,0,railStartRiser.Value))

            vOffsetH = DraftVecUtils.scaleTo(dvec,offsetHLeft.Value)
            if isArc:
                vOffsetH1 = DraftVecUtils.scaleTo(dvec1,offsetHLeft.Value)
                vOffsetH2 = DraftVecUtils.scaleTo(dvec2,offsetHLeft.Value)

            if align == "Left":
                vBase[i] = _Stairs.align(vBase[i], "Right", -vOffsetH)
                if isArc:
                    vBase1 = _Stairs.align(vBase1, "Right", -vOffsetH1)
                    vBase2 = _Stairs.align(vBase2, "Right", -vOffsetH2)
            elif align == "Right":
                vBase[i] = _Stairs.align(vBase[i], "Right", vOffsetH)
                if isArc:
                    vBase1 = _Stairs.align(vBase1, "Right", vOffsetH1)
                    vBase2 = _Stairs.align(vBase2, "Right", vOffsetH2)


            ''' (3b, 2b/1c) get + alter [p1, p2, p3, p4] - Need vBase '''

            p1.append(_Stairs.align(vBase[i], align, vWidth[i]).add(Vector(0,0,-abs(treadThickness.Value)))) # vWidth already calculated above against arc geometry
            if isLine:
                p2.append(p1[i].add(vLength[i]).add(Vector(0,0,-railStartRiser.Value)))
                p3.append(p2[i].add(vWidth[i]).add(Vector(0,0,(offsetVRight-offsetVLeft).Value)))
                p4.append(p3[i].add(DraftVecUtils.neg(vLength[i])).add(Vector(0,0,railStartRiser.Value)))
                pArc1.append(None)
                pArc2.append(None)
            elif isArc:
                p2.append(_Stairs.align(vBase1, align, vWidth1).add(Vector(0,0,-abs(treadThickness.Value))).add(Vector(0,0,-railStartRiser.Value)))
                p3.append(p2[i].add(vWidth1.add(Vector(0,0,(offsetVRight-offsetVLeft).Value))))
                p4.append(p1[i].add(vWidth[i].add(Vector(0,0,(offsetVRight-offsetVLeft).Value))))
                pArc1.append(_Stairs.align(vBase2, align, vWidth2).add(Vector(0,0,-abs(treadThickness.Value))).add(Vector(0,0,-railStartRiser.Value)))
                pArc2.append(pArc1[i].add(vWidth2.add(Vector(0,0,(offsetVRight-offsetVLeft).Value))))

            ''' (3c, 2c/2d) from [p1, p2, p3, p4] - calc outlineP1P2, outlineP3P4 '''

            if i > 0:
                lastEdge = edges[i-1]    # thisEdge = edge
                p1last =  p1[i-1]
                p2last =  p2[i-1]
                p3last =  p3[i-1]
                p4last =  p4[i-1]
                p1this =  p1[i]
                p2this =  p2[i]
                p3this =  p3[i]
                p4this =  p4[i]
                pArc1last = pArc1[i-1]
                pArc2last = pArc2[i-1]
                pArc1this = pArc1[i]
                pArc2this = pArc2[i]


                lastEdgeIsLineSegmentBool = isinstance(lastEdge.Curve,(Part.Line, Part.LineSegment))
                thisEdgeIsLineSegmentBool = isinstance(edge.Curve,(Part.Line, Part.LineSegment))

                lastEdgeIsCircleBool = isinstance(lastEdge.Curve,(Part.Circle))    # why it is Part.Circle for an Arc Edge? - why Part.ArcOfCircle Not Working?
                thisEdgeIsCircleBool = isinstance(edge.Curve,(Part.Circle))

                intersectionP1P2, intersectionP3P4 = _Stairs.findLineArcIntersections(p1last, p2last, p3last, p4last, p1this, p2this, p3this, p4this, lastEdgeIsLineSegmentBool, thisEdgeIsLineSegmentBool,
                                                                                      lastEdgeIsCircleBool, thisEdgeIsCircleBool, pArc1last, pArc2last, pArc1this, pArc2this)

                outlineP1P2.append(intersectionP1P2)
                outlineP3P4.insert(0, intersectionP3P4)

            else:
                outlineP1P2.append(p1[i])
                outlineP3P4.insert(0, p4[i])

        # add back last/first 'missing' point(s)
        outlineP1P2.append(p2[i])
        outlineP3P4.insert(0, p3[i])
        outline = outlineP1P2 + outlineP3P4
        outline.append(p1[0])

        pArc1.append(None)
        pArc2 = pArc2[::-1]                            # pArcReverse = pArc2[::-1]
        pArc2.append(None)
        pArc.extend(pArc1)
        pArc.extend(pArc2)                            # pArc.extend(pArcReverse)

        firstEdgeIsLineSegmentBool = isinstance(edges[0].Curve,(Part.Line, Part.LineSegment))
        firstEdgeIsCircleBool = isinstance(edges[0].Curve,(Part.Circle))    # why it is Part.Circle for an Arc Edge? - why Part.ArcOfCircle Not Working?

        if mode in ["OrderedClose", "OrderedCloseAndOrderedOpen"]:        # seem only using 'OrderedClose'
            intersectionP1P2, intersectionP3P4 = _Stairs.findLineArcIntersections(p1this, p2this, p3this, p4this, p1[0], p2[0], p3[0], p4[0], thisEdgeIsLineSegmentBool, firstEdgeIsLineSegmentBool,
                                                                                  thisEdgeIsCircleBool, firstEdgeIsCircleBool, pArc1this, pArc2this, pArc1[0], pArc2[0])
            outlineP1P2Closed = list(outlineP1P2)
            outlineP1P2Closed[0] = intersectionP1P2    #intersection[0]
            outlineP1P2Closed[i+1] = intersectionP1P2    #intersection[0]

            outlineP3P4Closed = list(outlineP3P4)
            outlineP3P4Closed[0] = intersectionP3P4    #intersection[0]
            outlineP3P4Closed[i+1] = intersectionP3P4    #intersection[0]

        if mode in ["OrderedOpen", "OrderedCloseAndOrderedOpen"]:
            if i > 0: # Multi-edge, otherwise no use

                outlineP1P2Ordered = list(outlineP1P2)

                ''' Guessing the 1st Start Point based on Intersection '''
                vx1 = Vector(outlineP1P2[1].x, outlineP1P2[1].y, 0)
                l0 = Part.LineSegment(edges[0].Vertexes[0].Point, edges[0].Vertexes[1].Point)
                try:
                    distFrom1stParameter = l0.parameter(vx1)
                    distFrom2ndParameter = l0.length()-distFrom1stParameter

                    ''' Further point of this line from intersection '''
                    if distFrom2ndParameter > distFrom1stParameter:
                        foundStart = edges[0].Vertexes[1].Point
                    else: # if distFrom2ndParameter = / < distFrom1stParameter (i.e. if equal, Vertexes[0].Point is taken ?)
                        foundStart = edges[0].Vertexes[0].Point
                except Exception:
                    print('Intersection point Not on this edge')

                ''' Guessing the last End Point based on Intersection '''
                vx99 = Vector(outlineP1P2[i].x, outlineP1P2[i].y, 0)
                l99 = Part.LineSegment(edges[i].Vertexes[0].Point, edges[i].Vertexes[1].Point)
                try:
                    distFrom1stParameter = l99.parameter(vx99)
                    distFrom2ndParameter = l99.length()-distFrom1stParameter
                    if distFrom2ndParameter > distFrom1stParameter:
                        foundEnd = edges[i].Vertexes[1].Point
                    else:
                        foundEnd = edges[i].Vertexes[0].Point
                except Exception:
                    print('Intersection point Not on this edge')

                outlineP1P2Ordered[0] = foundStart
                outlineP1P2Ordered[i+1] = foundEnd

        return outline, outlineP1P2, outlineP3P4, vBase, outlineP1P2Closed, outlineP3P4Closed, outlineP1P2Ordered, pArc, pArc1, pArc2


    @staticmethod
    def findLineArcIntersections(p1last, p2last, p3last, p4last, p1this, p2this, p3this, p4this, lastEdgeIsLineSegmentBool, thisEdgeIsLineSegmentBool, lastEdgeIsCircleBool, thisEdgeIsCircleBool,
                                 pArc1last, pArc2last, pArc1this, pArc2this):

        if lastEdgeIsLineSegmentBool and thisEdgeIsLineSegmentBool:
            intersectionsP1P2 = DraftGeomUtils.findIntersection(p1last,p2last,p1this,p2this,True,True)
            intersectionsP3P4 = DraftGeomUtils.findIntersection(p3last,p4last,p3this,p4this,True,True)
            return intersectionsP1P2[0], intersectionsP3P4[0]
        else:
            if lastEdgeIsCircleBool:
                edge1  = Part.Arc(p1last,pArc1last,p2last).toShape()        # edge1  = Part.Arc(p1[i-1],pArc1[i-1],p2[i-1]).toShape()
                edge1a = Part.Arc(p3last,pArc2last,p4last).toShape()        # edge1a = Part.Arc(p3[i-1],pArc2[i-1],p4[i-1]).toShape()
            else:
                edge1  = Part.LineSegment(p1last,p2last).toShape()        # edge1  = Part.LineSegment(p1[i-1],p2[i-1]).toShape()
                edge1a = Part.LineSegment(p3last,p4last).toShape()        # edge1a = Part.LineSegment(p3[i-1],p4[i-1]).toShape()

            if thisEdgeIsCircleBool:                        # why it is Part.Circle for an Arc Edge? - why Part.ArcOfCircle Not Working?
                edge2  = Part.Arc(p1this,pArc1this,p2this).toShape()        # edge2  = Part.Arc(p1[i],pArc1[i],p2[i]).toShape()
                edge2a = Part.Arc(p3this,pArc2this,p4this).toShape()        # edge2a = Part.Arc(p3[i],pArc2[i],p4[i]).toShape()
            else:
                edge2  = Part.LineSegment(p1this,p2this).toShape()        # edge2  = Part.LineSegment(p1[i],p2[i]).toShape()
                edge2a = Part.LineSegment(p3this,p4this).toShape()        # edge2a = Part.LineSegment(p3[i],p4[i]).toShape()
            intersections = DraftGeomUtils.findIntersection(edge1, edge2, True,True)

            enum_intersections = enumerate(intersections)
            distList = []
            for n, intersectionI in enum_intersections:
                distList.append((intersectionI-p1this).Length)            # distList.append((intersectionI-p1[i]).Length)) # TODO just use p1[i] for test; may be p2[i-1]...?

            # TODO - To test and follow up if none intersection is found
            nearestIntersectionIndex = distList.index(min(distList))
            nearestIntersectionP1P2 = intersections[nearestIntersectionIndex]

            intersections = DraftGeomUtils.findIntersection(edge1a, edge2a, True,True)
            enum_intersections = enumerate(intersections)
            distList = []
            for n, intersectionI in enum_intersections:
                distList.append((intersectionI-p4this).Length)            # distList.append((intersectionI-p4[i]).Length)) # TODO just use p4[i] for test; may be p3[i-1]...?
            nearestIntersectionIndex = distList.index(min(distList))
            nearestIntersectionP3P4 = intersections[nearestIntersectionIndex]
            return nearestIntersectionP1P2, nearestIntersectionP3P4

    @staticmethod
    def vbaseFollowLastSegment(obj, vBase):
        if obj.LastSegment:
            lastSegmentAbsTop = obj.LastSegment.AbsTop
            vBase = Vector(vBase.x, vBase.y,lastSegmentAbsTop.z)        # use Last Segment top's z-coordinate
        return vBase


    # Add flag (temporarily?) for indicating which method call this to determine whether the landing has been 're-based' before or not
    def makeStraightLanding(self,obj,edge,numberofsteps=None, callByMakeStraightStairsWithLanding=False):    # what is use of numberofsteps ?
        "builds a landing from a straight edge"

        # general data
        if not numberofsteps:
            numberofsteps = obj.NumberOfSteps
        v = DraftGeomUtils.vec(edge)
        vLength = Vector(v.x,v.y,0)
        vWidth = vWidth = DraftVecUtils.scaleTo(vLength.cross(Vector(0,0,1)),obj.Width.Value)
        vBase = edge.Vertexes[0].Point

        # if not call by makeStraightStairsWithLanding() - not 're-base' in function there, then 're-base' here
        if not callByMakeStraightStairsWithLanding:
            vBase = self.vbaseFollowLastSegment(obj, vBase)
            obj.AbsTop = vBase

        if not obj.Flight in ["HalfTurnLeft","HalfTurnRight"]:
            vNose = DraftVecUtils.scaleTo(vLength,-abs(obj.Nosing.Value))
        else:
            vNose = Vector(0,0,0)

        h = 0
        l = 0

        if obj.RiserHeightEnforce != 0:
            h = obj.RiserHeightEnforce * numberofsteps
        elif obj.Base: # TODO - should this happen? - though in original code
            if hasattr(obj.Base,'Shape'):
                #l = obj.Base.Shape.Length
                #if obj.Base.Shape.BoundBox.ZLength:
                if round(obj.Base.Shape.BoundBox.ZLength,Draft.precision()) != 0: # ? - need precision
                    h = obj.Base.Shape.BoundBox.ZLength #.Value?
                else:
                    print ("obj.Base has 0 z-value")
                    print (h)
        if (h == 0) and obj.Height.Value != 0:
            h = obj.Height.Value
        else:
            print (h)

        if obj.TreadDepthEnforce != 0:
            l = obj.TreadDepthEnforce.Value * (numberofsteps-2)
            if obj.LandingDepth:
                l += obj.LandingDepth.Value
            else:
                l += obj.Width.Value
        elif obj.Base:
            if hasattr(obj.Base,'Shape'):
                l = obj.Base.Shape.Length #.Value?
        elif obj.Length.Value != 0:
            l = obj.Length.Value

        if obj.LandingDepth:
            fLength = float(l-obj.LandingDepth.Value)/(numberofsteps-2)
        else:
            fLength = float(l-obj.Width.Value)/(numberofsteps-2)

        fHeight = float(h)/numberofsteps
        a = math.atan(fHeight/fLength)
        print("landing data:",fLength,":",fHeight)

        # step

        p1 = self.align(vBase,obj.Align,vWidth)
        p1o = p1.add(Vector(0,0,-abs(obj.TreadThickness.Value)))

        p1 = p1.add(vNose).add(Vector(0,0,-abs(obj.TreadThickness.Value)))
        p2 = p1.add(DraftVecUtils.neg(vNose)).add(vLength)
        p3 = p2.add(vWidth)
        p4 = p3.add(DraftVecUtils.neg(vLength)).add(vNose)

        p4o = p3.add(DraftVecUtils.neg(vLength))
        if not callByMakeStraightStairsWithLanding:
            p2o = p2
            p3o = p3

        if callByMakeStraightStairsWithLanding:
            if obj.Flight == "HalfTurnLeft":
                p1 = p1.add(-vWidth)
                p2 = p2.add(-vWidth)
            elif obj.Flight == "HalfTurnRight":
                p3 = p3.add(vWidth)
                p4 = p4.add(vWidth)

        step = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))
        if obj.TreadThickness.Value:
            step = step.extrude(Vector(0,0,abs(obj.TreadThickness.Value)))
            self.steps.append(step)
        else:
            self.pseudosteps.append(step)

        # structure
        struct = None
        p1 = p1.add(DraftVecUtils.neg(vNose))
        p2 = p1.add(Vector(0,0,-(abs(fHeight) - obj.TreadThickness.Value)))
        p3 = p2.add(vLength)
        p4 = p1.add(vLength)

        if obj.Structure == "Massive":
            if obj.StructureThickness.Value:
                struct = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))
                evec = vWidth
                mvec = FreeCAD.Vector(0,0,0)
                if obj.StructureOffset.Value:
                    mvec = DraftVecUtils.scaleTo(vWidth,obj.StructureOffset.Value)
                    struct.translate(mvec)
                if obj.Flight in ["HalfTurnLeft","HalfTurnRight"]:
                    evec = DraftVecUtils.scaleTo(evec,2*evec.Length-2*mvec.Length)
                else:
                    evec = DraftVecUtils.scaleTo(evec,evec.Length-(2*mvec.Length))
                struct = struct.extrude(evec)
        elif obj.Structure in ["One stringer","Two stringers"]:
            if obj.StringerWidth.Value and obj.StructureThickness.Value:
                reslength = fHeight/math.tan(a)
                p1b = p1.add(DraftVecUtils.scaleTo(vLength,reslength))
                p1c = p1.add(Vector(0,0,-fHeight))
                reslength = obj.StructureThickness.Value/math.cos(a)
                p1d = p1c.add(Vector(0,0,-reslength))
                reslength = obj.StructureThickness.Value*math.tan(a/2)
                p2 = p1b.add(DraftVecUtils.scaleTo(vLength,reslength)).add(Vector(0,0,-obj.StructureThickness.Value))
                p3 = p4.add(DraftVecUtils.scaleTo(vLength,reslength)).add(Vector(0,0,-obj.StructureThickness.Value))
                if obj.TreadThickness.Value:
                    reslength = obj.TreadThickness.Value/math.tan(a)
                    p3c = p4.add(DraftVecUtils.scaleTo(vLength,reslength)).add(Vector(0,0,obj.TreadThickness.Value))
                    reslength = obj.StructureThickness.Value/math.sin(a)
                    p3b = p3c.add(DraftVecUtils.scaleTo(vLength,reslength))
                    pol = Part.Face(Part.makePolygon([p1b,p1c,p1d,p2,p3,p3b,p3c,p4,p1b]))
                else:
                    reslength = obj.StructureThickness.Value/math.sin(a)
                    p3b = p4.add(DraftVecUtils.scaleTo(vLength,reslength))
                    pol = Part.Face(Part.makePolygon([p1b,p1c,p1d,p2,p3,p3b,p1b]))
                evec = DraftVecUtils.scaleTo(vWidth,obj.StringerWidth.Value)
                if obj.Structure == "One stringer":
                    if obj.StructureOffset.Value:
                        mvec = DraftVecUtils.scaleTo(vWidth,obj.StructureOffset.Value)
                    else:
                        mvec = DraftVecUtils.scaleTo(vWidth,(vWidth.Length/2)-obj.StringerWidth.Value/2)
                    pol.translate(mvec)
                    struct = pol.extrude(evec)
                elif obj.Structure == "Two stringers":
                    pol2 = pol.copy()
                    if obj.StructureOffset.Value:
                        mvec = DraftVecUtils.scaleTo(vWidth,obj.StructureOffset.Value)
                        pol.translate(mvec)
                        mvec = vWidth.add(mvec.negative())
                        pol2.translate(mvec)
                    else:
                        pol2.translate(vWidth)
                    s1 = pol.extrude(evec)
                    s2 = pol2.extrude(evec.negative())
                    struct = Part.makeCompound([s1,s2])

        # Overwriting result of above functions if case fit - should better avoid running the above in first place (better rewrite later)
        if not callByMakeStraightStairsWithLanding:
            if obj.StructureThickness.Value:
                struct = None
                landingFace = Part.Face(Part.makePolygon([p1o,p2o,p3o,p4o,p1o]))
                struct = landingFace.extrude(Vector(0,0,-abs(obj.StructureThickness.Value)))

        if struct:
            self.structures.append(struct)


    def makeStraightStairs(self,obj,edge,s1,s2,numOfSteps=None,
                           downstartstairs=None,endstairsup=None,hgt=None,
                           vWidth=None,align=None,vLength=None,vHeight=None,
                           vNose=None,vRiserThickness=None,
                           vTreadThickness=None,structure=None,
                           structureThickness=None,downSlabThickness=None,
                           upSlabThickness=None,structureOffset=None,
                           stringerWidth=None,stringerOverlap=None):

        "builds a simple, straight staircase from a straight edge"

        '''
        edge                  : Edge defining the flight/landing like stairs' direction, run, rise/height etc. (mandatory)

        Below parameters, if provided, would overrides information derived from the edge and/or Stairs' built-in properties -
        s1                    : (to be clarified)
        s2                    : (to be clarified)
        numOfSteps            : Numbers of Steps
        downstartstairs       : (to be clarified)
        endstairsup           : "toFlightThickness", "toSlabThickness"
        hgt                   : Height of flight
        vWidth                : Vector - Width of flight/tread
        align                 : Align of flight/tread
        vLength               : Vector - Depth/Length of tread
        vHeight               : Vector - Height of riser
        vNose                 : Vector - Length of nosing
        vRiserThickness       : Vector - Thickness of riser
        vTreadThickness       : Vector - Thickness of tread
        structure             : "Massive", "One stringer", "Two stringers"
        structureThickness    : (to be clarified)
        structureOffset       : Value - (to be clarified)
        downSlabThickness     : Value - (to be clarified)
        upSlabThickness       : Value - (to be clarified)
        stringerWidth         : Value - (to be clarified)
        stringerOverlap       : (to be clarified)

        (TODO : To support custom input of tread, riser, structure, stringer etc.
                and output of these parts individually)
        '''

        # Upgrade obj if it is from an older version of FreeCAD
        if not hasattr(obj, "StringerOverlap"):
            obj.addProperty("App::PropertyLength","StringerOverlap","Structure",QT_TRANSLATE_NOOP("App::Property","The overlap of the stringers above the bottom of the treads"), locked=True)

        v = DraftGeomUtils.vec(edge)
        v_proj = Vector(v.x, v.y, 0)  # Projected on XY plane.  May not be

        # setup numOfSteps
        if not numOfSteps:
            numOfSteps = obj.NumberOfSteps
            # if not numOfSteps - not call by makeStraightStairsWithLanding()
            # if not 're-base' there (StraightStair is part of StraightStairsWithLanding 'flight'), then 're-base' here (StraightStair is individual 'flight')

            # TODO 2025.3.23 Add attribute to identify the 'caller' instead of
            callByMakeStraightStairsWithLanding = False
        else:
            callByMakeStraightStairsWithLanding = True

        # general data
        if not downstartstairs:
            downstartstairs = obj.ConnectionDownStartStairs
        if not endstairsup:
            endstairsup = obj.ConnectionEndStairsUp

        # setup hgt (flight height)
        if not hgt:
            if round(v.z,Draft.precision()) != 0:
                hgt = v.z
            else:
                hgt = obj.Height.Value

        # setup vWidth (flight/tread width)
        if not vWidth:
            vWidth = DraftVecUtils.scaleTo(v_proj.cross(Vector(0,0,1)),obj.Width.Value)

        # setup align
        if not align:
            align = obj.Align

        # setup vLength (tread length(depth) )
        if not vLength:
            vLength = DraftVecUtils.scaleTo(v_proj,float(v_proj.Length)/(numOfSteps-1))  # TODO need this float?

        # setup vHeight (tread height)
        if not vHeight:
            vHeight = Vector(0,0,float(hgt)/numOfSteps)

        # setup vBase: Based on edge provided
        vBase = edge.Vertexes[0].Point
        # TODO Review
        if not callByMakeStraightStairsWithLanding:
            if obj.LastSegment:  # TODO
                lastSegmentAbsTop = obj.LastSegment.AbsTop  # TODO
                vBase = Vector(vBase.x, vBase.y,lastSegmentAbsTop.z)  # TODO # use Last Segment top's z-coordinate
            obj.AbsTop = vBase.add(Vector(0,0,hgt))  # TODO

        # setup vNose (nosing length)
        if not vNose:
            vNose = DraftVecUtils.scaleTo(v_proj,-abs(obj.Nosing.Value))

        # setup ang
        ang = math.atan(vHeight.Length/vLength.Length)

        # setup vBasedAligned
        vBasedAligned = self.align(vBase,align,vWidth)

        if not vRiserThickness:
            vRiserThickness = DraftVecUtils.scaleTo(v_proj,obj.RiserThickness.Value)
        if not vTreadThickness:
            vTreadThickness = DraftVecUtils.scaleTo(vHeight,obj.TreadThickness.Value)

        # steps and risers
        for i in range(numOfSteps-1):
            p1 = vBasedAligned.add((Vector(vLength).multiply(i)).add(Vector(vHeight).multiply(i+1)))  # TODO need Vector()?
            p1 = p1.add(-vTreadThickness)  # TODO 2025.4.20 Tested: Seems DraftVecUtils.neg(x) == -x
            r1 = p1
            p1 = p1.add(vNose)
            p2 = p1.add(DraftVecUtils.neg(vNose)).add(vLength)  # TODO 2025.4.20 Tested: Seems DraftVecUtils.neg(x) == -x
            p3 = p2.add(vWidth)
            p4 = p3.add(DraftVecUtils.neg(vLength)).add(vNose)  # TODO 2025.4.20
            step = Part.Face(Part.makePolygon([p1,p2,p3,p4,p1]))

            if vTreadThickness.Length:
                step = step.extrude(vTreadThickness)  # TODO need abs()?
                # TODO Add here; or add in self.execute() routine?
                self.steps.append(step)
            else:
                self.pseudosteps.append(step)

            # TODO 2025.4.20 To Review
            ''' risers - add to steps or pseudosteps in the meantime before adding self.risers / self.pseudorisers '''

            r2 = r1.add(-vHeight)    #vResHeight  # TODO 2025.4.20 Tested: Seems DraftVecUtils.neg(x) == -x
            if i == 0:
                r2 = r2.add(vTreadThickness)
            r3 = r2.add(vWidth)
            r4 = r3.add(vHeight)    #vResHeight
            if i == 0:
                r4 = r4.add(-vTreadThickness)
            riser = Part.Face(Part.makePolygon([r1,r2,r3,r4,r1]))
            if vRiserThickness.Length:
                riser = riser.extrude(vRiserThickness)    #Vector(0,100,0))
                self.steps.append(riser)
                # Separate risers from steps, keep above in the meantime
                self.risers.append(riser)
            else:
                self.pseudosteps.append(riser)
                # Separate risers from steps, keep above in the meantime
                self.pseudorisers.append(riser)

        # structure
        lProfile = []
        struct = None

        # setup structure, structureThickness, structureOffset
        if not structure:
            structure = obj.Structure
        if not structureThickness:
            structureThickness = obj.StructureThickness.Value
        if not structureOffset:
            structureOffset = obj.StructureOffset.Value

        if structure == "Massive":
            if structureThickness:
                # TODO Why 'reuse' vBase?
                # '# Massive Structure to respect 'align' attribute'
                vBase = vBasedAligned.add(vRiserThickness)

                for i in range(numOfSteps-1):
                    if not lProfile:
                        lProfile.append(vBase)
                    last = lProfile[-1]

                    if len(lProfile) == 1:
                        last = last.add(-vTreadThickness)
                    lProfile.append(last.add(vHeight))
                    lProfile.append(lProfile[-1].add(vLength))

                lProfile[-1] = lProfile[-1].add(-vRiserThickness)
                resHeight1 = structureThickness/math.cos(ang)
                dh = s2 - float(hgt)/numOfSteps

                resHeight2 = ((numOfSteps-1)*vHeight.Length) - dh

                if endstairsup == "toFlightThickness":
                    lProfile.append(lProfile[-1].add(Vector(0,0,-resHeight1)))
                    resHeight2 = ((numOfSteps-1)*vHeight.Length)-(resHeight1+vTreadThickness.z)

                    resLength = (vLength.Length/vHeight.Length)*resHeight2
                    h = DraftVecUtils.scaleTo(vLength,-resLength)
                elif endstairsup == "toSlabThickness":
                    resLength = (vLength.Length/vHeight.Length) * resHeight2
                    h = DraftVecUtils.scaleTo(vLength,-resLength)
                    th = (resHeight1 + vTreadThickness.z) - dh
                    resLength2 = th / math.tan(ang)

                    lProfile.append(lProfile[-1].add(Vector(0,0,vTreadThickness.z - dh)))
                    lProfile.append(lProfile[-1].add(DraftVecUtils.scaleTo(vLength,resLength2)))

                if  s1 > resHeight1:
                    downstartstairs = "VerticalCut"
                if downstartstairs == "VerticalCut":
                    if not downSlabThickness:
                        downSlabThickness = obj.DownSlabThickness.Value
                    dh = downSlabThickness - resHeight1 - vTreadThickness.z
                    resHeight2 = resHeight2 + downSlabThickness - dh
                    resLength = (vLength.Length/vHeight.Length)*resHeight2
                    lProfile.append(lProfile[-1].add(DraftVecUtils.scaleTo(vLength,-resLength)).add(Vector(0,0,-resHeight2)))
                elif downstartstairs == "HorizontalVerticalCut":
                    temp_s1 = s1

                    # setup upSlabThickness
                    if not upSlabThickness:
                        upSlabThickness = obj.UpSlabThickness.Value
                    if upSlabThickness > resHeight1:
                        s1 = temp_s1
                    resHeight2 = resHeight2 + s1
                    resLength = (vLength.Length/vHeight.Length) * resHeight2
                    th = (resHeight1 - s1) + vTreadThickness.z
                    resLength2 = th / math.tan(ang)
                    lProfile.append(lProfile[-1].add(DraftVecUtils.scaleTo(vLength,-resLength)).add(Vector(0,0,-resHeight2)))
                    lProfile.append(lProfile[-1].add(DraftVecUtils.scaleTo(vLength,-resLength2)))
                else:
                    lProfile.append(lProfile[-1].add(Vector(h.x,h.y,-resHeight2)))

                lProfile.append(vBase)

                pol = Part.makePolygon(lProfile)
                struct = Part.Face(pol)
                evec = vWidth
                if structureOffset:
                    mvec = DraftVecUtils.scaleTo(vWidth,structureOffset)
                    struct.translate(mvec)
                    evec = DraftVecUtils.scaleTo(evec,evec.Length-(2*mvec.Length))
                struct = struct.extrude(evec)

        elif structure in ["One stringer","Two stringers"]:
            # setup stringerWidth
            if not stringerWidth:
                stringerWidth = obj.StringerWidth.Value

            if stringerWidth and structureThickness:
                hyp = math.sqrt(vHeight.Length**2 + vLength.Length**2)
                l1 = Vector(vLength).multiply(numOfSteps-1)

                # setup stringerOverlap
                if not stringerOverlap:
                    stringerOverlap = obj.StringerOverlap.Value
                h1 = Vector(vHeight).multiply(numOfSteps-1).add(Vector(0,0,-vTreadThickness.Length+stringerOverlap))
                p1 = vBase.add(l1).add(h1)
                p1 = self.align(p1,align,vWidth)
                if stringerOverlap <= float(hgt)/numOfSteps:
                    lProfile.append(p1)
                else:
                    p1b = vBase.add(l1).add(Vector(0,0,float(h)))
                    p1a = p1b.add(Vector(vLength).multiply((p1b.z-p1.z)/vHeight.Length))
                    lProfile.append(p1a)
                    lProfile.append(p1b)
                h2 = (structureThickness/vLength.Length)*hyp
                lProfile.append(p1.add(Vector(0,0,-abs(h2))))
                h3 = lProfile[-1].z-vBase.z
                l3 = (h3/vHeight.Length)*vLength.Length
                v3 = DraftVecUtils.scaleTo(vLength,-l3)
                lProfile.append(lProfile[-1].add(Vector(0,0,-abs(h3))).add(v3))
                l4 = (structureThickness/vHeight.Length)*hyp
                v4 = DraftVecUtils.scaleTo(vLength,-l4)
                lProfile.append(lProfile[-1].add(v4))
                lProfile.append(lProfile[0])
                #print(lProfile)
                pol = Part.makePolygon(lProfile)
                pol = Part.Face(pol)
                evec = DraftVecUtils.scaleTo(vWidth,stringerWidth)

                if obj.Structure == "One stringer":
                    if structureOffset:
                        mvec = DraftVecUtils.scaleTo(vWidth,structureOffset)
                    else:
                        mvec = DraftVecUtils.scaleTo(vWidth,(vWidth.Length/2)-stringerWidth/2)
                    pol.translate(mvec)
                    struct = pol.extrude(evec)
                elif obj.Structure == "Two stringers":
                    pol2 = pol.copy()
                    if structureOffset:
                        mvec = DraftVecUtils.scaleTo(vWidth,structureOffset)
                        pol.translate(mvec)
                        mvec = vWidth.add(mvec.negative())
                        pol2.translate(mvec)
                    else:
                        pol2.translate(vWidth)
                    s1 = pol.extrude(evec)
                    s2 = pol2.extrude(evec.negative())
                    struct = Part.makeCompound([s1,s2])

        if struct:
            self.structures.append(struct)


    def makeStraightStairsWithLanding(self,obj,edge, numOfSteps=None, landings=None, landingDepth=None, height=None, width=None, align=None, treadDepthEnforce=None, riserHeightEnforce=None, flight=None):

        "builds a straight staircase with/without a landing in the middle"

        '''
        edge                : Edge defining the flight/landing like stairs' direction, run, rise/height etc. (mandatory)

        Below parameters are optional, and if provided, would overrides information derived from the edge and/or Stairs' built-in properties -
        numOfSteps          : None (default), or parameters from ArchSketch - int (default)
        landings            : None (default), or parameters from ArchSketch - 'None' (default), or 'At center' (not supported at the moment)
        landingDepth        : None (default), or parameters from ArchSketch - 'Auto' (default), or float
        height              : None (default), or parameters from ArchSketch - float (default)
        width               : None (default), or parameters from ArchSketch - float (default)
        align               : None (default), or parameters from ArchSketch - 'Left' (default), or 'Right'
        treadDepthEnforce   : None (default), or parameters from ArchSketch - 'Auto' (default), or float
        riserHeightEnforce  : None (default), or parameters from ArchSketch - 'Auto' (default), or float
        flight              : None (default), or parameters from ArchSketch - 'Straight' (default), or ['HalfTurnLeft', 'HalfTurnRight']

        (TODO : To support custom input of tread, riser, structure, stringer etc.
                and output of these parts individually)
        '''

        v = DraftGeomUtils.vec(edge)
        v_proj = Vector(v.x, v.y, 0) # Projected on XY plane.
        landingStep = 0

        # setup numOfSteps
        if numOfSteps is not None:
            if numOfSteps < 2:
                print("Fewer than 2 steps, unable to create/update stairs")
                return
            #else: print(" OK, numOfSteps equal or > 2 steps, proceed creation")
        elif obj.NumberOfSteps < 2:  #elif numOfSteps is None and obj.NumberOfSteps < 2:  # Would not be called by execute()
            print("Fewer than 2 steps, unable to create/update stairs")
            return
        else:  #elif obj.NumberOfSteps >= 2:  #elif numOfSteps is None and obj.NumberOfSteps >= 2:
            numOfSteps = obj.NumberOfSteps
            print(" numOfSteps = obj.NumberOfSteps - ", numOfSteps)

        # setup landingStep - step number of landing, if present
        if (landings == "At center" or obj.Landings == "At center"):
            wantLanding = True
        else:
            wantLanding = False
        if wantLanding and numOfSteps > 3:
            #landing = int(obj.NumberOfSteps/2)
            landingStep = int(numOfSteps/2)  # so, landing >= 1
            hasLanding = True
        else:
            #landing = obj.NumberOfSteps
            landingStep = numOfSteps
            hasLanding = False
            if wantLanding:  # but not numOfSteps > 3, so no hasLanding
                print("Fewer than 4 steps, unable to create landing")

        # setup height
        if height is None:  #if not height:
            height = obj.Height.Value
        # check width
        if width is None:  #if not width:
            width = obj.Width.Value
        # check align
        if align is None:
            align = obj.Align

        # setup vLength (tread length(depth) ) : check treadDepthEnforce
        if treadDepthEnforce and treadDepthEnforce != 'Auto':
            vLength = DraftVecUtils.scaleTo(v_proj,treadDepthEnforce)
        elif treadDepthEnforce == 'Auto' or obj.TreadDepthEnforce == 0:  #elif treadDepth is None and ...
            # check landings
            if hasLanding:
                # check landingDepth
                if landingDepth and landingDepth != 'Auto':  # i.e. landingDepth == float
                    reslength = v_proj.Length - landingDepth
                elif (landingDepth is None) and obj.LandingDepth:
                    reslength = v_proj.Length - obj.LandingDepth.Value
                    landingDepth = obj.LandingDepth.Value
                else:  # e.g. landingDepth == 'Auto', obj.LandingDepth == 0
                    reslength = v_proj.Length - width
                    landingDepth = width
                treadDepth = reslength/(numOfSteps-2)
            else:
                reslength = v_proj.Length
                treadDepth = reslength/(numOfSteps-1)
            if treadDepthEnforce != 'Auto':
                obj.TreadDepth = treadDepth
            vLength = DraftVecUtils.scaleTo(v_proj,treadDepth)
        else:  # if obj.TreadDepthEnforce
            obj.TreadDepth = obj.TreadDepthEnforce
            vLength = DraftVecUtils.scaleTo(v_proj,obj.TreadDepthEnforce.Value)

        # setup vWidth (tread width)
        vWidth = DraftVecUtils.scaleTo(vLength.cross(Vector(0,0,1)),width)

        # setup h, hstep (flight height, riser height) : check riserHeightEnforce
        if riserHeightEnforce and riserHeightEnforce != 'Auto':
            h = riserHeightEnforce * numOfSteps
            hstep = riserHeight
        #if obj.RiserHeightEnforce == 0:
        elif riserHeightEnforce == 'Auto' or obj.RiserHeightEnforce == 0:  # elif riserHeightEnforce is None and ...
            if round(v.z,Draft.precision()) != 0:
                h = v.z
            else:
                h = height
            hstep = h/numOfSteps
            if riserHeightEnforce != 'Auto':
                obj.RiserHeight = hstep
        else:  # if obj.RiserHeightEnforce
            #h = obj.RiserHeightEnforce.Value * (obj.NumberOfSteps)
            h = obj.RiserHeightEnforce.Value * numOfSteps
            hstep = obj.RiserHeightEnforce.Value
            obj.RiserHeight = hstep

        vHeight = Vector(0,0,hstep)
        p1 = edge.Vertexes[0].Point

        # TODO To add support for multi-segments stairs
        if obj.LastSegment:
            lastSegmentAbsTop = obj.LastSegment.AbsTop
            p1 = Vector(p1.x, p1.y,lastSegmentAbsTop.z)  # use Last Segment top's z-coordinate
        obj.AbsTop = p1.add(Vector(0,0,h))
        p2h = landingStep*hstep
        p2 = p1.add(DraftVecUtils.scale(vLength,landingStep-1).add(Vector(0,0,p2h)))
        edgeP1p2 = Part.LineSegment(p1,p2).toShape()

        if hasLanding:
            p3 = p2.add(DraftVecUtils.scaleTo(vLength,landingDepth))
            halfTurn = ['HalfTurnLeft', 'HalfTurnRight']
            #if obj.Flight in ["HalfTurnLeft", "HalfTurnRight"]:
            if ( (flight in halfTurn) or
                 (flight is None and (obj.Flight in halfTurn)) ):
                if (flight == 'HalfTurnLeft') or (obj.Flight in 'HalfTurnLeft') :
                    #(flight is None and (obj.Flight in 'HalfTurnLeft')) ):
                    halfTurnLR = 'HalfTurnLeft'
                else:
                    halfTurnLR = 'HalfTurnRight'
                if (align == "Left" and halfTurnLR == "HalfTurnLeft") or (align == "Right" and halfTurnLR == "HalfTurnRight"):
                    p3r = p2
                elif (align == "Left" and halfTurnLR == "HalfTurnRight"):
                    p3r = self.align(p2,"Right",-2*vWidth) # -ve / opposite direction of "Right" - no "Left" in _Stairs.Align()
                elif (align == "Right" and halfTurnLR == "HalfTurnLeft"):
                    p3r = self.align(p2,"Right",2*vWidth)
                elif (align == "Center" and halfTurnLR == "HalfTurnLeft"):
                    p3r = self.align(p2,"Right",vWidth)
                elif (align == "Center" and halfTurnLR == "HalfTurnRight"):
                    p3r = self.align(p2,"Right",-vWidth) # -ve / opposite direction of "Right" - no "Left" in _Stairs.Align()
                else:
                    print("Should have a bug here, if see this")
                if p3r:
                    p4r = p3r.add(DraftVecUtils.scale(-vLength,numOfSteps-(landingStep+1)).add(Vector(0,0,(numOfSteps-landingStep)*hstep)))

                # TODO To support DownSlabThickness, UpSlabThickness per flight
                self.makeStraightStairs(obj,Part.LineSegment(p3r,p4r).toShape(),
                                        hstep,obj.UpSlabThickness.Value,
                                        numOfSteps-landingStep,
                                        "HorizontalVerticalCut",None,hgt=h,
                                        vWidth=-vWidth,align=align,
                                        vLength=-vLength,vHeight=vHeight)
            else:
                #p4 = p3.add(DraftVecUtils.scale(vLength,numOfSteps-(landingStep+1)).add(Vector(0,0,(numOfSteps-landingStep)*hstep)))
                p4 = p3.add(DraftVecUtils.scale(vLength,numOfSteps-(
                                landingStep+1)).add(Vector(0,0,(numOfSteps-
                                landingStep)*hstep)))

                # TODO To support DownSlabThickness, UpSlabThickness per flight
                self.makeStraightStairs(obj,Part.LineSegment(p3,p4).toShape(),
                                        hstep,obj.UpSlabThickness.Value,
                                        numOfSteps-landingStep,
                                        "HorizontalVerticalCut",None,hgt=h,
                                        vWidth=vWidth,align=align,
                                        vLength=vLength,vHeight=vHeight)

            self.makeStraightLanding(obj,Part.LineSegment(p2,p3).toShape(),
                                     None,True)  # TODO numOfSteps=None, callByMakeStraightStairsWithLanding=True

            # TODO To support DownSlabThickness, UpSlabThickness per flight
            self.makeStraightStairs(obj,edgeP1p2,obj.DownSlabThickness.Value,
                                    hstep,landingStep,None,'toSlabThickness',
                                    vWidth=vWidth,align=align,
                                    vLength=vLength,vHeight=vHeight)
                                    # Remark: work without h=h, review

        else:
            # TODO To support DownSlabThickness, UpSlabThickness per flight
            self.makeStraightStairs(obj,edgeP1p2,obj.DownSlabThickness.Value,
                                    obj.UpSlabThickness.Value,landingStep,None,
                                    None,hgt=h,vWidth=vWidth,align=align,
                                    vLength=vLength,vHeight=vHeight)

        # TODO To review
        outlineNotUsed, outlineRailL, outlineRailR, vBase2, outlineP1P2ClosedNU, outlineP3P4ClosedNU, NU, pArc, pArc1, pArc2 = self.returnOutlines(obj, edgeP1p2, obj.Align, None, obj.Width,
                                                                                                                                                   obj.WidthOfLanding,
                                                                                                                                                   obj.TreadThickness, obj.RiserHeight, obj.RailingOffsetLeft,
                                                                                                                                                   obj.RailingOffsetRight, obj.RailingHeightLeft,
                                                                                                                                                   obj.RailingHeightRight,True)
        # TODO To review
        self.connectRailingVector(obj, outlineRailL, outlineRailR, pArc1, pArc2)


    def connectRailingVector(self, obj, outlineRailL, outlineRailR, pArcRailL, pArcRailR):

        obj.OutlineLeft = outlineRailL # outlineL # outlineP1P2
        obj.OutlineRight = outlineRailR # outlineR # outlineP3P4

        self.OutlineRailArcLeft = pArcRailL    #obj.OutlineRailArcLeft = pArcRailL
        self.OutlineRailArcRight = pArcRailR    #obj.OutlineRailArcRight = pArcRailR

        outlineLeftAll, outlineRightAll, outlineRailArcLeftAll, outlineRailArcRightAll = [], [], [], []

        outlineRightAll.extend(obj.OutlineRight)
        outlineRailArcRightAll = self.OutlineRailArcRight

        if obj.LastSegment:
            if obj.LastSegment.OutlineLeftAll:
                outlineLeftAll.extend(obj.LastSegment.OutlineLeftAll)

            if obj.LastSegment.Proxy.OutlineRailArcLeftAll: # need if?
                outlineRailArcLeftAll.extend(obj.LastSegment.Proxy.OutlineRailArcLeftAll)

            if (outlineLeftAll[-1] - obj.OutlineLeft[0]).Length < 0.01: # To avoid 2 points overlapping fail creating LineSegment # TODO to allow tolerance Part.LineSegment / edge.toShape() allow?
                # no need abs() after .Length right?
                del outlineLeftAll[-1]
                del outlineRailArcLeftAll[-1]

            if (outlineRightAll[-1] - obj.LastSegment.OutlineRightAll[0]).Length < 0.01: # See above
                del outlineRightAll[-1]
                del outlineRailArcRightAll[-1]

            if obj.LastSegment.OutlineRightAll: # need if?
                outlineRightAll.extend(obj.LastSegment.OutlineRightAll)

            if obj.LastSegment.Proxy.OutlineRailArcRightAll: # need if?
                outlineRailArcRightAll.extend(obj.LastSegment.Proxy.OutlineRailArcRightAll)

        outlineLeftAll.extend(obj.OutlineLeft)
        outlineRailArcLeftAll.extend(self.OutlineRailArcLeft)

        obj.OutlineLeftAll = outlineLeftAll
        obj.OutlineRightAll = outlineRightAll
        self.OutlineRailArcLeftAll = outlineRailArcLeftAll
        self.OutlineRailArcRightAll = outlineRailArcRightAll


    def makeCurvedStairs(self,obj,edge):

        print("Not yet implemented!")

    def makeCurvedStairsWithLanding(self,obj,edge):

        print("Not yet implemented!")


class _ViewProviderStairs(ArchComponent.ViewProviderComponent):

    "A View Provider for Stairs"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Stairs_Tree.svg"

    def claimChildren(self):

        "Define which objects will appear as children in the tree view"

        if hasattr(self, "Object"):
            obj = self.Object
            lst = []
            if hasattr(obj, "Base"):
                lst.append(obj.Base)
            if hasattr(obj, "RailingLeft"):
                lst.append(obj.RailingLeft)
            if hasattr(obj, "RailingRight"):
                lst.append(obj.RailingRight)
            if hasattr(obj, "Additions"):
                lst.extend(obj.Additions)
            if hasattr(obj, "Subtractions"):
                lst.extend(obj.Subtractions)
            return lst
        return []
