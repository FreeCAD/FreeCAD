// SPDX-License-Identifier: LGPL-2.1-or-later

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "SketcherTestHelpers.h"

using namespace SketcherTestHelpers;


TEST_F(SketchObjectTest, createSketchObject)  // NOLINT
{
    // Arrange

    // Act

    // Assert
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeEdge)
{
    // Arrange
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("Edge", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, 0);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeVertex)
{
    // Arrange
    // For operating on vertices, there is newName a check if the vertex exists.
    Base::Vector3d p1(0.0, 0.0, 0.0), p2(1.0, 0.0, 0.0);
    std::unique_ptr<Part::Geometry> geoline(new Part::GeomLineSegment());
    static_cast<Part::GeomLineSegment*>(geoline.get())->setPoints(p1, p2);
    getObject()->addGeometry(geoline.get());
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("Vertex", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, 0);
    EXPECT_EQ(posId, Sketcher::PointPos::start);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeExternalEdge)
{
    // Arrange
    // TODO: Do we need to separate existing vs non-existing?
    // It would need to be implemented in code as well.
    Data::IndexedName name("ExternalEdge", 1);
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::RefExt);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeHAxis)
{
    // Arrange
    Data::IndexedName name("H_Axis");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::HAxis);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeVAxis)
{
    // Arrange
    Data::IndexedName name("V_Axis");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::VAxis);
    EXPECT_EQ(posId, Sketcher::PointPos::none);
}

TEST_F(SketchObjectTest, testGeoIdFromShapeTypeRootPoint)
{
    // Arrange
    Data::IndexedName name("RootPoint");
    int geoId;
    Sketcher::PointPos posId;

    // Act
    getObject()->geoIdFromShapeType(name, geoId, posId);

    // Assert
    EXPECT_EQ(geoId, Sketcher::GeoEnum::RtPnt);
    EXPECT_EQ(posId, Sketcher::PointPos::start);
}

