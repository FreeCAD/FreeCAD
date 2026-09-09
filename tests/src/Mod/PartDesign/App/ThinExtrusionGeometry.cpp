// SPDX-License-Identifier: LGPL-2.1-or-later
#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include <Mod/PartDesign/App/ThinExtrusionGeometry.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomConvert.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopoDS.hxx>
#include <gp_Circ.hxx>

TEST(ThinExtrusionGeometry, PeriodicC2StopsInsideFirstBarrier)
{
    tests::initApplication();
    const Part::TopoShape source(
        BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(), gp_Dir(0, 0, 1)), 10), M_PI / 3, 2 * M_PI / 3)
            .Edge()
    );
    const Part::TopoShape left(BRepPrimAPI_MakeBox(gp_Pnt(-12, 0, -10), 4, 15, 20).Shape());
    const Part::TopoShape right(BRepPrimAPI_MakeBox(gp_Pnt(8, 0, -10), 4, 15, 20).Shape());
    const Part::TopoShape floor(BRepPrimAPI_MakeBox(gp_Pnt(-12, -3, -10), 24, 3, 20).Shape());
    const auto body = Part::TopoShape().makeElementFuse({left, right, floor});
    const auto extended = PartDesign::extendThinProfile(source, body, true, 0, true);
    for (const auto& edge : extended.getSubTopoShapes(TopAbs_EDGE)) {
        double first, last;
        auto curve = BRep_Tool::Curve(TopoDS::Edge(edge.getShape()), first, last);
        EXPECT_GT(first, 0);
        EXPECT_LT(last, M_PI);
        EXPECT_TRUE(edge.makeElementPrism(gp_Vec(0, -30, 0)).isValid()) << first << " " << last;
    }
}

TEST(ThinExtrusionGeometry, C2EndpointJetsAndSourcePreservation)
{
    tests::initApplication();
    TColgp_Array1OfPnt poles(1, 4);
    poles(1) = gp_Pnt(8, 30, 0);
    poles(2) = gp_Pnt(12, 25, 0);
    poles(3) = gp_Pnt(25, 15, 0);
    poles(4) = gp_Pnt(38, 10, 0);
    Handle(Geom_Curve) curve = GeomConvert::CurveToBSplineCurve(new Geom_BezierCurve(poles));
    const Part::TopoShape source(BRepBuilderAPI_MakeEdge(curve).Edge());
    const Part::TopoShape wall(BRepPrimAPI_MakeBox(gp_Pnt(0, -3, -5), 3, 50, 10).Shape());
    const Part::TopoShape floor(BRepPrimAPI_MakeBox(gp_Pnt(0, -3, -5), 80, 3, 10).Shape());
    const auto body = Part::TopoShape().makeElementCompound({wall, floor});
    const auto extended = PartDesign::extendThinProfile(source, body, true, 0);
    bool retained = false;
    for (const auto& edge : extended.getSubTopoShapes(TopAbs_EDGE)) {
        retained |= edge.getShape().IsSame(source.getShape());
    }
    EXPECT_TRUE(retained);
    for (const double parameter : {0., 1.}) {
        gp_Pnt point;
        gp_Vec d1, d2;
        curve->D2(parameter, point, d1, d2);
        if (parameter == 0.) {
            d1.Reverse();
        }
        bool found = false;
        for (const auto& edge : extended.getSubTopoShapes(TopAbs_EDGE)) {
            if (edge.getShape().IsSame(source.getShape())) {
                continue;
            }
            double first, last;
            auto continuation = BRep_Tool::Curve(TopoDS::Edge(edge.getShape()), first, last);
            for (const double join : {first, last}) {
                gp_Pnt at;
                gp_Vec v, a;
                continuation->D2(join, at, v, a);
                if (at.Distance(point) > 1e-7) {
                    continue;
                }
                if (join == last) {
                    v.Reverse();
                }
                const double scale = v.Magnitude() / d1.Magnitude();
                EXPECT_LT((v - d1 * scale).Magnitude(), 1e-7);
                EXPECT_LT((a - d2 * scale * scale).Magnitude(), 1e-7);
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
}
