// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Geom_BSplineCurve.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Compound.hxx>

#include "FeaturePatch.h"
#include <Base/Converter.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Projection.h>
#include <Mod/Mesh/App/MeshFeature.h>

using namespace MeshPart;

PROPERTY_SOURCE(MeshPart::Patch, Part::Feature)

const App::PropertyIntegerConstraint::Constraints numberOfEdges = {2, 100, 1};
const App::PropertyIntegerConstraint::Constraints maxDegree = {2, 8, 1};

Patch::Patch()
{
    const double tol3d = 0.005;
    ADD_PROPERTY_TYPE(Mesh, (nullptr), "Patch", App::Prop_None, "Input mesh");
    ADD_PROPERTY_TYPE(P1, (Base::Vector3d()), "Patch", App::Prop_None, "Point of patch");
    ADD_PROPERTY_TYPE(P2, (Base::Vector3d()), "Patch", App::Prop_None, "Point of patch");
    ADD_PROPERTY_TYPE(P3, (Base::Vector3d()), "Patch", App::Prop_None, "Point of patch");
    ADD_PROPERTY_TYPE(P4, (Base::Vector3d()), "Patch", App::Prop_None, "Point of patch");
    ADD_PROPERTY_TYPE(ViewDirection, (Base::Vector3d(0, 0, 1)), "Patch", App::Prop_None, "View direction");
    ADD_PROPERTY_TYPE(Tolerance, (tol3d), "Patch", App::Prop_None, "Tolerance");
    ADD_PROPERTY_TYPE(MaxDegree, (3L), "Patch", App::Prop_None, "Maximum degree of approximation");
    ADD_PROPERTY_TYPE(Profiles, (2L), "Patch", App::Prop_None, "Number of Profile edges");
    ADD_PROPERTY_TYPE(Guides, (2L), "Patch", App::Prop_None, "Number of Guide edges");

    MaxDegree.setConstraints(&maxDegree);
    Profiles.setConstraints(&numberOfEdges);
    Guides.setConstraints(&numberOfEdges);
}