TEST_F(SketchObjectTest, testGetPointFromGeomPoint)
{
    // Arrange
    Base::Vector3d coords(1.0, 2.0, 0.0);
    Part::GeomPoint point(coords);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&point, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&point, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&point, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&point, Sketcher::PointPos::none);

    // Assert
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
    EXPECT_DOUBLE_EQ(ptEnd[0], 1.0);
    EXPECT_DOUBLE_EQ(ptEnd[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomLineSegment)
{
    // Arrange
    Base::Vector3d coords1(1.0, 2.0, 0.0);
    Base::Vector3d coords2(3.0, 4.0, 0.0);
    Part::GeomLineSegment lineSeg;
    lineSeg.setPoints(coords1, coords2);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::start);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptMid =
        Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::none);

    // Assert
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    EXPECT_DOUBLE_EQ(ptEnd[0], 3.0);
    EXPECT_DOUBLE_EQ(ptEnd[1], 4.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomCircle)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double radius = 3.0;
    Part::GeomCircle circle;
    circle.setCenter(coordsCenter);
    circle.setRadius(radius);

    // Act
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptStart = Sketcher::SketchObject::getPoint(&circle, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&circle, Sketcher::PointPos::mid);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptEnd = Sketcher::SketchObject::getPoint(&circle, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&circle, Sketcher::PointPos::none);

    // Assert
    // NOTE: Presently, start/end points of a circle are defined as the point on circle right of the
    // the center
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0 + radius);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    EXPECT_DOUBLE_EQ(ptEnd[0], 1.0 + radius);
    EXPECT_DOUBLE_EQ(ptEnd[1], 2.0);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomEllipse)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    Part::GeomEllipse ellipse;
    ellipse.setCenter(coordsCenter);
    ellipse.setMajorRadius(majorRadius);
    ellipse.setMinorRadius(minorRadius);

    // Act
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptStart = Sketcher::SketchObject::getPoint(&ellipse, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&ellipse, Sketcher::PointPos::mid);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptEnd = Sketcher::SketchObject::getPoint(&ellipse, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&ellipse, Sketcher::PointPos::none);

    // Assert
    // NOTE: Presently, start/end points of an ellipse are defined as the point on the major axis in
    // it's "positive" direction
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0 + majorRadius);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    EXPECT_DOUBLE_EQ(ptEnd[0], 1.0 + majorRadius);
    EXPECT_DOUBLE_EQ(ptEnd[1], 2.0);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomArcOfCircle)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double radius = 3.0, startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
    Part::GeomArcOfCircle arcOfCircle;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::none);

    // Assert
    // NOTE: parameters for arc of circle are CCW angles from positive x-axis
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0 + cos(startParam) * radius);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0 + sin(startParam) * radius);
    EXPECT_DOUBLE_EQ(ptEnd[0], 1.0 + cos(endParam) * radius);
    EXPECT_DOUBLE_EQ(ptEnd[1], 2.0 + sin(endParam) * radius);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomArcOfEllipse)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    double startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
    Part::GeomArcOfEllipse arcOfEllipse;
    arcOfEllipse.setCenter(coordsCenter);
    arcOfEllipse.setMajorRadius(majorRadius);
    arcOfEllipse.setMinorRadius(minorRadius);
    arcOfEllipse.setRange(startParam, endParam, true);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&arcOfEllipse, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfEllipse, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&arcOfEllipse, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&arcOfEllipse, Sketcher::PointPos::none);

    // Assert
    // NOTE: parameters for arc of ellipse are CCW angles from positive x-axis
    EXPECT_DOUBLE_EQ(ptStart[0], 1.0 + cos(startParam) * majorRadius);
    EXPECT_DOUBLE_EQ(ptStart[1], 2.0 + sin(startParam) * minorRadius);
    EXPECT_DOUBLE_EQ(ptEnd[0], 1.0 + cos(endParam) * majorRadius);
    EXPECT_DOUBLE_EQ(ptEnd[1], 2.0 + sin(endParam) * minorRadius);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomArcOfHyperbola)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    double startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
    Part::GeomArcOfHyperbola arcOfHyperbola;
    arcOfHyperbola.setCenter(coordsCenter);
    arcOfHyperbola.setMajorRadius(majorRadius);
    arcOfHyperbola.setMinorRadius(minorRadius);
    arcOfHyperbola.setRange(startParam, endParam, true);

    // Act
    [[maybe_unused]] auto ptStart =
        Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::mid);
    [[maybe_unused]] auto ptEnd =
        Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::none);

    // Assert
    // FIXME: Figure out how this is defined
    // EXPECT_DOUBLE_EQ(ptStart[0], 1.0);
    // EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    // EXPECT_DOUBLE_EQ(ptEnd[0], 1.0);
    // EXPECT_DOUBLE_EQ(ptEnd[1], 2.0);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomArcOfParabola)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double focal = 3.0;
    double startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
    Part::GeomArcOfParabola arcOfParabola;
    arcOfParabola.setCenter(coordsCenter);
    arcOfParabola.setFocal(focal);
    arcOfParabola.setRange(startParam, endParam, true);

    // Act
    [[maybe_unused]] auto ptStart =
        Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::mid);
    [[maybe_unused]] auto ptEnd =
        Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::none);

    // Assert
    // FIXME: Figure out how this is defined
    // EXPECT_DOUBLE_EQ(ptStart[0], 1.0);
    // EXPECT_DOUBLE_EQ(ptStart[1], 2.0);
    // EXPECT_DOUBLE_EQ(ptEnd[0], 1.0);
    // EXPECT_DOUBLE_EQ(ptEnd[1], 2.0);
    EXPECT_DOUBLE_EQ(ptMid[0], 1.0);
    EXPECT_DOUBLE_EQ(ptMid[1], 2.0);
}

TEST_F(SketchObjectTest, testGetPointFromGeomBSplineCurveNonPeriodic)
{
    // Arrange
    int degree = 3;
    std::vector<Base::Vector3d> poles;
    poles.emplace_back(1, 0, 0);
    poles.emplace_back(1, 1, 0);
    poles.emplace_back(1, 0.5, 0);
    poles.emplace_back(0, 1, 0);
    poles.emplace_back(0, 0, 0);
    std::vector<double> weights(5, 1.0);
    std::vector<double> knotsNonPeriodic = {0.0, 1.0, 2.0};
    std::vector<int> multiplicitiesNonPeriodic = {degree + 1, 1, degree + 1};
    Part::GeomBSplineCurve nonPeriodicBSpline(poles,
                                              weights,
                                              knotsNonPeriodic,
                                              multiplicitiesNonPeriodic,
                                              degree,
                                              false);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::start);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptMid =
        Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::none);

    // Assert
    EXPECT_DOUBLE_EQ(ptStart[0], poles.front()[0]);
    EXPECT_DOUBLE_EQ(ptStart[1], poles.front()[1]);
    EXPECT_DOUBLE_EQ(ptEnd[0], poles.back()[0]);
    EXPECT_DOUBLE_EQ(ptEnd[1], poles.back()[1]);
}

