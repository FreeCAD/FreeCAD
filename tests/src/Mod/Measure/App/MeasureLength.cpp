// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Measure/App/MeasureLength.h>
#include <Mod/Part/App/MeasureClient.h>
#include <Mod/Part/App/MeasureInfo.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <gtest/gtest.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <numbers>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class MeasureLength: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        // The label placement comes from the Part measure-info handlers; register them
        // here (as module load does) so it resolves headless.
        Part::MeasureClient::initialize();
        for (auto& entry : Part::MeasureClient::reportLengthCB()) {
            Measure::MeasureBaseExtendable<Part::MeasureLengthInfo>::addGeometryHandler(
                entry.m_module,
                entry.m_callback
            );
        }
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("MeasureLengthTest");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    Part::Feature* addFeature(const char* name, const TopoDS_Shape& shape) const
    {
        auto feature = document->addObject<Part::Feature>(name);
        feature->Shape.setValue(shape);
        return feature;
    }

    Measure::MeasureLength* measureEdge(Part::Feature* feature) const
    {
        auto measure = document->addObject<Measure::MeasureLength>("Length");
        measure->Elements.setValues({feature}, {"Edge1"});
        document->recompute();
        return measure;
    }

private:
    App::Document* document {};
};

// A quarter arc's label sits on the arc at 45 degrees, not at its center of mass.
TEST_F(MeasureLength, testArcLabelAtMidpoint)
{
    const gp_Circ circle(gp_Ax2(), 10.0);
    auto arc = addFeature("Arc", BRepBuilderAPI_MakeEdge(circle, 0.0, std::numbers::pi / 2.0).Edge());

    auto measure = measureEdge(arc);
    const Base::Vector3d position = measure->getPlacement().getPosition();
    const double expected = 10.0 / std::numbers::sqrt2;

    EXPECT_NEAR(position.x, expected, Precision::Confusion());
    EXPECT_NEAR(position.y, expected, Precision::Confusion());
    EXPECT_NEAR(position.z, 0.0, Precision::Confusion());
}

// A straight line's label stays at its middle.
TEST_F(MeasureLength, testLineLabelAtMidpoint)
{
    auto line = addFeature(
        "Line",
        BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(10.0, 4.0, 2.0)).Edge()
    );

    auto measure = measureEdge(line);
    const Base::Vector3d position = measure->getPlacement().getPosition();

    EXPECT_NEAR(position.x, 5.0, Precision::Confusion());
    EXPECT_NEAR(position.y, 2.0, Precision::Confusion());
    EXPECT_NEAR(position.z, 1.0, Precision::Confusion());
}

// The label follows the feature placement.
TEST_F(MeasureLength, testPlacedArcLabelAtMidpoint)
{
    const gp_Circ circle(gp_Ax2(), 5.0);
    auto arc = addFeature("Arc", BRepBuilderAPI_MakeEdge(circle, 0.0, std::numbers::pi).Edge());
    arc->Placement.setValue(
        Base::Placement(
            Base::Vector3d(20.0, -3.0, 7.0),
            Base::Rotation(Base::Vector3d(1.0, 0.0, 0.0), std::numbers::pi / 3.0)
        )
    );

    auto measure = measureEdge(arc);
    const Base::Vector3d position = measure->getPlacement().getPosition();

    // Local midpoint (0, 5, 0) rotated 60 degrees about X, then translated.
    EXPECT_NEAR(position.x, 20.0, Precision::Confusion());
    EXPECT_NEAR(position.y, -0.5, Precision::Confusion());
    EXPECT_NEAR(position.z, 7.0 + 5.0 * std::numbers::sqrt3 / 2.0, Precision::Confusion());
}

// A degenerated edge (a sphere pole) has no curve to walk along and must still measure.
TEST_F(MeasureLength, testDegeneratedEdgeStillMeasures)
{
    TopoDS_Edge pole;
    const TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(1.0).Shape();
    for (TopExp_Explorer exp(sphere, TopAbs_EDGE); exp.More(); exp.Next()) {
        if (BRep_Tool::Degenerated(TopoDS::Edge(exp.Current()))) {
            pole = TopoDS::Edge(exp.Current());
            break;
        }
    }
    ASSERT_FALSE(pole.IsNull());

    auto feature = addFeature("Pole", pole);
    auto measure = measureEdge(feature);

    EXPECT_FALSE(measure->isError());
    EXPECT_NO_THROW(measure->getPlacement());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
