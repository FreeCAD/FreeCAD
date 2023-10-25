/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string>

#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepFill_Filling.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#endif

#include "FeatureFilling.h"


using namespace Surface;

PROPERTY_SOURCE(Surface::Filling, Part::Spline)

// Initial values

Filling::Filling()
{
    // clang-format off
    ADD_PROPERTY_TYPE(BoundaryEdges,(nullptr,""), "Filling", App::Prop_None,
                      "Boundary Edges (C0 is required for edges without a corresponding face)");
    ADD_PROPERTY_TYPE(BoundaryFaces,(""), "Filling", App::Prop_None, "Boundary Faces");
    ADD_PROPERTY_TYPE(BoundaryOrder,(-1), "Filling", App::Prop_None,
                      "Order of constraint on boundary faces (C0, G1 and G2 are possible)");

    ADD_PROPERTY_TYPE(UnboundEdges,(nullptr,""), "Filling", App::Prop_None,
                      "Unbound constraint edges (C0 is required for edges without a corresponding face)");
    ADD_PROPERTY_TYPE(UnboundFaces,(""), "Filling", App::Prop_None,
                      "Unbound constraint faces");
    ADD_PROPERTY_TYPE(UnboundOrder,(-1), "Filling", App::Prop_None,
                      "Order of constraint on curve faces (C0, G1 and G2 are possible)");

    ADD_PROPERTY_TYPE(FreeFaces,(nullptr,""), "Filling", App::Prop_None, "Free constraint on a face");
    ADD_PROPERTY_TYPE(FreeOrder,(0), "Filling", App::Prop_None, "Order of constraint on free faces");

    ADD_PROPERTY_TYPE(Points,(nullptr,""), "Filling", App::Prop_None, "Constraint Points (on Surface)");
    ADD_PROPERTY_TYPE(InitialFace,(nullptr), "Filling", App::Prop_None, "Initial surface to use");

    ADD_PROPERTY_TYPE(Degree,(3), "Filling", App::Prop_None, "Starting degree");
    ADD_PROPERTY_TYPE(PointsOnCurve,(15), "Filling", App::Prop_None,
                      "Number of points on an edge for constraint");
    ADD_PROPERTY_TYPE(Iterations,(2), "Filling", App::Prop_None, "Number of iterations");
    ADD_PROPERTY_TYPE(Anisotropy,(false), "Filling", App::Prop_None, "Anisotropy");
    ADD_PROPERTY_TYPE(Tolerance2d,(0.00001), "Filling", App::Prop_None, "2D Tolerance");
    ADD_PROPERTY_TYPE(Tolerance3d,(0.0001), "Filling", App::Prop_None, "3D Tolerance");
    ADD_PROPERTY_TYPE(TolAngular,(0.01), "Filling", App::Prop_None, "G1 tolerance");
    ADD_PROPERTY_TYPE(TolCurvature,(0.1), "Filling", App::Prop_None, "G2 tolerance");
    ADD_PROPERTY_TYPE(MaximumDegree,(8), "Filling", App::Prop_None, "Maximum curve degree");
    ADD_PROPERTY_TYPE(MaximumSegments,(9), "Filling", App::Prop_None, "Maximum number of segments");
    // clang-format on

    BoundaryEdges.setScope(App::LinkScope::Global);
    UnboundEdges.setScope(App::LinkScope::Global);
    FreeFaces.setScope(App::LinkScope::Global);
    Points.setScope(App::LinkScope::Global);
    InitialFace.setScope(App::LinkScope::Global);

    BoundaryEdges.setSize(0);
    BoundaryFaces.setSize(0);
    BoundaryOrder.setSize(0);
    UnboundEdges.setSize(0);
    UnboundFaces.setSize(0);
    UnboundOrder.setSize(0);
    FreeFaces.setSize(0);
    FreeOrder.setSize(0);
    Points.setSize(0);
}