namespace
{
std::unique_ptr<MeshCore::MeshFacetGrid> makeGrid(const Mesh::MeshObject& mesh)
{
    const MeshCore::MeshKernel& kernel = mesh.getKernel();
    MeshCore::MeshAlgorithm alg(kernel);
    float fAvgLen = alg.GetAverageEdgeLength();
    const float factor = 5.0F;
    return std::make_unique<MeshCore::MeshFacetGrid>(kernel, factor * fAvgLen);
}

MeshCore::FacetIndex getFacetIndex(const MeshCore::MeshFacetGrid& grid, const Base::Vector3f& pnt)
{
    return grid.SearchNearestFromPoint(pnt);
}

Handle(
    Geom_BSplineCurve
) approximateSpline(const std::vector<Base::Vector3f>& points, int maxDegree, double tol3d)
{
    TColgp_Array1OfPnt pnts(1, static_cast<int>(points.size()));
    Standard_Integer index = 1;
    for (const auto& it : points) {
        pnts(index++) = gp_Pnt(it.x, it.y, it.z);
    }

    try {
        GeomAPI_PointsToBSpline fit(pnts, 1, maxDegree, GeomAbs_G2, tol3d);
        Handle(Geom_BSplineCurve) spline = fit.Curve();
        return spline;
    }
    catch (...) {
        return Handle(Geom_BSplineCurve)();
    }
}

Handle(Geom_BSplineCurve) approximateSpline(
    const Mesh::MeshObject& kernel,
    const MeshCore::MeshFacetGrid& grid,
    const Base::Vector3d& P1,
    const Base::Vector3d& P2,
    const Base::Vector3d& Vd,
    int maxDegree,
    double tol3d
)
{
    auto p1 = Base::convertTo<Base::Vector3f>(P1);
    auto p2 = Base::convertTo<Base::Vector3f>(P2);
    auto vd = Base::convertTo<Base::Vector3f>(Vd);

    MeshCore::FacetIndex f1 = getFacetIndex(grid, p1);
    MeshCore::FacetIndex f2 = getFacetIndex(grid, p2);

    MeshCore::MeshProjection meshProjection(kernel.getKernel());
    std::vector<Base::Vector3f> polyline;

    if (meshProjection.projectLineOnMesh(grid, p1, f1, p2, f2, vd, polyline)) {
        return approximateSpline(polyline, maxDegree, tol3d);
    }

    return Handle(Geom_BSplineCurve)();
}

TopoDS_Edge makeEdge(Handle(Geom_BSplineCurve) spline)
{
    double u = spline->FirstParameter();
    double v = spline->LastParameter();
    BRepBuilderAPI_MakeEdge mkBuilder(spline, u, v);
    TopoDS_Edge edge = mkBuilder.Edge();
    return edge;
}

struct Line
{
    Base::Vector3d p1;
    Base::Vector3d p2;
};

std::vector<Line> makeIsoCurves(
    Handle(Geom_BSplineCurve) spline1,  // NOLINT
    Handle(Geom_BSplineCurve) spline2,  // NOLINT
    long count
)
{
    if (count < 3) {
        return {};
    }

    std::vector<Line> lines;
    double u1 = spline1->FirstParameter();
    double v1 = spline1->LastParameter();

    double u2 = spline2->FirstParameter();
    double v2 = spline2->LastParameter();

    double s1 = (v1 - u1) / static_cast<double>(count - 1);
    double s2 = (v2 - u2) / static_cast<double>(count - 1);

    for (long i = 1; i < count - 1; i++) {
        double step = static_cast<double>(i);  // NOLINT
        gp_Pnt p1 = spline1->Value(u1 + step * s1);
        gp_Pnt p2 = spline2->Value(v2 - step * s2);

        Line line;
        line.p1.Set(p1.X(), p1.Y(), p1.Z());
        line.p2.Set(p2.X(), p2.Y(), p2.Z());
        lines.push_back(line);
    }

    return lines;
}

}  // namespace

App::DocumentObjectExecReturn* Patch::execute()
{
    auto mesh = Mesh.getValue<Mesh::Feature*>();
    if (!mesh) {
        throw Base::TypeError("No mesh object linked");
    }

    int maxDegree = static_cast<int>(MaxDegree.getValue());
    double tol3d = Tolerance.getValue();

    const Mesh::MeshObject& kernel = mesh->Mesh.getValue();
    std::unique_ptr<MeshCore::MeshFacetGrid> grid(makeGrid(kernel));

    Base::Vector3d vd = ViewDirection.getValue();
    Handle(Geom_BSplineCurve) spline;

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    auto approximate = [&](const Base::Vector3d& v1, const Base::Vector3d& v2) {
        Handle(Geom_BSplineCurve) spline;
        spline = approximateSpline(kernel, *grid, v1, v2, vd, maxDegree, tol3d);
        if (!spline.IsNull()) {
            builder.Add(comp, makeEdge(spline));
        }

        return spline;
    };

    auto spline1 = approximate(P1.getValue(), P2.getValue());
    auto spline2 = approximate(P2.getValue(), P3.getValue());
    auto spline3 = approximate(P3.getValue(), P4.getValue());
    auto spline4 = approximate(P4.getValue(), P1.getValue());

    if (!spline1.IsNull() && !spline2.IsNull() && !spline3.IsNull() && !spline4.IsNull()) {
        std::vector<Line> profiles = makeIsoCurves(spline1, spline3, Profiles.getValue());  // NOLINT
        for (const auto& line : profiles) {
            approximate(line.p1, line.p2);
        }

        std::vector<Line> guides = makeIsoCurves(spline2, spline4, Guides.getValue());  // NOLINT
        for (const auto& line : guides) {
            approximate(line.p1, line.p2);
        }
    }

    Shape.setValue(comp);

    return App::DocumentObject::StdReturn;
}