TEST_F(SketchObjectTest, testGetPointFromGeomBSplineCurvePeriodic)
{
    // Arrange
    int degree = 3;
    std::vector<Base::Vector3d> poles;
    poles.emplace_back(1, 0, 0);
    poles.emplace_back(1, 1, 0);
    poles.emplace_back(1, 0.5, 0);
    poles.emplace_back(0, 1, 0);
    poles.emplace_back(0, 0, 0);
    std::vector<double> weights(5, 1.0);
    std::vector<double> knotsPeriodic = {0.0, 0.3, 1.0, 1.5, 1.8, 2.0};
    std::vector<int> multiplicitiesPeriodic(6, 1);
    Part::GeomBSplineCurve periodicBSpline(poles,
                                           weights,
                                           knotsPeriodic,
                                           multiplicitiesPeriodic,
                                           degree,
                                           true);

    // Act
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptStart = Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::start);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptMid =
        Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::mid);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptEnd = Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    [[maybe_unused]] auto ptNone =
        Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::none);

    // Assert
    // With non-trivial values for weights, knots, mults, etc, getting the coordinates is
    // non-trivial as well. This is the best we can do.
    EXPECT_DOUBLE_EQ(ptStart[0], ptEnd[0]);
    EXPECT_DOUBLE_EQ(ptStart[1], ptEnd[1]);
}

TEST_F(SketchObjectTest, testConstraintAfterDeletingGeo)
{
    // Arrange
    int geoId1 = 42, geoId2 = 10, geoId3 = 0, geoId4 = -8;

    Sketcher::Constraint* nullConstr = nullptr;

    Sketcher::Constraint constr1;
    constr1.Type = Sketcher::ConstraintType::Coincident;
    constr1.First = geoId1;
    constr1.FirstPos = Sketcher::PointPos::start;
    constr1.Second = geoId2;
    constr1.SecondPos = Sketcher::PointPos::end;

    Sketcher::Constraint constr2;
    constr2.Type = Sketcher::ConstraintType::Tangent;
    constr2.First = geoId4;
    constr2.FirstPos = Sketcher::PointPos::none;
    constr2.Second = geoId3;
    constr2.SecondPos = Sketcher::PointPos::none;
    constr2.Third = geoId1;
    constr2.ThirdPos = Sketcher::PointPos::start;

    // Act
    auto nullConstrAfter = getObject()->getConstraintAfterDeletingGeo(nullConstr, 5);

    // Assert
    EXPECT_EQ(nullConstrAfter, nullptr);

    // Act
    getObject()->changeConstraintAfterDeletingGeo(nullConstr, 5);

    // Assert
    EXPECT_EQ(nullConstr, nullptr);

    // Act
    // delete typical in-sketch geo
    auto constr1PtrAfter1 = getObject()->getConstraintAfterDeletingGeo(&constr1, 5);
    // delete external geo (negative id)
    auto constr1PtrAfter2 = getObject()->getConstraintAfterDeletingGeo(&constr1, -5);
    // Delete a geo involved in the constraint
    auto constr1PtrAfter3 = getObject()->getConstraintAfterDeletingGeo(&constr1, 10);

    // Assert
    EXPECT_EQ(constr1.Type, Sketcher::ConstraintType::Coincident);
    EXPECT_EQ(constr1.First, geoId1);
    EXPECT_EQ(constr1.Second, geoId2);
    EXPECT_EQ(constr1PtrAfter1->First, geoId1 - 1);
    EXPECT_EQ(constr1PtrAfter1->Second, geoId2 - 1);
    EXPECT_EQ(constr1PtrAfter2->Third, Sketcher::GeoEnum::GeoUndef);
    EXPECT_EQ(constr1PtrAfter3.get(), nullptr);

    // Act
    getObject()->changeConstraintAfterDeletingGeo(&constr2, -3);

    // Assert
    EXPECT_EQ(constr2.Type, Sketcher::ConstraintType::Tangent);
    EXPECT_EQ(constr2.First, geoId4 + 1);
    EXPECT_EQ(constr2.Second, geoId3);
    EXPECT_EQ(constr2.Third, geoId1);

    // Act
    // Delete a geo involved in the constraint
    getObject()->changeConstraintAfterDeletingGeo(&constr2, 0);

    // Assert
    EXPECT_EQ(constr2.Type, Sketcher::ConstraintType::None);
}

