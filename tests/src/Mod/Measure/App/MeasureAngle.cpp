// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>

#include <algorithm>
#include <cmath>

#include <App/Datums.h>
#include <App/Document.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Part/App/Datums.h>
#include <Mod/Part/App/PartFeature.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Precision.hxx>
#include <gp_Lin.hxx>
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

    App::Document* getDocument() const
    {
        return document;
    }

private:
    App::Document* document {};
};

TEST_F(MeasureAngle, testAngleBetweenLcsAxes)
{
    App::Document* doc = getDocument();
    auto lcs = doc->addObject<App::LocalCoordinateSystem>("LCS");
    doc->recompute();

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(lcs, {"X_Axis"});
    measure->Element2.setValue(lcs, {"Y_Axis"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 90.0, Base::toDegrees(Precision::Angular()));
}

TEST_F(MeasureAngle, testLcsAxisMatchesPlaneNormal)
{
    App::Document* doc = getDocument();
    auto lcs = doc->addObject<App::LocalCoordinateSystem>("LCS");
    lcs->Placement.setValue(
        Base::Placement(Base::Vector3d(3.0, 4.0, 5.0), Base::Rotation(Base::Vector3d(1.0, 2.0, 3.0), 0.7))
    );
    doc->recompute();

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(lcs, {"XY_Plane"});
    measure->Element2.setValue(lcs, {"Z_Axis"});
    doc->recompute();

    const double angle = measure->Angle.getValue();
    const double deviation = std::min(std::abs(angle), std::abs(std::abs(angle) - 180.0));
    EXPECT_NEAR(deviation, 0.0, Base::toDegrees(Precision::Angular()));
}

TEST_F(MeasureAngle, testDatumLineMatchesPlaneNormal)
{
    App::Document* doc = getDocument();
    auto line = doc->addObject<Part::DatumLine>("DatumLine");
    auto plane = doc->addObject<Part::DatumPlane>("DatumPlane");
    doc->recompute();

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(plane);
    measure->Element2.setValue(line);
    doc->recompute();

    const double angle = measure->Angle.getValue();
    const double deviation = std::min(std::abs(angle), std::abs(std::abs(angle) - 180.0));
    EXPECT_NEAR(deviation, 0.0, Base::toDegrees(Precision::Angular()));
}

TEST_F(MeasureAngle, testNonDatumInfiniteLineUsesShapeDirection)
{
    App::Document* doc = getDocument();
    auto xLine = doc->addObject<Part::Feature>("XLine");
    TopoDS_Edge xEdge
        = BRepBuilderAPI_MakeEdge(gp_Lin(gp_Pnt(1.0, 2.0, 3.0), gp_Dir(1.0, 0.0, 0.0))).Edge();
    xEdge.Infinite(Standard_True);
    xLine->Shape.setValue(xEdge);

    auto yLine = doc->addObject<Part::Feature>("YLine");
    TopoDS_Edge yEdge
        = BRepBuilderAPI_MakeEdge(gp_Lin(gp_Pnt(1.0, 2.0, 3.0), gp_Dir(0.0, 1.0, 0.0))).Edge();
    yEdge.Infinite(Standard_True);
    yLine->Shape.setValue(yEdge);
    doc->recompute();

    auto measure = doc->addObject<Measure::MeasureAngle>("Angle");
    measure->Element1.setValue(xLine, {"Edge1"});
    measure->Element2.setValue(yLine, {"Edge1"});
    doc->recompute();

    EXPECT_NEAR(measure->Angle.getValue(), 90.0, Base::toDegrees(Precision::Angular()));
}
