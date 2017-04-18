/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com> *
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
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepFill_Filling.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <Precision.hxx>
#endif

#include "FeatureFilling.h"
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <string>

using namespace Surface;

PROPERTY_SOURCE(Surface::Filling, Part::Feature)

//Initial values

Filling::Filling()
{
    ADD_PROPERTY_TYPE(Border,(0,""), "Filling", App::Prop_None, "Border Edges (C0 is required for edges without a corresponding face)");
    ADD_PROPERTY_TYPE(BorderFaces,(0,""), "Filling", App::Prop_None, "Border Faces");
    ADD_PROPERTY_TYPE(OrderBorderFaces,(-1), "Filling", App::Prop_None, "Order of constraint on border faces (C0, G1 and G2 are possible)");

    ADD_PROPERTY_TYPE(Curves,(0,""), "Filling", App::Prop_None, "Other Constraint Curves (C0 is required for edges without a corresponding face)");
    ADD_PROPERTY_TYPE(CurveFaces,(0,""), "Filling", App::Prop_None, "Curve Faces");
    ADD_PROPERTY_TYPE(OrderCurveFaces,(-1), "Filling", App::Prop_None, "Order of constraint on curve faces (C0, G1 and G2 are possible)");

    ADD_PROPERTY_TYPE(FreeFaces,(0,""), "Filling", App::Prop_None, "Free constraint on a face");
    ADD_PROPERTY_TYPE(OrderFreeFaces,(0), "Filling", App::Prop_None, "Order of constraint on free faces");

    ADD_PROPERTY_TYPE(Points,(0,""), "Filling", App::Prop_None, "Constraint Points (on Surface)");
    ADD_PROPERTY_TYPE(InitialFace,(0), "Filling", App::Prop_None, "Initial surface to use");

    ADD_PROPERTY_TYPE(Degree,(3), "Filling", App::Prop_None, "Starting degree");
    ADD_PROPERTY_TYPE(PointsOnCurve,(3), "Filling", App::Prop_None, "Number of points on an edge for constraint");
    ADD_PROPERTY_TYPE(Iterations,(2), "Filling", App::Prop_None, "Number of iterations");
    ADD_PROPERTY_TYPE(Anisotropy,(false), "Filling", App::Prop_None, "Anisotropy");
    ADD_PROPERTY_TYPE(Tolerance2d,(0.00001), "Filling", App::Prop_None, "2D Tolerance");
    ADD_PROPERTY_TYPE(Tolerance3d,(0.0001), "Filling", App::Prop_None, "3D Tolerance");
    ADD_PROPERTY_TYPE(TolAngular,(0.001), "Filling", App::Prop_None, "G1 tolerance");
    ADD_PROPERTY_TYPE(TolCurvature,(0.01), "Filling", App::Prop_None, "G2 tolerance");
    ADD_PROPERTY_TYPE(MaximumDegree,(8), "Filling", App::Prop_None, "Maximum curve degree");
    ADD_PROPERTY_TYPE(MaximumSegments,(10000), "Filling", App::Prop_None, "Maximum number of segments");
}

short Filling::mustExecute() const
{
    if (Border.isTouched() ||
        BorderFaces.isTouched() ||
        OrderBorderFaces.isTouched() ||
        Curves.isTouched() ||
        CurveFaces.isTouched() ||
        OrderCurveFaces.isTouched() ||
        FreeFaces.isTouched() ||
        OrderFreeFaces.isTouched() ||
        Points.isTouched() ||
        InitialFace.isTouched() ||
        Degree.isTouched() ||
        PointsOnCurve.isTouched() ||
        Iterations.isTouched() ||
        Anisotropy.isTouched() ||
        Tolerance2d.isTouched() ||
        Tolerance3d.isTouched() ||
        TolAngular.isTouched() ||
        TolCurvature.isTouched() ||
        MaximumDegree.isTouched() ||
        MaximumSegments.isTouched())
        return 1;
    return 0;
}