TEST_F(SketchObjectTest, testDeleteExposeInternalGeometryOfEllipse)
{
    // Arrange
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);

    // Act
    // "Expose" internal geometry
    getObject()->exposeInternalGeometry(geoId);

    // Assert
    // Ensure all internal geometry is satisfied
    // TODO: Also try to ensure types of geometries that have this type
    const auto constraints = getObject()->Constraints.getValues();
    for (auto alignmentType : {Sketcher::InternalAlignmentType::EllipseMajorDiameter,
                               Sketcher::InternalAlignmentType::EllipseMinorDiameter,
                               Sketcher::InternalAlignmentType::EllipseFocus1,
                               Sketcher::InternalAlignmentType::EllipseFocus2}) {
        // TODO: Ensure there exists one and only one curve with this type
        int numConstraintsOfThisType =
            std::count_if(constraints.begin(),
                          constraints.end(),
                          [&geoId, &alignmentType](const auto* constr) {
                              return constr->Type == Sketcher::ConstraintType::InternalAlignment
                                  && constr->AlignmentType == alignmentType
                                  && constr->Second == geoId;
                          });
        EXPECT_EQ(numConstraintsOfThisType, 1);
    }

    // Act
    // Delete internal geometry (again)
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

TEST_F(SketchObjectTest, testDeleteExposeInternalGeometryOfHyperbola)
{
    // Arrange
    Part::GeomArcOfHyperbola aoh;
    setupArcOfHyperbola(aoh);
    int geoId = getObject()->addGeometry(&aoh);

    // Act
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);

    // Act
    // "Expose" internal geometry
    getObject()->exposeInternalGeometry(geoId);

    // Assert
    // Ensure all internal geometry is satisfied
    // TODO: Also try to ensure types of geometries that have this type
    const auto constraints = getObject()->Constraints.getValues();
    for (auto alignmentType : {Sketcher::InternalAlignmentType::HyperbolaMajor,
                               Sketcher::InternalAlignmentType::HyperbolaMinor,
                               Sketcher::InternalAlignmentType::HyperbolaFocus}) {
        // TODO: Ensure there exists one and only one curve with this type
        int numConstraintsOfThisType =
            std::count_if(constraints.begin(),
                          constraints.end(),
                          [&geoId, &alignmentType](const auto* constr) {
                              return constr->Type == Sketcher::ConstraintType::InternalAlignment
                                  && constr->AlignmentType == alignmentType
                                  && constr->Second == geoId;
                          });
        EXPECT_EQ(numConstraintsOfThisType, 1);
    }

    // Act
    // Delete internal geometry (again)
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

TEST_F(SketchObjectTest, testDeleteExposeInternalGeometryOfParabola)
{
    // Arrange
    Part::GeomArcOfParabola aoh;
    setupArcOfParabola(aoh);
    int geoId = getObject()->addGeometry(&aoh);

    // Act
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);

    // Act
    // "Expose" internal geometry
    getObject()->exposeInternalGeometry(geoId);

    // Assert
    // Ensure all internal geometry is satisfied
    // TODO: Also try to ensure types of geometries that have this type
    const auto constraints = getObject()->Constraints.getValues();
    for (auto alignmentType : {Sketcher::InternalAlignmentType::ParabolaFocalAxis,
                               Sketcher::InternalAlignmentType::ParabolaFocus}) {
        // TODO: Ensure there exists one and only one curve with this type
        int numConstraintsOfThisType =
            std::count_if(constraints.begin(),
                          constraints.end(),
                          [&geoId, &alignmentType](const auto* constr) {
                              return constr->Type == Sketcher::ConstraintType::InternalAlignment
                                  && constr->AlignmentType == alignmentType
                                  && constr->Second == geoId;
                          });
        EXPECT_EQ(numConstraintsOfThisType, 1);
    }

    // Act
    // Delete internal geometry (again)
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

