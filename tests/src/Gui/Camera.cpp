#include <gtest/gtest.h>

#include <array>
#include <cmath>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbViewVolume.h>
#include <Base/Tools.h>
#include <Gui/Camera.h>

/*
 This comment was previously used to get the hard coded axonometric view quaternions
 in the Camera class.
 This has since been replaced with unit tests that verify the correctness of the
 quaternion calculations.

 The old code is kept for reference and to show how the quaternions were calculated.

 ---

 Formulas to get quaternion for axonometric views:

 \code
from math import sqrt, degrees, asin, atan
p1=App.Rotation(App.Vector(1,0,0),90)
p2=App.Rotation(App.Vector(0,0,1),alpha)
p3=App.Rotation(p2.multVec(App.Vector(1,0,0)),beta)
p4=p3.multiply(p2).multiply(p1)

from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
c.orientation.setValue(*p4.Q)
 \endcode

 The angles alpha and beta depend on the type of axonometry
 Isometric:
 \code
alpha=45
beta=degrees(asin(-sqrt(1.0/3.0)))
 \endcode

 Dimetric:
 \code
alpha=degrees(asin(sqrt(1.0/8.0)))
beta=degrees(-asin(1.0/3.0))
 \endcode

 Trimetric:
 \code
alpha=30.0
beta=-35.0
 \endcode

 Verification code that the axonomtries are correct:

 \code
from pivy import coin
c=Gui.ActiveDocument.ActiveView.getCameraNode()
vo=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,0)).getValue())
vx=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(10,0,0)).getValue())
vy=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,10,0)).getValue())
vz=App.Vector(c.getViewVolume().getMatrix().multVecMatrix(coin.SbVec3f(0,0,10)).getValue())
(vx-vo).Length
(vy-vo).Length
(vz-vo).Length

# Projection
vo.z=0
vx.z=0
vy.z=0
vz.z=0

(vx-vo).Length
(vy-vo).Length
(vz-vo).Length
 \endcode

 See also:
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/graphics_6_2_ger_web.html#1
 http://www.mathematik.uni-marburg.de/~thormae/lectures/graphics1/code_v2/Axonometric/qt/Axonometric.cpp
 https://de.wikipedia.org/wiki/Arkussinus_und_Arkuskosinus
*/

using namespace Base;

namespace
{

Rotation buildAxonometricRotation(double alphaRad, double betaRad)
{
    const auto p1 = Rotation(Vector3d(1, 0, 0), Base::toRadians<float>(90.0));
    const auto p2 = Rotation(Vector3d(0, 0, 1), alphaRad);
    const auto p3 = Rotation(p2.multVec(Vector3d(1, 0, 0)), betaRad);
    const auto p4 = p3 * p2 * p1;

    return p4;
}

Base::Rotation toRotation(const SbRotation& sb)
{
    return Base::Rotation(sb[0], sb[1], sb[2], sb[3]);
}

double project2DLength(const Vector3d& v)
{
    Vector3d flat = v;
    flat.z = 0;  // Projection to XY plane
    return flat.Length();
}

// Returns a tuple of 2D lengths of X, Y, Z unit vectors after applying rotation
std::array<double, 3> getProjectedLengths(const SbRotation& rot)
{
    // Set up a simple view volume to test the projection of the unit vectors.
    // The actual values don't matter much, as we are only interested in the
    // relative lengths of the projected vectors.
    SbViewVolume volume;
    // left, right, bottom, top, near, far
    volume.ortho(-10, 10, -10, 10, -10, 10);

    volume.rotateCamera(rot);
    const auto matrix = volume.getMatrix();

    // Get the transformed unit vectors
    SbVec3f vo, vx, vy, vz;
    matrix.multVecMatrix(SbVec3f(0, 0, 0), vo);
    matrix.multVecMatrix(SbVec3f(10, 0, 0), vx);
    matrix.multVecMatrix(SbVec3f(0, 10, 0), vy);
    matrix.multVecMatrix(SbVec3f(0, 0, 10), vz);

    // Project to XY plane by setting Z to 0
    vo[2] = 0;
    vx[2] = 0;
    vy[2] = 0;
    vz[2] = 0;

    // Return the lengths of the projected vectors
    return {(vx - vo).length(), (vy - vo).length(), (vz - vo).length()};
}

}  // namespace

TEST(CameraPrecalculatedQuaternions, testIsometric)
{
    // Use the formula to get the isometric rotation
    double alpha = toRadians(45.0f);
    double beta = std::asin(-std::sqrt(1.0 / 3.0));

    const Rotation actual = buildAxonometricRotation(alpha, beta);
    const Rotation expected = toRotation(Gui::Camera::isometric());

    EXPECT_TRUE(actual.isSame(expected, 1e-6));
}

TEST(CameraPrecalculatedQuaternions, testDimetric)
{
    // Use the formula to get the dimetric rotation
    double alpha = std::asin(std::sqrt(1.0 / 8.0));
    double beta = -std::asin(1.0 / 3.0);

    const Rotation actual = buildAxonometricRotation(alpha, beta);
    const Rotation expected = toRotation(Gui::Camera::dimetric());

    EXPECT_TRUE(actual.isSame(expected, 1e-6));
}

TEST(CameraPrecalculatedQuaternions, testTrimetric)
{
    // Use the formula to get the trimetric rotation
    double alpha = toRadians(30.0);
    double beta = toRadians(-35.0);

    const Rotation actual = buildAxonometricRotation(alpha, beta);
    const Rotation expected = toRotation(Gui::Camera::trimetric());

    EXPECT_TRUE(actual.isSame(expected, 1e-6));
}

TEST(CameraRotation, testIsometricProjection)
{
    auto rot = Gui::Camera::isometric();
    auto lengths = getProjectedLengths(rot);

    // In isometric, expect all lengths to be roughly equal
    EXPECT_NEAR(lengths[0], lengths[1], 1e-6);  // X == Y
    EXPECT_NEAR(lengths[0], lengths[2], 1e-6);  // X == Z
    EXPECT_NEAR(lengths[1], lengths[2], 1e-6);  // Y == Z
}

TEST(CameraRotation, testDimetricProjection)
{
    auto rot = Gui::Camera::dimetric();
    auto lengths = getProjectedLengths(rot);

    // In dimetric, expect two lengths to be roughly equal, one different
    int similarCount = 0;
    if (std::abs(lengths[0] - lengths[1]) < 1e-6) {
        similarCount++;
    }
    if (std::abs(lengths[1] - lengths[2]) < 1e-6) {
        similarCount++;
    }
    if (std::abs(lengths[0] - lengths[2]) < 1e-6) {
        similarCount++;
    }

    EXPECT_EQ(similarCount, 1);  // Exactly two are equal
}

TEST(CameraRotation, testTrimetricProjection)
{
    auto rot = Gui::Camera::trimetric();
    auto lengths = getProjectedLengths(rot);

    // In trimetric, all should differ significantly
    EXPECT_GT(std::abs(lengths[0] - lengths[1]), 1e-3);
    EXPECT_GT(std::abs(lengths[1] - lengths[2]), 1e-3);
    EXPECT_GT(std::abs(lengths[0] - lengths[2]), 1e-3);
}