void Filling::addConstraints(BRepFill_Filling& builder,
                             const App::PropertyLinkSubList& edges,
                             const App::PropertyLinkSubList& faces,
                             const App::PropertyIntegerList& orders,
                             Standard_Boolean bnd)
{
    auto edge_obj = edges.getValues();
    auto edge_sub = edges.getSubValues();
    auto face_obj = faces.getValues();
    auto face_sub = faces.getSubValues();
    auto contvals = orders.getValues();

    if (edge_obj.size() == edge_sub.size() &&
        edge_obj.size() == contvals.size()) {
        for (std::size_t index = 0; index < edge_obj.size(); index++) {
            App::DocumentObject* obj = edge_obj[index];
            const std::string& sub = edge_sub[index];
            if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
                TopoDS_Shape edge = shape.getSubShape(sub.c_str());
                if (!edge.IsNull() && edge.ShapeType() == TopAbs_EDGE) {
                    GeomAbs_Shape cont = static_cast<GeomAbs_Shape>(contvals[index]);
                    if (cont == GeomAbs_C0) {
                        builder.Add(TopoDS::Edge(edge), cont, bnd);
                    }
                    else {
                        builder.Add(TopoDS::Edge(edge), cont, bnd);
                    }
                }
            }
        }
    }
    else {
        Standard_Failure::Raise("Number of links doesn't match with number of orders");
    }
}

void Filling::addConstraints(BRepFill_Filling& builder,
                             const App::PropertyLinkSubList& faces,
                             const App::PropertyIntegerList& orders)
{
    auto face_obj = faces.getValues();
    auto face_sub = faces.getSubValues();
    auto contvals = orders.getValues();

    if (face_obj.size() == face_sub.size() &&
        face_obj.size() == contvals.size()) {
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
            }
        }
    }
    else {
        Standard_Failure::Raise("Number of links doesn't match with number of orders");
    }
}

void Filling::addConstraints(BRepFill_Filling& builder,
                             const App::PropertyLinkSubList& pointsList)
{
    auto points = pointsList.getSubListValues();
    for (auto it : points) {
        App::DocumentObject* obj = it.first;
        std::vector<std::string> sub = it.second;
        if (obj && obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const Part::TopoShape& shape = static_cast<Part::Feature*>(obj)->Shape.getShape();
            for (auto jt : sub) {
                TopoDS_Shape subShape = shape.getSubShape(jt.c_str());
                if (!subShape.IsNull() && subShape.ShapeType() == TopAbs_VERTEX) {
                    gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(subShape));
                    builder.Add(pnt);
                }
            }
        }
    }
}

App::DocumentObjectExecReturn *Filling::execute(void)
{
    //Assign Variables
    unsigned int degree  = Degree.getValue();
    unsigned int ptsoncurve = PointsOnCurve.getValue();
    unsigned int numIter   = Iterations.getValue();
    bool anisotropy = Anisotropy.getValue();
    double tol2d = Tolerance2d.getValue();
    double tol3d = Tolerance3d.getValue();
    double tolG1 = TolAngular.getValue();
    double tolG2 = TolCurvature.getValue();
    unsigned int maxdeg = MaximumDegree.getValue();
    unsigned int maxseg = MaximumSegments.getValue();

    try {
        BRepFill_Filling builder(degree, ptsoncurve, numIter, anisotropy, tol2d,
                                 tol3d, tolG1, tolG2, maxdeg, maxseg);

        if ((Border.getSize()) < 1) {
            return new App::DocumentObjectExecReturn("Border must have at least one curve defined.");
        }

        // Load the initial surface if set
        App::DocumentObject* initFace = InitialFace.getValue();
        if (initFace && initFace->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const Part::TopoShape& shape = static_cast<Part::Feature*>(initFace)->Shape.getShape();
            std::vector<std::string> subNames = InitialFace.getSubValues();
            for (auto it : subNames) {
                TopoDS_Shape subShape = shape.getSubShape(it.c_str());
                if (!subShape.IsNull() && subShape.ShapeType() == TopAbs_FACE) {
                    builder.LoadInitSurface(TopoDS::Face(subShape));
                    break;
                }
            }
        }

        // Add the constraints of border curves/faces (bound)
        addConstraints(builder, Border, BorderFaces, OrderBorderFaces, Standard_True);

        // Add additional curves constraints if available (unbound)
        if (Curves.getSize() > 0) {
            addConstraints(builder, Curves, CurveFaces, OrderCurveFaces, Standard_False);
        }

        // Add additional constraint on free faces
        if (FreeFaces.getSize() > 0) {
            addConstraints(builder, FreeFaces, OrderFreeFaces);
        }

        // App point constraints
        if (Points.getSize() > 0) {
            addConstraints(builder, Points);
        }

        //Build the face
        builder.Build();
        if (!builder.IsDone()) {
            Standard_Failure::Raise("Failed to create a face from constraints");
        }

        //Return the face
        TopoDS_Face aFace = builder.Face();
        this->Shape.setValue(aFace);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle(Standard_Failure) e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}
