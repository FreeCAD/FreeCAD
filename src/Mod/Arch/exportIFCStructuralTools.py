# ***************************************************************************
# *   Copyright (c) 2020 Yorik van Havre <yorik@uncreated.net>              *
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

from __future__ import print_function


__title__ =  "FreeCAD structural IFC export tools"
__author__ = "Yorik van Havre"
__url__ =    "https://www.freecadweb.org"

ALLOW_LINEAR_OBJECTS = True # allow non-solid objects (wires, etc) to become analytic objects?

structural_nodes = {} # this keeps track of nodes during this session
scaling = 1.0 # this keeps track of scaling during this session


def setup(ifcfile,ifcbin,scale):

    """Creates all the needed setup for structural model."""

    global structural_nodes,scaling
    structural_nodes = {}
    scaling = scale
    import ifcopenshell
    uid = ifcopenshell.guid.new
    owh = ifcfile.by_type("IfcOwnerHistory")[0]
    prj = ifcfile.by_type("IfcProject")[0]
    ctx = createStructuralContext(ifcfile)
    if ifcfile.wrapped_data.schema_name == "IFC2X3":
        mod = ifcfile.createIfcStructuralAnalysisModel(uid(),owh,"Structural Analysis Model",None,None,"NOTDEFINED",None,None,None)
    else:
        pla = ifcbin.createIfcLocalPlacement()
        mod = ifcfile.createIfcStructuralAnalysisModel(uid(),owh,"Structural Analysis Model",None,None,"NOTDEFINED",None,None,None,pla)
    rel = ifcfile.createIfcRelDeclares(uid(),owh,None,None,prj,[mod])


def createStructuralContext(ifcfile):

    """Creates an additional geometry context for structural objects. Returns the new context"""

    contexts = ifcfile.by_type("IfcGeometricRepresentationContext")
    # filter out subcontexts
    contexts = [c for c in contexts if c.is_a() == "IfcGeometricRepresentationContext"]
    ctx = contexts[0] # arbitrarily take the first one...
    structcontext = ifcfile.createIfcGeometricRepresentationSubContext('Analysis','Axis',None,None,None,None,ctx,None,"GRAPH_VIEW",None)
    return structcontext


def getStructuralContext(ifcfile):

    """Returns the structural context from the file"""
    for c in ifcfile.by_type("IfcGeometricRepresentationSubContext"):
        if c.ContextIdentifier == "Analysis":
            return c


def createStructuralNode(ifcfile,ifcbin,point):

    """Creates a connection node at the given point"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    owh = ifcfile.by_type("IfcOwnerHistory")[0]
    ctx = getStructuralContext(ifcfile)
    cpt = ifcbin.createIfcCartesianPoint(tuple(point))
    vtx = ifcfile.createIfcVertexPoint(cpt)
    rep = ifcfile.createIfcTopologyRepresentation(ctx,'Analysis','Vertex',[vtx])
    psh = ifcfile.createIfcProductDefinitionShape(None,None,[rep])
    # boundary conditions serve for ex. to create fixed nodes
    #cnd = ifcfile.createIfcBoundaryNodeCondition("Fixed",ifcfile.createIfcBoolean(True),ifcfile.createIfcBoolean(True),ifcfile.createIfcBoolean(True),ifcfile.createIfcBoolean(True),ifcfile.createIfcBoolean(True),ifcfile.createIfcBoolean(True))
    # for now we don't create any boundary condition
    cnd = None
    pla = ifcbin.createIfcLocalPlacement()
    prd = ifcfile.createIfcStructuralPointConnection(uid(),owh,'Vertex',None,None,pla,psh,cnd,None)
    return prd


def createStructuralMember(ifcfile,ifcbin,obj):

    """Creates a structural member if possible. Returns the member"""

    global structural_nodes
    prd = None
    import Draft
    import Part
    import ifcopenshell
    uid = ifcopenshell.guid.new
    owh = ifcfile.by_type("IfcOwnerHistory")[0]
    ctx = getStructuralContext(ifcfile)
    edges = None
    if Draft.getType(obj) not in ["Structure"]:
        if ALLOW_LINEAR_OBJECTS and obj.isDerivedFrom("Part::Feature"):
            if obj.Shape.Faces:
                return None
            elif not obj.Shape.Edges:
                return None
            else:
                edges = obj.Shape.Edges
    else:
        wire = Part.makePolygon([obj.Placement.multVec(n) for n in obj.Nodes])
        edges = wire.Edges
    if not edges:
        return None
    for edge in edges:
        if len(edge.Vertexes) > 1:
            # we don't care about curved edges just now...
            v0 = edge.Vertexes[0].Point.multiply(scaling)
            v1 = edge.Vertexes[-1].Point.multiply(scaling)
            cp1 = ifcbin.createIfcCartesianPoint(tuple(v0))
            cp2 = ifcbin.createIfcCartesianPoint(tuple(v1))
            upv = ifcbin.createIfcDirection((0,0,1))
            pla = ifcbin.createIfcLocalPlacement()
            vp1 = ifcfile.createIfcVertexPoint(cp1)
            vp2 = ifcfile.createIfcVertexPoint(cp2)
            edg = ifcfile.createIfcEdge(vp1,vp2)
            rep = ifcfile.createIfcTopologyRepresentation(ctx,'Analysis','Edge',[edg])
            psh = ifcfile.createIfcProductDefinitionShape(None,None,[rep])
            prd = ifcfile.createIfcStructuralCurveMember(uid(),owh,obj.Label,None,None,pla,psh,"RIGID_JOINED_MEMBER",upv)
            # check for existing connection nodes
            for v in [v0,v1]:
                vk = tuple(v)
                if vk in structural_nodes:
                    if structural_nodes[vk]:
                        n = structural_nodes[vk]
                    else:
                        # there is another member with same point, create a new node
                        n = createStructuralNode(ifcfile,ifcbin,v)
                        structural_nodes[vk] = n
                    ifcfile.createIfcRelConnectsStructuralMember(uid(),None,None,None,prd,n,None,None,None,None);
                else:
                    # just add the point, no other member using it yet
                    structural_nodes[vk] = None
    return prd


def createStructuralGroup(ifcfile):

    "Assigns all structural objects found in the file to the structural model"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    owh = ifcfile.by_type("IfcOwnerHistory")[0]
    edges = ifcfile.by_type("IfcStructuralCurveMember")
    verts = ifcfile.by_type("IfcStructuralPointConnection")
    model = ifcfile.by_type("IfcStructuralAnalysisModel")[0]
    if model:
        members = edges + verts
        if members:
            ifcfile.createIfcRelAssignsToGroup(uid(),owh,None,None,members,"PRODUCT",model)


def associates(ifcfile,aobj,sobj):

    """Associates an arch object with a struct object"""

    # This is probably not the right way to do this, ie. relate a structural
    # object with an IfcProduct. Needs to investigate more....

    import ifcopenshell
    uid = ifcopenshell.guid.new
    owh = ifcfile.by_type("IfcOwnerHistory")[0]
    ifcfile.createIfcRelAssignsToProduct(uid(),owh,None,None,[sobj],None,aobj)
