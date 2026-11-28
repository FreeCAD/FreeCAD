// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include <App/Application.h>
#include <App/Document.h>
#include "Mod/Part/App/FaceMakerBuildFace.h"
#include "Mod/Part/App/TopoShape.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>

#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

using namespace Part;

namespace
{

using LooseEdgePolicy = FaceMakerBuildFace::LooseEdgePolicy;

TopoDS_Wire lineWire(gp_Pnt from, gp_Pnt to)
{
    return BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(from, to).Edge()).Wire();
}

TopoDS_Wire lineWire(double x0, double y0, double x1, double y1)
{
    return lineWire(gp_Pnt(x0, y0, 0), gp_Pnt(x1, y1, 0));
}

TopoDS_Wire circleWire(double radius)
{
    gp_Circ circle(gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), radius);
    return BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(circle).Edge()).Wire();
}

TopoDS_Wire squareWire(double side)
{
    BRepBuilderAPI_MakeWire wire;
    wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(side, 0, 0)).Edge());
    wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(side, 0, 0), gp_Pnt(side, side, 0)).Edge());
    wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(side, side, 0), gp_Pnt(0, side, 0)).Edge());
    wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(0, side, 0), gp_Pnt(0, 0, 0)).Edge());
    return wire.Wire();
}

double area(const TopoShape& shape)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(shape.getShape(), props);
    return props.Mass();
}

std::vector<TopoShape> looseEdges(const TopoShape& shape)
{
    return shape.getSubTopoShapes(TopAbs_EDGE, TopAbs_FACE);
}

double length(const std::vector<TopoShape>& edges)
{
    double sum = 0;
    for (const auto& edge : edges) {
        GProp_GProps props;
        BRepGProp::LinearProperties(edge.getShape(), props);
        sum += props.Mass();
    }
    return sum;
}

}  // namespace

class FaceMakerBuildFaceTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _doc = App::GetApplication().getDocument(_docName.c_str());
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    std::vector<TopoShape> sources(const std::vector<TopoDS_Wire>& wires)
    {
        long tag = 1;
        std::vector<TopoShape> shapes;
        for (const auto& wire : wires) {
            TopoShape shape(tag++, _doc->getStringHasher());
            shape.setShape(wire);
            shape.mapSubElement(shape);
            shapes.push_back(std::move(shape));
        }
        return shapes;
    }

    TopoShape build(LooseEdgePolicy policy, const std::vector<TopoDS_Wire>& wires)
    {
        FaceMakerBuildFace maker;
        maker.setLooseEdgePolicy(policy);
        TopoShape result(100, _doc->getStringHasher());
        result.makeElementFace(sources(wires), maker);
        return result;
    }

    std::string _docName;
    App::Document* _doc = nullptr;
};

TEST_F(FaceMakerBuildFaceTest, looseEdgePolicy)
{
    struct Case
    {
        const char* name;
        LooseEdgePolicy policy;
        std::vector<TopoDS_Wire> wires;
        std::size_t faceCount;
        double faceArea;
        std::size_t looseEdgeCount;
        double looseEdgeLength;
    };
    const auto keep = LooseEdgePolicy::Keep;
    const double pi = std::numbers::pi;
    const TopoDS_Wire upright = lineWire(gp_Pnt(20, 0, 0), gp_Pnt(20, 0, 10));
    const std::vector<Case> cases {
        // The disc, and the separate line, 10 long, only when kept.
        {"separate line", keep, {circleWire(10), lineWire(20, -5, 20, 5)}, 1, 100 * pi, 1, 10},
        {"separate line dropped",
         LooseEdgePolicy::Drop,
         {circleWire(10), lineWire(20, -5, 20, 5)},
         1,
         100 * pi,
         0,
         0},
        // A line inside a face bounds nothing: it stays loose, 4 * sqrt(2) long.
        {"floating line", keep, {squareWire(10), lineWire(3, 3, 7, 7)}, 1, 100, 1, 4 * std::numbers::sqrt2},
        // The chord splits the disc in two; the stubs outside it, 5 long each, stay loose.
        {"line through", keep, {circleWire(10), lineWire(-15, 0, 15, 0)}, 2, 100 * pi, 2, 10},
        // y = 0, y = 2x and y = 20 - 2x bound the triangle (0,0) (10,0) (5,10), area 50. Their
        // six overshoots are 2, 2 and four of sqrt(1^2 + 2^2) = sqrt(5).
        {"overshooting lines",
         keep,
         {lineWire(-2, 0, 12, 0), lineWire(-1, -2, 6, 12), lineWire(11, -2, 4, 12)},
         1,
         50,
         6,
         4 + 4 * std::sqrt(5.0)},
        // No face: the crossing splits both lines at their midpoints.
        {"crossing lines", keep, {lineWire(-5, 0, 5, 0), lineWire(0, -5, 0, 5)}, 0, 0, 4, 20},
        {"open line alone", keep, {lineWire(0, 0, 10, 0)}, 0, 0, 1, 10},
        // No plane holds both: every edge is loose.
        {"off plane", keep, {circleWire(10), upright}, 0, 0, 2, 20 * pi + 10},
        // Every edge bounds a face.
        {"concentric circles", keep, {circleWire(10), circleWire(5)}, 2, 100 * pi, 0, 0},
    };

    for (const auto& c : cases) {
        SCOPED_TRACE(c.name);
        auto result = build(c.policy, c.wires);
        auto loose = looseEdges(result);
        EXPECT_EQ(result.getSubTopoShapes(TopAbs_FACE).size(), c.faceCount);
        EXPECT_NEAR(area(result), c.faceArea, 1e-6);
        EXPECT_EQ(loose.size(), c.looseEdgeCount);
        EXPECT_NEAR(length(loose), c.looseEdgeLength, 1e-6);

        // Split pieces are named through the splitter history, the same way on every build.
        auto again = build(c.policy, c.wires);
        const int edgeCount = result.countSubShapes(TopAbs_EDGE);
        ASSERT_EQ(edgeCount, again.countSubShapes(TopAbs_EDGE));
        for (int i = 1; i <= edgeCount; ++i) {
            auto indexed = Data::IndexedName::fromConst("Edge", i);
            auto mapped = result.getMappedName(indexed);
            EXPECT_TRUE(mapped) << "Edge" << i << " has no mapped name";
            EXPECT_EQ(mapped, again.getMappedName(indexed)) << "Edge" << i << " name differs";
        }
    }
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