short Filling::mustExecute() const
{
    if (BoundaryEdges.isTouched() || BoundaryFaces.isTouched() || BoundaryOrder.isTouched()
        || UnboundEdges.isTouched() || UnboundFaces.isTouched() || UnboundOrder.isTouched()
        || FreeFaces.isTouched() || FreeOrder.isTouched() || Points.isTouched()
        || InitialFace.isTouched() || Degree.isTouched() || PointsOnCurve.isTouched()
        || Iterations.isTouched() || Anisotropy.isTouched() || Tolerance2d.isTouched()
        || Tolerance3d.isTouched() || TolAngular.isTouched() || TolCurvature.isTouched()
        || MaximumDegree.isTouched() || MaximumSegments.isTouched()) {
        return 1;
    }
    return 0;
}

void Filling::addConstraints(BRepFill_Filling& builder,
                             const App::PropertyLinkSubList& edges,
                             const App::PropertyStringList& faces,
                             const App::PropertyIntegerList& orders,
                             Standard_Boolean bnd)
{
    auto edge_obj = edges.getValues();
    auto edge_sub = edges.getSubValues();
    auto face_sub = faces.getValues();
    auto contvals = orders.getValues();

    // if the number of continuities doesn't match then fall back to C0
    if (edge_sub.size() != contvals.size()) {
        contvals.resize(edge_sub.size());
        std::fill(contvals.begin(), contvals.end(), static_cast<long>(GeomAbs_C0));
    }

    // if the number of faces doesn't match then fall back to empty strings
    // an empty face string indicates that there is no face associated to an edge
    if (face_sub.size() != edge_sub.size()) {
        face_sub.resize(edge_obj.size());
        std::fill(face_sub.begin(), face_sub.end(), std::string());
    }

    if (edge_obj.size() == edge_sub.size()) {
        // BRepFill_Filling crashes if the boundary edges are not added in a consecutive order.
        // these edges are first added to a test wire to check that they can be securely added to
        // the Filling algo
        BRepBuilderAPI_MakeWire testWire;
        for (std::size_t index = 0; index < edge_obj.size(); index++) {
            // get the part object
            App::DocumentObject* obj = edge_obj[index];
            const std::string& sub = edge_sub[index];

            if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                // get the sub-edge of the part's shape
                const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
                TopoDS_Shape edge = shape.getSubShape(sub.c_str());
                if (!edge.IsNull() && edge.ShapeType() == TopAbs_EDGE) {
                    GeomAbs_Shape cont = static_cast<GeomAbs_Shape>(contvals[index]);

                    // check for an adjacent face of the edge
                    std::string subFace = face_sub[index];

                    // edge doesn't have set an adjacent face
                    if (subFace.empty()) {
                        if (!bnd) {
                            // not a boundary edge: safe to add it directly
                            builder.Add(TopoDS::Edge(edge), cont, bnd);
                        }
                        else {
                            // boundary edge: try to add it to the test wire first
                            testWire.Add(TopoDS::Edge(edge));
                            if (testWire.IsDone()) {
                                builder.Add(TopoDS::Edge(edge), cont, bnd);
                            }
                            else {
                                Standard_Failure::Raise(
                                    "Boundary edges must be added in a consecutive order");
                            }
                        }
                    }
                    else {
                        TopoDS_Shape face = shape.getSubShape(subFace.c_str());
                        if (!face.IsNull() && face.ShapeType() == TopAbs_FACE) {
                            if (!bnd) {
                                // not a boundary edge: safe to add it directly
                                builder.Add(TopoDS::Edge(edge), TopoDS::Face(face), cont, bnd);
                            }
                            else {
                                // boundary edge: try to add it to the test wire first
                                testWire.Add(TopoDS::Edge(edge));
                                if (testWire.IsDone()) {
                                    builder.Add(TopoDS::Edge(edge), TopoDS::Face(face), cont, bnd);
                                }
                                else {
                                    Standard_Failure::Raise(
                                        "Boundary edges must be added in a consecutive order");
                                }
                            }
                        }
                        else {
                            Standard_Failure::Raise("Sub-shape is not a face");
                        }
                    }
                }
                else {
                    Standard_Failure::Raise("Sub-shape is not an edge");
                }
            }
        }
    }
    else {
        Standard_Failure::Raise("Number of links doesn't match with number of orders");
    }
}

