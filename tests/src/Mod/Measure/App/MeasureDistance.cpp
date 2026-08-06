// SPDX-License-Identifier: LGPL-2.1-or-later

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <App/Part.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/MeasureManager.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Part/App/PartFeature.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <gtest/gtest.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <gp_Circ.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

class MeasureDistance: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().newDocument("Measure");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::Document* getDocument() const
    {
        return document;
    }

    TopoDS_Edge makeCircle(const gp_Pnt& pnt) const
    {
        gp_Circ circle;
        circle.SetLocation(pnt);
        circle.SetRadius(1.0);
        BRepBuilderAPI_MakeEdge mkEdge(circle);
        return mkEdge.Edge();
    }

private:
    App::Document* document {};
};

// NOLINTBEGIN
TEST_F(MeasureDistance, testCurvedFaceValidSelection)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/29235
    App::Document* doc = getDocument();

    auto sphere = doc->addObject<Part::Feature>("Sphere");
    sphere->Shape.setValue(BRepPrimAPI_MakeSphere(5.0).Solid());

    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(20.0, 0.0, 0.0), 10.0, 10.0, 10.0).Solid());

    doc->recompute();

    App::MeasureSelectionItem item1 {App::SubObjectT {sphere, "Face1"}, Base::Vector3d {}};
    App::MeasureSelectionItem item2 {App::SubObjectT {box, "Face1"}, Base::Vector3d {}};

    EXPECT_TRUE(Measure::MeasureDistance::isValidSelection({item1, item2}));
}

TEST_F(MeasureDistance, testCurvedFaceDistance)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/29235
    App::Document* doc = getDocument();

    auto sphere = doc->addObject<Part::Feature>("Sphere");
    sphere->Shape.setValue(BRepPrimAPI_MakeSphere(5.0).Solid());

    auto box = doc->addObject<Part::Feature>("Box");
    box->Shape.setValue(BRepPrimAPI_MakeBox(gp_Pnt(20.0, 0.0, 0.0), 10.0, 10.0, 10.0).Solid());

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(sphere, {"Face1"});
    md->Element2.setValue(box, {"Face1"});

    doc->recompute();

    EXPECT_NEAR(md->Distance.getValue(), 15.0, 1e-6);
}

TEST_F(MeasureDistance, testCircleCircle)
{
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(3.0, 4.0, 0.0)));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 3.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 4.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 0.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(0.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(3.0, 4.0, 0.0));
}

TEST_F(MeasureDistance, testCircleCircleWithPlacement)
{
    // Baseline for issue #30365: distance and the reported positions must respect
    // each object's own top-level Placement. Worked *before* #30423.
    App::Document* doc = getDocument();
    auto p1 = doc->addObject<Part::Feature>("Shape1");
    p1->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    p1->Placement.setValue(Base::Placement(Base::Vector3d(10.0, 0.0, 0.0), Base::Rotation()));
    auto p2 = doc->addObject<Part::Feature>("Shape2");
    p2->Shape.setValue(makeCircle(gp_Pnt(0.0, 0.0, 0.0)));
    p2->Placement.setValue(Base::Placement(Base::Vector3d(10.0, 0.0, 5.0), Base::Rotation()));

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(p1, {"Edge1"});
    md->Element2.setValue(p2, {"Edge1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 5.0);
    EXPECT_EQ(md->Position1.getValue(), Base::Vector3d(10.0, 0.0, 0.0));
    EXPECT_EQ(md->Position2.getValue(), Base::Vector3d(10.0, 0.0, 5.0));
}

TEST_F(MeasureDistance, testTwoBoxesMovedByContainers)
{
    // Regression: https://github.com/FreeCAD/FreeCAD/issues/30365
    // Mirrors the issue report: two bodies built at the origin and then moved. Requires #30423 fix.
    App::Document* doc = getDocument();

    auto box1 = doc->addObject<Part::Feature>("Box1");
    box1->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    auto container1 = doc->addObject<App::Part>("Container1");
    container1->getExtensionByType<App::GeoFeatureGroupExtension>()->addObject(box1);

    auto box2 = doc->addObject<Part::Feature>("Box2");
    box2->Shape.setValue(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Solid());
    auto container2 = doc->addObject<App::Part>("Container2");
    container2->Placement.setValue(Base::Placement(Base::Vector3d(50.0, 0.0, 0.0), Base::Rotation()));
    container2->getExtensionByType<App::GeoFeatureGroupExtension>()->addObject(box2);

    auto md = doc->addObject<Measure::MeasureDistance>("Distance");
    md->Element1.setValue(container1, {"Box1.Vertex1"});
    md->Element2.setValue(container2, {"Box2.Vertex1"});

    doc->recompute();

    EXPECT_DOUBLE_EQ(md->Distance.getValue(), 50.0);
    EXPECT_DOUBLE_EQ(md->DistanceX.getValue(), 50.0);
    EXPECT_DOUBLE_EQ(md->DistanceY.getValue(), 0.0);
    EXPECT_DOUBLE_EQ(md->DistanceZ.getValue(), 0.0);
}
// NOLINTEND
