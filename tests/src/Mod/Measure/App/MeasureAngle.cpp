// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <cmath>

#include <src/App/InitApplication.h>

#include <App/Document.h>
#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Part/App/PartFeature.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Precision.hxx>
#include <gp_Pln.hxx>
#include <gtest/gtest.h>

class MeasureAngle: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("MeasureAngle");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::Document* document {};
};

TEST_F(MeasureAngle, EdgeParallelToFaceNormal)
{
    auto face = document->addObject<Part::Feature>("Face");
    face->Shape.setValue(BRepBuilderAPI_MakeFace(
                             gp_Pln(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)),
                             -10.0,
                             10.0,
                             -10.0,
                             10.0
    )
                             .Face());

    auto edge = document->addObject<Part::Feature>("Edge");
    edge->Shape.setValue(BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 5.0), gp_Pnt(0.0, 0.0, 10.0)).Edge());

    auto measure = document->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(face, {"Face1"});
    measure->Element2.setValue(edge, {"Edge1"});
    document->recompute();

    const double angle = measure->Angle.getValue();
    // Allow for 0, 180, or -180
    const double deviation = std::min(std::abs(angle), std::abs(std::abs(angle) - 180.0));
    EXPECT_NEAR(deviation, 0.0, Precision::Angular());
}
