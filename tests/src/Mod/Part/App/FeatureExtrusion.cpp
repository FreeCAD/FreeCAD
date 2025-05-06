// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <Base/Tools.h>
#include "Mod/Part/App/FeatureExtrusion.h"
#include <src/App/InitApplication.h>

#include "BRepBuilderAPI_MakeEdge.hxx"

#include "PartTestHelpers.h"

class FeatureExtrusionTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _extrusion = _doc->addObject<Part::Extrusion>();
        PartTestHelpers::rectangle(len, wid, "Rect1");
        _extrusion->Base.setValue(_doc->getObjects().back());
        _extrusion->LengthFwd.setValue(ext1);
    }

    void TearDown() override
    {}

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    Part::Extrusion* _extrusion = nullptr;
    // Arbtitrary constants for testing.  Named here for clarity.
    const double len = 3.0;
    const double wid = 4.0;
    const double ext1 = 10.0;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

TEST_F(FeatureExtrusionTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
    // Act
    _extrusion->Base.setValue(_extrusion->Base.getValue());
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
    // Act
    _extrusion->Solid.setValue(Standard_True);
    // Assert
    EXPECT_TRUE(_extrusion->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_extrusion->mustExecute());
}

TEST_F(FeatureExtrusionTest, testGetProviderName)
{
    // Act
    _extrusion->execute();
    const char* name = _extrusion->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderExtrusion");
}

// Not clear if there is test value in this one.

TEST_F(FeatureExtrusionTest, testFetchAxisLink)
{
    // static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
    //                           Base::Vector3d& basepoint,
    //                           Base::Vector3d& dir);
}

// Filling in these next two tests seems very redundant, since they are used in execute()
// and thus tested by the results there.  In the event that ever went funny, then maybe
// implementation here would make sense.

TEST_F(FeatureExtrusionTest, testExtrudeShape)
{
    // static TopoShape extrudeShape(const TopoShape& source, const ExtrusionParameters& params);
}

TEST_F(FeatureExtrusionTest, testComputeFinalParameters)
{
    // ExtrusionParameters computeFinalParameters();
}

TEST_F(FeatureExtrusionTest, testExecuteSimple)
{
    // Arrange
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteSimpleRev)
{
    const double ext2 = 9;
    // Arrange
    _extrusion->LengthFwd.setValue(0);
    _extrusion->LengthRev.setValue(ext2);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext2);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext2, len, wid, 0)));
}

TEST_F(FeatureExtrusionTest, testExecuteSolid)
{
    // Arrange
    _extrusion->Solid.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteReverse)
{
    // Arrange
    _extrusion->Reversed.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext1, len, wid, 0)));
}

TEST_F(FeatureExtrusionTest, testExecuteSymmetric)
{
    // Arrange
    _extrusion->Symmetric.setValue(true);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(
        PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, -ext1 / 2, len, wid, ext1 / 2)));
}

TEST_F(FeatureExtrusionTest, testExecuteAngled)
{
    // Arrange
    const double ang = 30;
    const double tangent = tan(Base::toRadians(ang));

    // The shape is a truncated pyramid elongated by a truncated triangular prism in the middle.
    // Calc the volume of full size pyramid and prism, and subtract top volumes to truncate.
    const double shorterSide = len > wid ? wid : len;
    const double longerSide = len < wid ? wid : len;
    const double centerWidth = longerSide - shorterSide;  // Width of the triang prism.
    const double topHeight = shorterSide / tangent / 2;   // Height of the truncation
    const double fullHeight = ext1 + topHeight;
    const double fullPrismVol =
        fullHeight * (shorterSide + ext1 * tangent * 2.0) / 2.0 * centerWidth;
    const double fullPyrVol = pow(shorterSide + ext1 * tangent * 2.0, 2.0) / 3.0 * fullHeight;
    const double topPrismVol = topHeight * shorterSide / 2.0 * centerWidth;
    const double topPyrVol = pow(shorterSide, 2.0) / 3.0 * topHeight;
    const double targetVol = (fullPyrVol + fullPrismVol) - (topPyrVol + topPrismVol);
    _extrusion->Solid.setValue(true);
    _extrusion->TaperAngle.setValue(ang);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, targetVol);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb,
                                            Base::BoundBox3d(-ext1 * tangent,
                                                             -ext1 * tangent,
                                                             0,
                                                             len + ext1 * tangent,
                                                             wid + ext1 * tangent,
                                                             ext1)));
}