TEST_F(SketchObjectTest, testDeleteExposeInternalGeometryOfBSpline)
{
    // NOTE: We test only non-periodic B-spline here. Periodic B-spline should behave exactly the
    // same.

    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    int geoId = getObject()->addGeometry(nonPeriodicBSpline.get());

    // Act
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);

    // Act
    // "Expose" internal geometry
    getObject()->exposeInternalGeometry(geoId);

    // Assert
    // Ensure all internal geometry is satisfied
    // TODO: Also try to ensure types of geometries that have this type
    const auto constraints = getObject()->Constraints.getValues();
    std::map<Sketcher::InternalAlignmentType, int> numConstraintsOfThisType;
    for (auto alignmentType : {Sketcher::InternalAlignmentType::BSplineControlPoint,
                               Sketcher::InternalAlignmentType::BSplineKnotPoint}) {
        // TODO: Ensure there exists one and only one curve with this type
        numConstraintsOfThisType[alignmentType] =
            std::count_if(constraints.begin(),
                          constraints.end(),
                          [&geoId, &alignmentType](const auto* constr) {
                              return constr->Type == Sketcher::ConstraintType::InternalAlignment
                                  && constr->AlignmentType == alignmentType
                                  && constr->Second == geoId;
                          });
    }
    EXPECT_EQ(numConstraintsOfThisType[Sketcher::InternalAlignmentType::BSplineControlPoint],
              nonPeriodicBSpline->countPoles());
    EXPECT_EQ(numConstraintsOfThisType[Sketcher::InternalAlignmentType::BSplineKnotPoint],
              nonPeriodicBSpline->countKnots());

    // Act
    // Delete internal geometry (again)
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoId);

    // Assert
    // Ensure there's only one curve
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 0);
}

