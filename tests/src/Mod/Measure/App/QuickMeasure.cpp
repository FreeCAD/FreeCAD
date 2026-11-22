// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Mod/Measure/App/Measurement.h>
#include <Mod/Part/App/PartFeature.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <gtest/gtest.h>

class QuickMeasureTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("QuickMeasure");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    Part::Feature* makeDisc(const char* name, const gp_Pnt& center) const
    {
        gp_Circ circle(gp_Ax2(center, gp_Dir(0.0, 0.0, 1.0)), 5.0);
        TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circle).Edge();
        TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edge).Wire();

        auto feature = document->addObject<Part::Feature>(name);
        feature->Shape.setValue(BRepBuilderAPI_MakeFace(wire).Face());
        return feature;
    }

    Part::Feature* makePlane(const char* name, double z) const
    {
        gp_Pln plane(gp_Pnt(0.0, 0.0, z), gp_Dir(0.0, 0.0, 1.0));

        auto feature = document->addObject<Part::Feature>(name);
        feature->Shape.setValue(BRepBuilderAPI_MakeFace(plane, -5.0, 5.0, -5.0, 5.0).Face());
        return feature;
    }

private:
    App::Document* document {};
};

// NOLINTBEGIN(readability-magic-numbers)
TEST_F(QuickMeasureTest, SingleDiscKeepsDiscType)
{
    Measure::Measurement measurement;
    measurement.addReference3D(makeDisc("Disc", gp_Pnt(0.0, 0.0, 0.0)), "Face1");

    EXPECT_EQ(measurement.getType(), Measure::MeasureType::Disc);
}

TEST_F(QuickMeasureTest, ParallelDiscsHaveNominalDistance)
{
    Measure::Measurement measurement;
    measurement.addReference3D(makeDisc("Disc1", gp_Pnt(0.0, 0.0, 0.0)), "Face1");
    measurement.addReference3D(makeDisc("Disc2", gp_Pnt(3.0, 0.0, 7.5)), "Face1");

    EXPECT_EQ(measurement.getType(), Measure::MeasureType::TwoDiscs);
    EXPECT_NEAR(measurement.planePlaneDistance(), 7.5, Precision::Confusion());
    EXPECT_NEAR(measurement.discAxisDistance(), 3.0, Precision::Confusion());
}

TEST_F(QuickMeasureTest, ParallelDiscAndPlaneHaveNominalDistance)
{
    Measure::Measurement measurement;
    measurement.addReference3D(makeDisc("Disc", gp_Pnt(0.0, 0.0, 2.0)), "Face1");
    measurement.addReference3D(makePlane("Plane", 10.0), "Face1");

    EXPECT_EQ(measurement.getType(), Measure::MeasureType::TwoPlanes);
    EXPECT_NEAR(measurement.planePlaneDistance(), 8.0, Precision::Confusion());
}
// NOLINTEND(readability-magic-numbers)
