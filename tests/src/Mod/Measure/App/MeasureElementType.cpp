// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Geom_BezierSurface.hxx>
#include <Precision.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Mod/Measure/App/MeasureAngle.h>
#include <Mod/Measure/App/MeasureArea.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Part/App/Datums.h>
#include <Mod/Part/App/PartFeature.h>
#include <src/App/InitApplication.h>

class MeasureElementType: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("MeasureElementType");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::Document* document {};
};

TEST_F(MeasureElementType, testBoundedPlaneIsPlaneSegment)
{
    auto box = document->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    document->recompute();

    App::MeasureSelectionItem face {App::SubObjectT {box, "Face1"}, Base::Vector3d {}};

    EXPECT_EQ(App::MeasureManager::getMeasureElementType(face), App::MeasureElementType::PLANESEGMENT);
}

TEST_F(MeasureElementType, testUnboundedPlaneIsPlane)
{
    auto plane = document->addObject<Part::DatumPlane>("Plane");
    document->recompute();

    App::MeasureSelectionItem datum {App::SubObjectT {plane, ""}, Base::Vector3d {}};

    EXPECT_EQ(App::MeasureManager::getMeasureElementType(datum), App::MeasureElementType::PLANE);
}

TEST_F(MeasureElementType, testBoundedPlanarBezierFaceIsPlaneSegment)
{
    TColgp_Array2OfPnt poles(1, 2, 1, 2);
    poles.SetValue(1, 1, gp_Pnt(0.0, 0.0, 0.0));
    poles.SetValue(1, 2, gp_Pnt(0.0, 10.0, 0.0));
    poles.SetValue(2, 1, gp_Pnt(10.0, 0.0, 0.0));
    poles.SetValue(2, 2, gp_Pnt(10.0, 10.0, 0.0));

    auto plane = document->addObject<Part::Feature>("BezierPlane");
    Handle(Geom_BezierSurface) surface = new Geom_BezierSurface(poles);
    plane->Shape.setValue(BRepBuilderAPI_MakeFace(surface, Precision::Confusion()).Face());
    document->recompute();

    App::MeasureSelectionItem face {App::SubObjectT {plane, "Face1"}, Base::Vector3d {}};

    EXPECT_EQ(App::MeasureManager::getMeasureElementType(face), App::MeasureElementType::PLANESEGMENT);
}

TEST_F(MeasureElementType, testDiscRemainsDisc)
{
    auto disc = document->addObject<Part::Feature>("Disc");
    TopoDS_Edge circle = BRepBuilderAPI_MakeEdge(gp_Circ(gp::XOY(), 5.0)).Edge();
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(circle).Wire();
    disc->Shape.setValue(BRepBuilderAPI_MakeFace(wire).Face());
    document->recompute();

    App::MeasureSelectionItem face {App::SubObjectT {disc, "Face1"}, Base::Vector3d {}};

    EXPECT_EQ(App::MeasureManager::getMeasureElementType(face), App::MeasureElementType::DISC);
}

TEST_F(MeasureElementType, testPlaneSegmentMeasurementSupport)
{
    auto box = document->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    document->recompute();

    App::MeasureSelectionItem face1 {App::SubObjectT {box, "Face1"}, Base::Vector3d {}};
    App::MeasureSelectionItem face2 {App::SubObjectT {box, "Face2"}, Base::Vector3d {}};

    EXPECT_TRUE(Measure::MeasureArea::isValidSelection({face1}));
    EXPECT_TRUE(Measure::MeasureAngle::isValidSelection({face1, face2}));
    EXPECT_TRUE(Measure::MeasureDistance::isValidSelection({face1, face2}));
}

TEST_F(MeasureElementType, testPlaneMeasurementSupport)
{
    auto plane1 = document->addObject<Part::DatumPlane>("Plane1");
    auto plane2 = document->addObject<Part::DatumPlane>("Plane2");
    document->recompute();

    App::MeasureSelectionItem datum1 {App::SubObjectT {plane1, ""}, Base::Vector3d {}};
    App::MeasureSelectionItem datum2 {App::SubObjectT {plane2, ""}, Base::Vector3d {}};

    EXPECT_FALSE(Measure::MeasureArea::isValidSelection({datum1}));
    EXPECT_TRUE(Measure::MeasureAngle::isValidSelection({datum1, datum2}));
    EXPECT_TRUE(Measure::MeasureDistance::isValidSelection({datum1, datum2}));
}
