// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Part/App/PartFeatures.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

using namespace Part;
using namespace PartTestHelpers;

class PartFeaturesTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
    }

    void TearDown() override
    {}
};

TEST_F(PartFeaturesTest, testRuledSurface)
{
    // Arrange
    auto _edge1 = _doc->addObject<Line>();
    auto _edge2 = _doc->addObject<Line>();
    _edge1->X1.setValue(0);
    _edge1->Y1.setValue(0);
    _edge1->Z1.setValue(0);
    _edge1->X2.setValue(2);
    _edge1->Y2.setValue(0);
    _edge1->Z2.setValue(0);
    _edge2->X1.setValue(0);
    _edge2->Y1.setValue(2);
    _edge2->Z1.setValue(0);
    _edge2->X2.setValue(2);
    _edge2->Y2.setValue(2);
    _edge2->Z2.setValue(0);
    auto _ruled = _doc->addObject<RuledSurface>();
    _ruled->Curve1.setValue(_edge1);
    _ruled->Curve2.setValue(_edge2);
    // Act
    _ruled->execute();
    TopoShape ts = _ruled->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    // Assert shape is correct
    EXPECT_DOUBLE_EQ(volume, 0.0);
    EXPECT_DOUBLE_EQ(area, 4.0);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 2, 2, 0)));
    // Assert element map is correct
    EXPECT_EQ(9, elementMap.size());
}

TEST_F(PartFeaturesTest, testLoft)
{
    // Arrange
    auto _plane1 = _doc->addObject<Plane>();
    _plane1->Length.setValue(4);
    _plane1->Width.setValue(4);
    auto _plane2 = _doc->addObject<Plane>();
    _plane2->Length.setValue(4);
    _plane2->Width.setValue(4);
    _plane2->Placement.setValue(Base::Placement(Base::Vector3d(0, 0, 2), Base::Rotation()));
    auto _loft = _doc->addObject<Loft>();
    _loft->Sections.setValues({_plane1, _plane2});
    _loft->Solid.setValue((true));
    // Act
    _loft->execute();
    TopoShape ts = _loft->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    // Assert shape is correct
    EXPECT_DOUBLE_EQ(volume, 32.0);
    EXPECT_DOUBLE_EQ(area, 64.0);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 4, 4, 2)));
    // Assert element map is correct
    EXPECT_EQ(26, elementMap.size());
}

TEST_F(PartFeaturesTest, testSweep)
{
    // Arrange
    auto _edge1 = _doc->addObject<Line>();
    _edge1->X1.setValue(0);
    _edge1->Y1.setValue(0);
    _edge1->Z1.setValue(0);
    _edge1->X2.setValue(0);
    _edge1->Y2.setValue(0);
    _edge1->Z2.setValue(3);
    auto _plane1 = _doc->addObject<Plane>();
    _plane1->Length.setValue(4);
    _plane1->Width.setValue(4);
    auto _sweep = _doc->addObject<Sweep>();
    _sweep->Sections.setValues({_plane1});
    _sweep->Spine.setValue(_edge1);
    _sweep->Solid.setValue((false));
    // Act
    _sweep->execute();
    TopoShape ts = _sweep->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    // Assert shape is correct
    EXPECT_DOUBLE_EQ(volume, 32.0);
    EXPECT_DOUBLE_EQ(area, 48.0);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 4, 4, 3)));
    // Assert element map is correct
    EXPECT_EQ(24, elementMap.size());
}

TEST_F(PartFeaturesTest, testThickness)
{
    // Arrange
    auto _thickness = _doc->addObject<Thickness>();
    _thickness->Faces.setValue(_boxes[0], {"Face1"});
    _thickness->Value.setValue(0.25);
    _thickness->Join.setValue("Intersection");
    // Act
    _thickness->execute();
    TopoShape ts = _thickness->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    // Assert shape is correct
    EXPECT_DOUBLE_EQ(volume, 4.9375);
    EXPECT_DOUBLE_EQ(area, 42.5);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, -0.25, -0.25, 1.25, 2.25, 3.25)));
    // Assert element map is correct
    EXPECT_EQ(51, elementMap.size());
}

TEST_F(PartFeaturesTest, testRefine)
{
    // Arrange
    auto _fuse = _doc->addObject<Part::Fuse>();
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[3]);
    _fuse->execute();
    Part::TopoShape fusedts = _fuse->Shape.getShape();
    auto _refine = _doc->addObject<Refine>();
    _refine->Source.setValue(_fuse);
    // Act
    _refine->execute();
    TopoShape ts = _refine->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    auto edges = fusedts.getSubTopoShapes(TopAbs_EDGE);
    auto refinedEdges = ts.getSubTopoShapes(TopAbs_EDGE);
    // Assert shape is correct
    EXPECT_EQ(edges.size(), 20);
    EXPECT_EQ(refinedEdges.size(), 12);
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_DOUBLE_EQ(area, 38.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 1, 4, 3)));
    // Assert element map is correct
    EXPECT_EQ(0, elementMap.size());  // TODO: Expect this to be non-zero.
}

TEST_F(PartFeaturesTest, testReverse)
{
    // Arrange
    auto _reverse = _doc->addObject<Reverse>();
    _reverse->Source.setValue(_boxes[0]);
    // Act
    _reverse->execute();
    TopoShape ts = _reverse->Shape.getShape();
    double volume = getVolume(ts.getShape());
    double area = getArea(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    auto elementMap = ts.getElementMap();
    auto faces = ts.getSubTopoShapes(TopAbs_FACE);
    auto originalFaces = _boxes[0]->Shape.getShape().getSubTopoShapes(TopAbs_FACE);
    // Assert shape is correct
    EXPECT_EQ(faces[0].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_EQ(faces[1].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(faces[2].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_EQ(faces[3].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(faces[4].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_EQ(faces[5].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(originalFaces[0].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(originalFaces[1].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_EQ(originalFaces[2].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(originalFaces[3].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_EQ(originalFaces[4].getShape().Orientation(), TopAbs_REVERSED);
    EXPECT_EQ(originalFaces[5].getShape().Orientation(), TopAbs_FORWARD);
    EXPECT_DOUBLE_EQ(volume, 6.0);
    EXPECT_DOUBLE_EQ(area, 22.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 1, 2, 3)));
    // Assert element map is correct
    EXPECT_EQ(0, elementMap.size());  // TODO: Expect this to be non-zero.
}
