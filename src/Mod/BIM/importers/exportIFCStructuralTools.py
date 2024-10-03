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


__title__ =  "FreeCAD structural IFC export tools"
__author__ = "Yorik van Havre"
__url__ =    "https://www.freecad.org"

ALLOW_LINEAR_OBJECTS = True # allow non-solid objects (wires, etc) to become analytic objects?

structural_nodes = {} # this keeps track of nodes during this session
structural_curves = {} # this keeps track of structural curves during this session
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


def createStructuralCurve(ifcfile, ifcbin, curve):

    """Creates a structural connection for a curve"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    structContext = getStructuralContext(ifcfile)

    cartPnt1 = ifcbin.createIfcCartesianPoint(tuple(curve.Vertexes[ 0].Point.multiply(scaling)))
    cartPnt2 = ifcbin.createIfcCartesianPoint(tuple(curve.Vertexes[-1].Point.multiply(scaling)))
    vertPnt1 = ifcfile.createIfcVertexPoint(cartPnt1)
    vertPnt2 = ifcfile.createIfcVertexPoint(cartPnt2)
    edge = ifcfile.createIfcEdge(vertPnt1, vertPnt2)
    topologyRep = ifcfile.createIfcTopologyRepresentation(structContext, "Analysis", "Edge", [edge])
    prodDefShape = ifcfile.createIfcProductDefinitionShape(None, None , [topologyRep])

    # boundary conditions serve for ex. to create fixed edges
    # for now we don't create any boundary condition
    appliedCondition = None
    localPlacement = ifcbin.createIfcLocalPlacement()
    origin = ifcfile.createIfcCartesianPoint((0.0, 0.0, 0.0))
    orientation = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
    xAxis = ifcfile.createIfcDirection(tuple(orientation[0]))
    zAxis = ifcfile.createIfcDirection(tuple(orientation[2]))
    localAxes = ifcfile.createIfcAxis2Placement3D(origin, zAxis, xAxis)
    structCrvConn = ifcfile.createIfcStructuralCurveConnection(
        uid(), ownerHistory, "Line", None, None, localPlacement, prodDefShape, appliedCondition, localAxes)
    return structCrvConn


def createStructuralMember(ifcfile, ifcbin, obj):

    """Creates a structural member if possible. Returns the member"""

    global structural_nodes, structural_curves
    structuralMember = None
    import Draft
    import Part
    import ifcopenshell
    import FreeCAD
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

    # OBJECT CLASSIFICATION by edge number
    # Linear elements for edge_number = 1, Surface elements for edge_number > 1
    # we don't care about curved edges just now...

    if len(edges) == 1:
        # LINEAR OBJECTS: beams, columns
        # ATM limitations:
        # - no profile properties are taken into account
        # - no materials properties are takein into account
        # -
        # create geometry
        verts = [None for _ in range(len(edges)+1)]
        verts[0] = tuple(edges[0].Vertexes[ 0].Point.multiply(scaling))
        verts[1] = tuple(edges[0].Vertexes[-1].Point.multiply(scaling))
        cartPnt1 = ifcfile.createIfcCartesianPoint(verts[0])
        cartPnt2 = ifcfile.createIfcCartesianPoint(verts[1])
        vertPnt1 = ifcfile.createIfcVertexPoint(cartPnt1)
        vertPnt2 = ifcfile.createIfcVertexPoint(cartPnt2)
        newEdge = ifcfile.createIfcEdge(vertPnt1, vertPnt2)
        topologyRep = ifcfile.createIfcTopologyRepresentation(structContext, "Analysis", "Edge", (newEdge,))
        prodDefShape = ifcfile.createIfcProductDefinitionShape(None, None, (topologyRep,))
        # set local coordinate system
        localPlacement = ifcbin.createIfcLocalPlacement()
        localZAxis = ifcbin.createIfcDirection((0, 0, 1))
        # create structural member
        if ifcfile.wrapped_data.schema_name() == "IFC2X3":
            structuralMember = ifcfile.createIfcStructuralCurveMember(
                uid(), ownerHistory, obj.Label, None, None, localPlacement, prodDefShape, "RIGID_JOINED_MEMBER")
        else:
            localZAxis = ifcbin.createIfcDirection((0, 0, 1))
            structuralMember = ifcfile.createIfcStructuralCurveMember(
                uid(), ownerHistory, obj.Label, None, None, localPlacement, prodDefShape, "RIGID_JOINED_MEMBER", localZAxis)

    elif len(edges) > 1:
        # SURFACE OBJECTS: slabs (horizontal, vertical, inclined)
        # ATM limitations:
        # - mo material properties are taken into account
        # - walls don't work because they miss a node system
        # -
        # creates geometry
        verts = [None for _ in range(len(edges))]
        for i, edge in enumerate(edges):
            verts[i] = tuple(edge.Vertexes[0].Point.multiply(scaling))
            cartPnt = ifcfile.createIfcCartesianPoint(verts[i])
            vertPnt = ifcfile.createIfcVertexPoint(cartPnt)
        orientedEdges = [None for _ in range(len(edges))]
        for i, vert in enumerate(verts):
            v2Index = (i + 1) if i < len(verts) - 1 else 0
            cartPnt1 = ifcfile.createIfcCartesianPoint(vert)
            cartPnt2 = ifcfile.createIfcCartesianPoint(verts[v2Index])
            vertPnt1 = ifcfile.createIfcVertexPoint(cartPnt1)
            vertPnt2 = ifcfile.createIfcVertexPoint(cartPnt2)
            edge = ifcfile.createIfcEdge(vertPnt1, vertPnt2)
            orientedEdges[i] = ifcfile.createIfcOrientedEdge(None, None, edge, True)
        edgeLoop = ifcfile.createIfcEdgeLoop(tuple(orientedEdges))
        # sets local coordinate system
        localPlacement = ifcbin.createIfcLocalPlacement()
        # sets face origin to the first vertex point of the planar surface
        origin = cartPnt2
        # find crossVect that is perpendicular to the planar surface
        vect0 = FreeCAD.Vector(verts[0])
        vect1 = FreeCAD.Vector(verts[1])
        vectn = FreeCAD.Vector(verts[-1])
        vect01 = vect1.sub(vect0)
        vect0n = vectn.sub(vect0)
        crossVect = vect01.cross(vect0n)
        # normalize crossVect
        normVect = crossVect.normalize()
        xAxis = ifcfile.createIfcDirection(tuple([vect01.x, vect01.y, vect01.z]))
        zAxis = ifcfile.createIfcDirection(tuple([normVect.x, normVect.y, normVect.z]))
        localAxes = ifcfile.createIfcAxis2Placement3D(origin, zAxis, xAxis)
        plane = ifcfile.createIfcPlane(localAxes)
        faceBound = ifcfile.createIfcFaceBound(edgeLoop, True)
        face = ifcfile.createIfcFaceSurface((faceBound,), plane, True)
        topologyRep = ifcfile.createIfcTopologyRepresentation(structContext, "Analysis", "Face", (face,))
        prodDefShape = ifcfile.createIfcProductDefinitionShape(None, None, (topologyRep,))
        # sets surface thickness
        # TODO: ATM limitations
        # - for vertical slabs (walls) or inclined slabs (ramps) the thickness is taken from the Height property
        thickness = float(obj.Height)*scaling
        # creates structural member
        structuralMember = ifcfile.createIfcStructuralSurfaceMember(
            uid(), ownerHistory, obj.Label, None, None, localPlacement, prodDefShape, "SHELL", thickness)

    # check for existing connection nodes
    for vert in verts:
        vertCoord = tuple(vert)
        if vertCoord in structural_nodes:
            if structural_nodes[vertCoord]:
                # there is already another member using this point
                structPntConn = structural_nodes[vertCoord]
            else:
                # there is another member with same point, create a new node connection
                structPntConn = createStructuralNode(ifcfile, ifcbin, vert)
                structural_nodes[vertCoord] = structPntConn
            ifcfile.createIfcRelConnectsStructuralMember(
                uid(), ownerHistory, None, None, structuralMember, structPntConn, None, None, None, None)
        else:
            # just add the point, no other member using it yet
            structural_nodes[vertCoord] = None

    # check for existing connection curves
    for edge in edges:
        verts12 = tuple([edge.Vertexes[ 0].Point.x, edge.Vertexes[ 0].Point.y, edge.Vertexes[ 0].Point.z,
                         edge.Vertexes[-1].Point.x, edge.Vertexes[-1].Point.y, edge.Vertexes[-1].Point.z])
        verts21 = tuple([edge.Vertexes[-1].Point.x, edge.Vertexes[-1].Point.y, edge.Vertexes[-1].Point.z,
                         edge.Vertexes[ 0].Point.x, edge.Vertexes[ 0].Point.y, edge.Vertexes[ 0].Point.z])
        verts12_in_curves = verts12 in structural_curves
        verts21_in_curves = verts21 in structural_curves
        if verts21_in_curves:
            verts = verts21
        else:
            verts = verts12
        if (verts12_in_curves or verts21_in_curves):
            if structural_curves[verts]:
                # there is already another member using this curve
                strucCrvConn = structural_curves[verts]
            else:
                # there is another member with same edge, create a new curve connection
                strucCrvConn = createStructuralCurve(ifcfile, ifcbin, edge)
                structural_curves[verts] = strucCrvConn
            ifcfile.createIfcRelConnectsStructuralMember(
                uid(), None, None, None, structuralMember, strucCrvConn, None, None, None, None)
        else:
            # just add the curve, no other member using it yet
            structural_curves[verts] = None
    return structuralMember


def createStructuralGroup(ifcfile):

    """Assigns all structural objects found in the file to the structural model"""

    import ifcopenshell
    uid = ifcopenshell.guid.new
    ownerHistory = ifcfile.by_type("IfcOwnerHistory")[0]
    structSrfMember = ifcfile.by_type("IfcStructuralSurfaceMember")
    structCrvMember = ifcfile.by_type("IfcStructuralCurveMember")
    structPntConn = ifcfile.by_type("IfcStructuralPointConnection")
    structCrvConn = ifcfile.by_type("IfcStructuralCurveConnection")
    structModel = ifcfile.by_type("IfcStructuralAnalysisModel")[0]
    if structModel:
        members = structSrfMember + structCrvMember + structPntConn + structCrvConn
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