// TODO: Needs to be done for other curves too but currently they are working as intended
TEST_F(SketchObjectTest, testDeleteOnlyUnusedInternalGeometryOfBSpline)
{
    // NOTE: We test only non-periodic B-spline here. Periodic B-spline should behave exactly the
    // same.

    // Arrange
    auto nonPeriodicBSpline = createTypicalNonPeriodicBSpline();
    int geoIdBsp = getObject()->addGeometry(nonPeriodicBSpline.get());
    // Ensure "exposed" internal geometry
    getObject()->exposeInternalGeometry(geoIdBsp);
    Base::Vector3d coords(1.0, 1.0, 0.0);
    Part::GeomPoint point(coords);
    int geoIdPnt = getObject()->addGeometry(&point);
    const auto constraints = getObject()->Constraints.getValues();
    auto it = std::find_if(constraints.begin(), constraints.end(), [&geoIdBsp](const auto* constr) {
        return constr->Type == Sketcher::ConstraintType::InternalAlignment
            && constr->AlignmentType == Sketcher::InternalAlignmentType::BSplineControlPoint
            && constr->Second == geoIdBsp && constr->InternalAlignmentIndex == 1;
    });
    // One Assert to avoid
    EXPECT_NE(it, constraints.end());
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Coincident;
    constraint->First = geoIdPnt;
    constraint->FirstPos = Sketcher::PointPos::start;
    constraint->Second = (*it)->First;
    constraint->SecondPos = Sketcher::PointPos::mid;
    getObject()->addConstraint(constraint);

    // Act
    getObject()->deleteUnusedInternalGeometryAndUpdateGeoId(geoIdBsp);

    // Assert
    // Ensure there are 3 curves: the B-spline, its pole, and the point coincident on the pole
    EXPECT_EQ(getObject()->getHighestCurveIndex(), 2);
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionNoUnits1)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("180 - 60");
    EXPECT_EQ(expr, std::string("60"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionNoUnits2)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60");
    EXPECT_EQ(expr, std::string("180 - (60)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits1)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("180 ° - 60 °");
    EXPECT_EQ(expr, std::string("60 °"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits2)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60 °");
    EXPECT_EQ(expr, std::string("180 ° - (60 °)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits3)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("60 deg");
    EXPECT_EQ(expr, std::string("180 ° - (60 deg)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionWithUnits4)
{
    std::string expr = Sketcher::SketchObject::reverseAngleConstraintExpression("1rad");
    EXPECT_EQ(expr, std::string("180 ° - (1rad)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse1)
{
    std::string expr = "180";
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    EXPECT_EQ(expr, std::string("(180)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse2)
{
    std::string expr = "(30 + 15) * 2 / 3";
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    expr = Sketcher::SketchObject::reverseAngleConstraintExpression(expr);
    EXPECT_EQ(expr, std::string("((30 + 15) * 2 / 3)"));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionSimple)
{
    // Arrange
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Angle;
    auto id = getObject()->addConstraint(constraint);

    App::ObjectIdentifier path(App::ObjectIdentifier::parse(getObject(), "Constraints[0]"));
    std::shared_ptr<App::Expression> shared_expr(App::Expression::parse(getObject(), "0"));
    getObject()->setExpression(path, shared_expr);

    getObject()->setConstraintExpression(id, "180 - (60)");

    // Act
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);

    // Assert
    EXPECT_EQ(std::string("60"), getObject()->getConstraintExpression(id));
}

TEST_F(SketchObjectTest, testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse)
{
    // Arrange
    auto constraint = new Sketcher::Constraint();  // Ownership will be transferred to the sketch
    constraint->Type = Sketcher::ConstraintType::Angle;
    auto id = getObject()->addConstraint(constraint);

    App::ObjectIdentifier path(App::ObjectIdentifier::parse(getObject(), "Constraints[0]"));
    std::shared_ptr<App::Expression> shared_expr(App::Expression::parse(getObject(), "0"));
    getObject()->setExpression(path, shared_expr);

    getObject()->setConstraintExpression(id, "32 °");

    // Act
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);
    getObject()->reverseAngleConstraintToSupplementary(constraint, id);

    // Assert
    EXPECT_EQ(std::string("32 °"), getObject()->getConstraintExpression(id));
}

TEST_F(SketchObjectTest, testGetElementName)
{
    // Arrange
    Base::Vector3d p1(0.0, 0.0, 0.0), p2(1.0, 0.0, 0.0);
    std::unique_ptr<Part::Geometry> geoline(new Part::GeomLineSegment());
    static_cast<Part::GeomLineSegment*>(geoline.get())->setPoints(p1, p2);
    auto id = getObject()->addGeometry(geoline.get());
    long tag;
    getObject()->getGeometryId(id, tag);  // We need to look up the tag that got assigned
    std::ostringstream oss;
    oss << "g" << tag;
    auto tagName = oss.str();
    getObject()->recomputeFeature();  // or ->execute()
    // Act
    // unless it's Export, we are really just testing the superclass App::GeoFeature::getElementName
    // call.
    auto forward_normal_name =
        getObject()->getElementName((tagName + ";SKT").c_str(),
                                    App::GeoFeature::ElementNameType::Normal);
    auto reverse_normal_name =
        getObject()->getElementName("Vertex2", App::GeoFeature::ElementNameType::Normal);
    auto reverse_export_name =
        getObject()->getElementName("Vertex1", App::GeoFeature::ElementNameType::Export);
    auto map = getObject()->Shape.getShape().getElementMap();
    ASSERT_EQ(map.size(), 3);
    EXPECT_STREQ(map[0].name.toString().c_str(), (tagName + ";SKT").c_str());
    EXPECT_EQ(map[0].index.toString(), "Edge1");
    EXPECT_STREQ(map[1].name.toString().c_str(), (tagName + "v1;SKT").c_str());
    EXPECT_EQ(map[1].index.toString(), "Vertex1");
    EXPECT_STREQ(map[2].name.toString().c_str(), (tagName + "v2;SKT").c_str());
    EXPECT_EQ(map[2].index.toString(), "Vertex2");
    // Assert
    EXPECT_STREQ(forward_normal_name.newName.c_str(), (";" + tagName + ";SKT.Edge1").c_str());
    EXPECT_STREQ(forward_normal_name.oldName.c_str(), "Edge1");
    EXPECT_STREQ(reverse_normal_name.newName.c_str(), (";" + tagName + "v2;SKT.Vertex2").c_str());
    EXPECT_STREQ(reverse_normal_name.oldName.c_str(), "Vertex2");
    EXPECT_STREQ(reverse_export_name.newName.c_str(), (";" + tagName + "v1;SKT.Vertex1").c_str());
    EXPECT_STREQ(reverse_export_name.oldName.c_str(), "Vertex1");
}
