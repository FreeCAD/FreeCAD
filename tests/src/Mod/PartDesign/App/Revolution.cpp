// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>

#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/BoundBox.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/Sketcher/App/SketchObject.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

namespace
{

// Every test below revolves the same profile: a circle of radius 10mm whose center is
// 30mm away from the revolution axis. A full revolution of it is a torus with volume
// 2 * pi^2 * majorRadius * minorRadius^2.
constexpr double profileRadius = 10.0;
constexpr double axisDistance = 30.0;

// The base solid used by the Groove test: a cylinder large enough to fully contain the
// torus (the torus reaches 40mm from the Z axis and 40mm along it).
constexpr double baseRadius = 45.0;
constexpr double baseHeight = 100.0;

// Expected volumes in these tests are all around 6e5mm^3, so 1mm^3 is close enough for our
// purposes
constexpr double volumeTolerance = 1.0;

double torusVolume(double sweepDegrees)
{
    const double fullTorus = 2.0 * std::numbers::pi * std::numbers::pi * axisDistance
        * profileRadius * profileRadius;
    return fullTorus * sweepDegrees / 360.0;
}

double cylinderVolume()
{
    return std::numbers::pi * baseRadius * baseRadius * baseHeight;
}

double volumeOf(const TopoDS_Shape& shape)
{
    GProp_GProps properties;
    BRepGProp::VolumeProperties(shape, properties);
    return std::abs(properties.Mass());
}

}  // namespace

class RevolutionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc = App::GetApplication().newDocument("Revolution_test", "testUser");
        _body = _doc->addObject<PartDesign::Body>();
        _profile = addCircleSketch(axisDistance, profileRadius);
        _doc->recompute();
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc->getName());
    }

    App::Document* getDocument() const
    {
        return _doc;
    }

    Sketcher::SketchObject* getProfile() const
    {
        return _profile;
    }

    /// A circle of the given radius on the XY plane, offset along X from the origin.
    Sketcher::SketchObject* addCircleSketch(double centerX, double radius)
    {
        auto sketch = _doc->addObject<Sketcher::SketchObject>("Sketch");
        _body->addObject(sketch);
        sketch->AttachmentSupport.setValue(_doc->getObject("XY_Plane"), "");
        sketch->MapMode.setValue("FlatFace");

        Part::GeomCircle circle;
        circle.setCenter(Base::Vector3d(centerX, 0.0, 0.0));
        circle.setRadius(radius);
        sketch->addGeometry(&circle, false);
        return sketch;
    }

    /// A revolution of the profile sketch about the Y axis, which lies in the sketch
    /// plane and outside the profile: the revolution always sweeps a torus segment.
    PartDesign::Revolution* addRevolution()
    {
        auto revolution = _doc->addObject<PartDesign::Revolution>("Revolution");
        _body->addObject(revolution);
        revolution->Profile.setValue(_profile, {""});
        revolution->ReferenceAxis.setValue(_doc->getObject("Y_Axis"), {""});
        return revolution;
    }

    /// A cylinder centred on the sketch plane, used as material for the Groove to cut.
    PartDesign::Pad* addBaseCylinder()
    {
        auto sketch = addCircleSketch(0.0, baseRadius);
        auto pad = _doc->addObject<PartDesign::Pad>("Pad");
        _body->addObject(pad);
        pad->Profile.setValue(sketch, {""});
        pad->SideType.setValue("Symmetric");
        pad->Length.setValue(baseHeight);
        return pad;
    }

    PartDesign::Groove* addGroove()
    {
        auto groove = _doc->addObject<PartDesign::Groove>("Groove");
        _body->addObject(groove);
        groove->Profile.setValue(_profile, {""});
        groove->ReferenceAxis.setValue(_doc->getObject("Y_Axis"), {""});
        return groove;
    }

private:
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
    Sketcher::SketchObject* _profile = nullptr;
};

// Two sides that do not overlap: 90 degrees each way is one 180 degree sweep.
TEST_F(RevolutionTest, TwoSidesAngles)
{
    // Arrange
    auto revolution = addRevolution();
    revolution->Angle.setValue(90.0);

    // For test purposes, a control -- the same profile as a plain one sided revolution
    getDocument()->recompute();
    ASSERT_FALSE(revolution->isError()) << revolution->getStatusString();
    EXPECT_NEAR(volumeOf(revolution->Shape.getValue()), torusVolume(90.0), volumeTolerance);

    // Now arrange for real:
    revolution->SideType.setValue("Two sides");
    revolution->Angle2.setValue(90.0);

    // Act: set up the real two-sided test
    getDocument()->recompute();

    // Assert
    ASSERT_FALSE(revolution->isError()) << revolution->getStatusString();
    EXPECT_NEAR(volumeOf(revolution->Shape.getValue()), torusVolume(180.0), volumeTolerance);
}