// Add free support faces with their continuities
void Filling::addConstraints(BRepFill_Filling& builder,
                             const App::PropertyLinkSubList& faces,
                             const App::PropertyIntegerList& orders)
{
    auto face_obj = faces.getValues();
    auto face_sub = faces.getSubValues();
    auto contvals = orders.getValues();

    if (face_obj.size() == face_sub.size() && face_obj.size() == contvals.size()) {
        for (std::size_t index = 0; index < face_obj.size(); index++) {
            App::DocumentObject* obj = face_obj[index];
            const std::string& sub = face_sub[index];
            if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
                TopoDS_Shape face = shape.getSubShape(sub.c_str());
                if (!face.IsNull() && face.ShapeType() == TopAbs_FACE) {
                    GeomAbs_Shape cont = static_cast<GeomAbs_Shape>(contvals[index]);
                    builder.Add(TopoDS::Face(face), cont);
                }
                else {
                    Standard_Failure::Raise("Sub-shape is not a face");
                }
            }
        }
    }
    else {
        Standard_Failure::Raise("Number of links doesn't match with number of orders");
    }
}

void Filling::addConstraints(BRepFill_Filling& builder, const App::PropertyLinkSubList& pointsList)
{
    auto points = pointsList.getSubListValues();
    for (const auto& it : points) {
        App::DocumentObject* obj = it.first;
        std::vector<std::string> sub = it.second;
        if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
            for (const auto& jt : sub) {
                TopoDS_Shape subShape = shape.getSubShape(jt.c_str());
                if (!subShape.IsNull() && subShape.ShapeType() == TopAbs_VERTEX) {
                    gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(subShape));
                    builder.Add(pnt);
                }
            }
        }
    }
}

App::DocumentObjectExecReturn* Filling::execute()
{
    // Assign Variables
    unsigned int degree = Degree.getValue();
    unsigned int ptsoncurve = PointsOnCurve.getValue();
    unsigned int numIter = Iterations.getValue();
    bool anisotropy = Anisotropy.getValue();
    double tol2d = Tolerance2d.getValue();
    double tol3d = Tolerance3d.getValue();
    double tolG1 = TolAngular.getValue();
    double tolG2 = TolCurvature.getValue();
    unsigned int maxdeg = MaximumDegree.getValue();
    unsigned int maxseg = MaximumSegments.getValue();

    try {
        BRepFill_Filling builder(degree,
                                 ptsoncurve,
                                 numIter,
                                 anisotropy,
                                 tol2d,
                                 tol3d,
                                 tolG1,
                                 tolG2,
                                 maxdeg,
                                 maxseg);

        if ((BoundaryEdges.getSize()) < 1) {
            return new App::DocumentObjectExecReturn(
                "Border must have at least one curve defined.");
        }

        // Load the initial surface if set
        App::DocumentObject* initFace = InitialFace.getValue();
        if (initFace && initFace->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const Part::TopoShape& shape = static_cast<Part::Feature*>(initFace)->Shape.getShape();
            std::vector<std::string> subNames = InitialFace.getSubValues();
            for (const auto& it : subNames) {
                TopoDS_Shape subShape = shape.getSubShape(it.c_str());
                if (!subShape.IsNull() && subShape.ShapeType() == TopAbs_FACE) {
                    builder.LoadInitSurface(TopoDS::Face(subShape));
                    break;
                }
            }
        }

        // Add the constraints of border curves/faces (bound)
        int numBoundaries = BoundaryEdges.getSize();
        addConstraints(builder, BoundaryEdges, BoundaryFaces, BoundaryOrder, Standard_True);

        // Add additional edge constraints if available (unbound)
        if (UnboundEdges.getSize() > 0) {
            addConstraints(builder, UnboundEdges, UnboundFaces, UnboundOrder, Standard_False);
        }

        // Add additional constraint on free faces
        if (FreeFaces.getSize() > 0) {
            addConstraints(builder, FreeFaces, FreeOrder);
        }

        // App point constraints
        if (Points.getSize() > 0) {
            addConstraints(builder, Points);
        }

        // Build the face
        if (numBoundaries > 1) {
            builder.Build();
        }
        if (!builder.IsDone()) {
            Standard_Failure::Raise("Failed to create a face from constraints");
        }

        // Return the face
        TopoDS_Face aFace = builder.Face();
        this->Shape.setValue(aFace);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
