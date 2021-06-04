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


def setup(ifcfile, ifcbin, scale):

    """Creates all the needed setup for structural model."""

    global structural_nodes, scaling
    structural_nodes = {}
    scaling = scale
    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    project = ifcfile.by_type("IfcProject")[0]
    structContext = createStructuralContext(ifcfile)
    if ifcfile.wrapped_data.schema_name() == "IFC2X3":
        mod = ifcfile.createIfcStructuralAnalysisModel(
            uid(), ownerHistory, "Structural Analysis Model", None, None, "NOTDEFINED", None, None, None)
    else:
        localPlacement = ifcbin.createIfcLocalPlacement()
        structModel = ifcfile.createIfcStructuralAnalysisModel(
            uid(), ownerHistory, "Structural Analysis Model", None, None, "NOTDEFINED", None, None, None, localPlacement)
        relation = ifcfile.createIfcRelDeclares(uid(), ownerHistory, None, None, project, [structModel])


def createStructuralContext(ifcfile):

    """Creates an additional geometry context for structural objects. Returns the new context"""

    contexts = ifcfile.by_type("IfcGeometricRepresentationContext")
    # filter out subcontexts
    contexts = [c for c in contexts if c.is_a() == "IfcGeometricRepresentationContext"]
    geomContext = contexts[0] # arbitrarily take the first one...
    structContext = ifcfile.createIfcGeometricRepresentationSubContext(
        'Analysis', 'Axis', None, None, None, None, geomContext, None, "GRAPH_VIEW", None)
    return structContext


def getStructuralContext(ifcfile):

    """Returns the structural context from the file"""
    for c in ifcfile.by_type("IfcGeometricRepresentationSubContext"):
        if c.ContextIdentifier == "Analysis":
            return c


def createStructuralNode(ifcfile, ifcbin, point):

    """Creates a connection node at the given point"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    structContext = getStructuralContext(ifcfile)
    cartPoint = ifcbin.createIfcCartesianPoint(tuple(point))
    vertPoint = ifcfile.createIfcVertexPoint(cartPoint)
    topologyRep = ifcfile.createIfcTopologyRepresentation(structContext, 'Analysis', 'Vertex', [vertPoint])
    prodDefShape = ifcfile.createIfcProductDefinitionShape(None, None, [topologyRep])
    # boundary conditions serve for ex. to create fixed nodes
    # appliedCondition = ifcfile.createIfcBoundaryNodeCondition(
    #     "Fixed",ifcfile.createIfcBoolean(True), ifcfile.createIfcBoolean(True), ifcfile.createIfcBoolean(True),
    #     ifcfile.createIfcBoolean(True), ifcfile.createIfcBoolean(True), ifcfile.createIfcBoolean(True))
    # for now we don't create any boundary condition
    appliedCondition = None
    localPlacement = ifcbin.createIfcLocalPlacement()
    if ifcfile.wrapped_data.schema_name() == "IFC2X3":
        structPntConn = ifcfile.createIfcStructuralPointConnection(
            uid(), ownerHistory, 'Vertex', None, None, localPlacement, prodDefShape, appliedCondition)
    else:
        structPntConn = ifcfile.createIfcStructuralPointConnection(
            uid(), ownerHistory, 'Vertex', None, None, localPlacement, prodDefShape, appliedCondition, None)
    return structPntConn


def createStructuralMember(ifcfile, ifcbin, obj):

    """Creates a structural member if possible. Returns the member"""

    global structural_nodes
    structuralMember = None
    import Draft
    import Part
    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    structContext = getStructuralContext(ifcfile)
    
    # find edges to convert into structural members
    edges = None
    if Draft.getType(obj) not in ["Structure"]:
        # for non structural elements 
        if ALLOW_LINEAR_OBJECTS and obj.isDerivedFrom("Part::Feature"):
            # for objects created with Part workbench
            if obj.Shape.Faces:
                # for objects with faces
                return None
            elif not obj.Shape.Edges:
                # for objects without edges
                return None
            else:
                edges = obj.Shape.Edges
    else:
        # for structural elements with nodes
        nodes = [obj.Placement.multVec(n) for n in obj.Nodes]
        if len(nodes) > 2:
            # when there are more then 2 nodes (i.e. for slabs) append closing node to produce closing edge
            nodes.append(nodes[0])
        wire = Part.makePolygon(nodes)
        edges = wire.Edges
    if not edges:
        return None
    for edge in edges:
        if len(edge.Vertexes) > 1:
            # we don't care about curved edges just now...
            vert0 = edge.Vertexes[ 0].Point.multiply(scaling)
            vert1 = edge.Vertexes[-1].Point.multiply(scaling)
            cartPoint1 = ifcbin.createIfcCartesianPoint(tuple(vert0))
            cartPoint2 = ifcbin.createIfcCartesianPoint(tuple(vert1))
            localPlacement = ifcbin.createIfcLocalPlacement()
            vertPoint1 = ifcfile.createIfcVertexPoint(cartPoint1)
            vertPoint2 = ifcfile.createIfcVertexPoint(cartPoint2)
            newEdge = ifcfile.createIfcEdge(vertPoint1, vertPoint2)
            topologyRep = ifcfile.createIfcTopologyRepresentation(structContext, 'Analysis', 'Edge', [newEdge])
            prodDefShape = ifcfile.createIfcProductDefinitionShape(None, None, [topologyRep])
            if ifcfile.wrapped_data.schema_name() == "IFC2X3":
                structuralMember = ifcfile.createIfcStructuralCurveMember(
                    uid(), ownerHistory, obj.Label, None, None, localPlacement, prodDefShape, "RIGID_JOINED_MEMBER")
            else:
                localZAxis = ifcbin.createIfcDirection((0, 0, 1))
                structuralMember = ifcfile.createIfcStructuralCurveMember(
                    uid(), ownerHistory, obj.Label, None, None, localPlacement, prodDefShape, "RIGID_JOINED_MEMBER", localZAxis)
            # check for existing connection nodes
            for vert in [vert0, vert1]:
                vertCoord = tuple(vert)
                if vertCoord in structural_nodes:
                    if structural_nodes[vertCoord]:
                        # there is already another member using this point
                        structPntConn = structural_nodes[vertCoord]
                    else:
                        # there is another member with same point, create a new node
                        structPntConn = createStructuralNode(ifcfile, ifcbin, vert)
                        structural_nodes[vertCoord] = structPntConn
                    ifcfile.createIfcRelConnectsStructuralMember(
                        uid(), ownerHistory, None, None, structuralMember, structPntConn, None, None, None, None)
                else:
                    # just add the point, no other member using it yet
                    structural_nodes[vertCoord] = None
    return structuralMember


def createStructuralGroup(ifcfile):

    """Assigns all structural objects found in the file to the structural model"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    structCrvMember  = ifcfile.by_type("IfcStructuralCurveMember")
    structPntConn = ifcfile.by_type("IfcStructuralPointConnection")
    structModel = ifcfile.by_type("IfcStructuralAnalysisModel")[0]
    if structModel:
        members = structCrvMember + structPntConn
        if members:
            ifcfile.createIfcRelAssignsToGroup(uid(), ownerHistory, None, None, members, "PRODUCT", structModel)


def associates(ifcfile, aobj, sobj):

    """Associates an arch object with a struct object"""

    # This is probably not the right way to do this, ie. relate a structural
    # object with an IfcProduct. Needs to investigate more....

    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    ifcfile.createIfcRelAssignsToProduct(uid(), ownerHistory, None, None, [sobj], None, aobj)