TEST_F(FeatureExtrusionTest, testExecuteAngledRev)
{
    // Arrange
    const double ang = 30;
    const double tangent = tan(Base::toRadians(ang));
    // The shape is a truncated pyramid elongated by a truncated triangular prism in the middle,
    // plus a rectangular prism.
    // Calc the volume of full size pyramid and prism, and subtract top volumes to truncate.
    const double shorterSide = len > wid ? wid : len;
    const double longerSide = len < wid ? wid : len;
    const double centerWidth = longerSide - shorterSide;  // Width of the triang prism.
    const double topHeight = shorterSide / tangent / 2;   // Height of the truncation
    const double fullHeight = ext1 / 2 + topHeight;
    const double fullPrismVol =
        fullHeight * (shorterSide + ext1 / 2 * tangent * 2.0) / 2.0 * centerWidth;
    const double fullPyrVol = pow(shorterSide + ext1 / 2 * tangent * 2.0, 2.0) / 3.0 * fullHeight;
    const double topPrismVol = topHeight * shorterSide / 2.0 * centerWidth;
    const double topPyrVol = pow(shorterSide, 2.0) / 3.0 * topHeight;
    const double targetVol =
        (fullPyrVol + fullPrismVol) - (topPyrVol + topPrismVol) + len * wid * ext1 / 2;

    _extrusion->Solid.setValue(true);
    _extrusion->Symmetric.setValue(true);
    _extrusion->TaperAngleRev.setValue(ang);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, targetVol);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb,
                                            Base::BoundBox3d(-ext1 * tangent / 2,
                                                             -ext1 * tangent / 2,
                                                             -ext1 / 2,
                                                             len + ext1 * tangent / 2,
                                                             wid + ext1 * tangent / 2,
                                                             ext1 / 2)));
}

TEST_F(FeatureExtrusionTest, testExecuteEdge)
{
    // Arrange
    const double ang = 30;
    const double tangent = tan(Base::toRadians(ang));
    BRepBuilderAPI_MakeEdge e1(gp_Pnt(0, 0, 0), gp_Pnt(ext1, ext1, ext1));
    auto edge = _doc->addObject<Part::Feature>("Edge");
    edge->Shape.setValue(e1);
    _extrusion->DirLink.setValue(edge);
    _extrusion->DirMode.setValue(1);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1 * tangent);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(0, 0, 0, len + ext1 * tangent, wid + ext1 * tangent, ext1 * tangent)));
}

TEST_F(FeatureExtrusionTest, testExecuteDir)
{
    // Arrange
    const double sin45 = sin(Base::toRadians(45.0));
    _extrusion->Dir.setValue(Base::Vector3d(0, 1, 1));
    _extrusion->DirMode.setValue((long)0);
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1 * sin45);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(0, 0, 0, len, wid + ext1 * sin45, ext1 * sin45)));
}

TEST_F(FeatureExtrusionTest, testExecuteFaceMaker)
{
    // Arrange
    _extrusion->FaceMakerClass.setValue("Part::FaceMakerCheese");
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
}

TEST_F(FeatureExtrusionTest, testFaceWithHoles)
{
    // Arrange
    float radius = 0.75;
    auto [face1, wire1, wire2] = PartTestHelpers::CreateFaceWithRoundHole(len, wid, radius);
    // face1 is the sum of the outside (wire1) and the internal hole (wire2).
    Part::TopoShape newFace = Part::TopoShape(face1).makeElementFace(nullptr);
    // newFace cleans that up and is the outside minus the internal hole.
    auto face2 = newFace.getShape();

    auto partFeature = _doc->addObject<Part::Feature>();
    partFeature->Shape.setValue(face2);
    _extrusion->Base.setValue(_doc->getObjects().back());
    _extrusion->FaceMakerClass.setValue("Part::FaceMakerCheese");
    // Act
    _extrusion->execute();
    Part::TopoShape ts = _extrusion->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_FLOAT_EQ(volume, len * wid * ext1 - radius * radius * std::numbers::pi * ext1);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, len, wid, ext1)));
    EXPECT_FLOAT_EQ(PartTestHelpers::getArea(face1),
                    len * wid + radius * radius * std::numbers::pi);
    EXPECT_FLOAT_EQ(PartTestHelpers::getArea(face2),
                    len * wid - radius * radius * std::numbers::pi);
}