// A negative second angle turns back into side 1. XOR removes the overlap, so the
// effective sweep is the signed total: 90 + (-10) = 80 degrees.
TEST_F(RevolutionTest, TwoSidesSignedAngles)
{
    // Arrange
    auto revolution = addRevolution();
    revolution->SideType.setValue("Two sides");
    revolution->Angle.setValue(90.0);
    revolution->Angle2.setValue(-10.0);

    // Act
    getDocument()->recompute();

    // Assert
    ASSERT_FALSE(revolution->isError()) << revolution->getStatusString();
    EXPECT_NEAR(volumeOf(revolution->Shape.getValue()), torusVolume(80.0), volumeTolerance);
}

// Two sides that *do* overlap. Side 1 sweeps [0, 200] and side 2 sweeps [-200, 0], so
// between them they cover the whole circle with a 40 degree overlap. The result should
// simply be a complete torus.
TEST_F(RevolutionTest, TwoSidesOverlappingAngles)
{
    // Arrange
    auto revolution = addRevolution();
    revolution->SideType.setValue("Two sides");
    revolution->Angle.setValue(200.0);
    revolution->Angle2.setValue(200.0);

    // Act
    getDocument()->recompute();

    // Assert
    ASSERT_FALSE(revolution->isError()) << revolution->getStatusString();
    EXPECT_NEAR(volumeOf(revolution->Shape.getValue()), torusVolume(360.0), volumeTolerance);
}

// Symmetric sweeps half the angle each side of the sketch plane. The sketch is on the
// XY plane and the axis is Y, so the result must be mirror symmetric in Z.
TEST_F(RevolutionTest, SymmetricAngle)
{
    // Arrange
    auto revolution = addRevolution();
    revolution->SideType.setValue("Symmetric");
    revolution->Angle.setValue(180.0);

    // Act
    getDocument()->recompute();

    // Assert
    ASSERT_FALSE(revolution->isError()) << revolution->getStatusString();
    EXPECT_NEAR(volumeOf(revolution->Shape.getValue()), torusVolume(180.0), volumeTolerance);

    const Base::BoundBox3d bbox = revolution->Shape.getBoundingBox();
    EXPECT_NEAR(bbox.MinZ, -bbox.MaxZ, Precision::Confusion());
}

// Midplane is deprecated in favor of SideType. Scripts that still set it must keep
// working, which the compatibility shim in Revolved::onChanged() is there to provide.
TEST_F(RevolutionTest, MidplaneMapsToSideType)
{
    auto revolution = addRevolution();
    revolution->Angle.setValue(180.0);

    revolution->Midplane.setValue(true);
    EXPECT_STREQ(revolution->SideType.getValueAsString(), "Symmetric");

    revolution->Midplane.setValue(false);
    EXPECT_STREQ(revolution->SideType.getValueAsString(), "One side");
}

// A groove set to "Through all" on both sides. Each side is a full 360 degree cut, so
// the two sides are the same volume and the union of them is a single whole torus.
TEST_F(RevolutionTest, GrooveTwoSidesThroughAll)
{
    addBaseCylinder();
    auto groove = addGroove();
    groove->Type.setValue("ThroughAll");

    // Control: one sided "Through all" already cuts the whole torus out.
    getDocument()->recompute();
    ASSERT_FALSE(groove->isError()) << groove->getStatusString();
    EXPECT_NEAR(
        volumeOf(groove->Shape.getValue()),
        cylinderVolume() - torusVolume(360.0),
        volumeTolerance
    );

    groove->SideType.setValue("Two sides");
    groove->Type2.setValue("ThroughAll");

    getDocument()->recompute();

    ASSERT_FALSE(groove->isError()) << groove->getStatusString();
    EXPECT_NEAR(
        volumeOf(groove->Shape.getValue()),
        cylinderVolume() - torusVolume(360.0),
        volumeTolerance
    );
}

// The angular start offset moves the groove profile before the cut is generated.
TEST_F(RevolutionTest, GrooveStartOffset)
{
    // Arrange
    addBaseCylinder();
    auto groove = addGroove();
    groove->Angle.setValue(30.0);
    groove->StartType.setValue("Offset");
    groove->StartOffset.setValue(90.0);

    // Act
    getDocument()->recompute();

    // Assert
    ASSERT_FALSE(groove->isError()) << groove->getStatusString();
    EXPECT_NEAR(volumeOf(groove->Shape.getValue()), cylinderVolume() - torusVolume(30.0), volumeTolerance);

    const Base::BoundBox3d bbox = groove->AddSubShape.getShape().getBoundBox();
    EXPECT_LT(bbox.MaxZ, -15.0);
}

// Side 2 set to "Up to face" with no face picked yet is the state the task panel starts
// in. This should show up as a recompute error (rather than an uncaught exception).
TEST_F(RevolutionTest, SecondSideUpToFaceWithoutTargetIsAnError)
{
    auto revolution = addRevolution();
    revolution->Angle.setValue(90.0);
    revolution->SideType.setValue("Two sides");
    revolution->Type2.setValue("UpToFace");

    getDocument()->recompute();

    EXPECT_TRUE(revolution->isError());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
