// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <src/App/InitApplication.h>

void setupLineSegment(Part::GeomLineSegment& lineSeg)
{
    Base::Vector3d coords1(1.0, 2.0, 0.0);
    Base::Vector3d coords2(3.0, 4.0, 0.0);
    lineSeg.setPoints(coords1, coords2);
}

void setupCircle(Part::GeomCircle& circle)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    double radius = 3.0;
    circle.setCenter(coordsCenter);
    circle.setRadius(radius);
}

void setupArcOfCircle(Part::GeomArcOfCircle& arcOfCircle)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double radius = 3.0;
    double startParam = M_PI / 3, endParam = M_PI * 1.5;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);
}

void setupEllipse(Part::GeomEllipse& ellipse)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    ellipse.setCenter(coordsCenter);
    ellipse.setMajorRadius(majorRadius);
    ellipse.setMinorRadius(minorRadius);
}

// TODO: How to set up B-splines here?
// It's not straightforward to change everything from a "default" one.

class SketchObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        // TODO: Do we add a body first, or is just adding sketch sufficient for this test?
        _sketchobj =
            static_cast<Sketcher::SketchObject*>(_doc->addObject("Sketcher::SketchObject"));
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Sketcher::SketchObject* getObject()
    {
        return _sketchobj;
    }

private:
    // TODO: use shared_ptr or something else here?
    Sketcher::SketchObject* _sketchobj;
    std::string _docName;
    std::vector<const char*> allowedTypes {"Vertex",
                                           "Edge",
                                           "ExternalEdge",
                                           "H_Axis",
                                           "V_Axis",
                                           "RootPoint"};
};

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
    // For operating on vertices, there is first a check if the vertex exists.
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
    auto ptNone = Sketcher::SketchObject::getPoint(&point, Sketcher::PointPos::none);

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
    auto ptMid = Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&lineSeg, Sketcher::PointPos::none);

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
    auto ptNone = Sketcher::SketchObject::getPoint(&circle, Sketcher::PointPos::none);

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
    auto ptNone = Sketcher::SketchObject::getPoint(&ellipse, Sketcher::PointPos::none);

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
    double radius = 3.0, startParam = M_PI / 3, endParam = M_PI * 1.5;
    Part::GeomArcOfCircle arcOfCircle;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&arcOfCircle, Sketcher::PointPos::none);

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
    double startParam = M_PI / 3, endParam = M_PI * 1.5;
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
    auto ptNone = Sketcher::SketchObject::getPoint(&arcOfEllipse, Sketcher::PointPos::none);

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
    double startParam = M_PI / 3, endParam = M_PI * 1.5;
    Part::GeomArcOfHyperbola arcOfHyperbola;
    arcOfHyperbola.setCenter(coordsCenter);
    arcOfHyperbola.setMajorRadius(majorRadius);
    arcOfHyperbola.setMinorRadius(minorRadius);
    arcOfHyperbola.setRange(startParam, endParam, true);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&arcOfHyperbola, Sketcher::PointPos::none);

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
    double startParam = M_PI / 3, endParam = M_PI * 1.5;
    Part::GeomArcOfParabola arcOfParabola;
    arcOfParabola.setCenter(coordsCenter);
    arcOfParabola.setFocal(focal);
    arcOfParabola.setRange(startParam, endParam, true);

    // Act
    auto ptStart = Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::start);
    auto ptMid = Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&arcOfParabola, Sketcher::PointPos::none);

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
    auto ptMid = Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::mid);
    auto ptEnd = Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&nonPeriodicBSpline, Sketcher::PointPos::none);

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
    auto ptMid = Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::mid);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptEnd = Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::end);
    // TODO: Maybe we want this to give an error instead of some default value
    auto ptNone = Sketcher::SketchObject::getPoint(&periodicBSpline, Sketcher::PointPos::none);

    // Assert
    // With non-trivial values for weights, knots, mults, etc, getting the coordinates is
    // non-trivial as well. This is the best we can do.
    EXPECT_DOUBLE_EQ(ptStart[0], ptEnd[0]);
    EXPECT_DOUBLE_EQ(ptStart[1], ptEnd[1]);
}

TEST_F(SketchObjectTest, testSplitLineSegment)
{
    // Arrange
    Base::Vector3d coords1(1.0, 2.0, 0.0);
    Base::Vector3d coords2(3.0, 4.0, 0.0);
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    Part::GeomLineSegment lineSeg;
    lineSeg.setPoints(coords1, coords2);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // One additional curve should be added
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: Expect the resultant curves are line segments and shape is conserved
}

TEST_F(SketchObjectTest, testSplitCircle)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    double radius = 3.0;
    Part::GeomCircle circle;
    circle.setCenter(coordsCenter);
    circle.setRadius(radius);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The circle should be split into an arc now
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: Expect the end points of the resultant curve are coincident.
}

TEST_F(SketchObjectTest, testSplitEllipse)
{
    // Arrange
    Base::Vector3d splitPoint(2.0, 3.1, 0.0);
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    Part::GeomEllipse ellipse;
    ellipse.setCenter(coordsCenter);
    ellipse.setMajorRadius(majorRadius);
    ellipse.setMinorRadius(minorRadius);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: The ellipse should be split into an arc of ellipse now
    // FIXME: Internal geometries may be added or removed which may cause some issues
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: Expect the end points of the resultant curve are coincident.
}

TEST_F(SketchObjectTest, testSplitArcOfCircle)
{
    // Arrange
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    Base::Vector3d splitPoint(-2.0, 3.1, 0.0);
    double radius = 3.0;
    double startParam = M_PI / 3, endParam = M_PI * 1.5;
    Part::GeomArcOfCircle arcOfCircle;
    arcOfCircle.setCenter(coordsCenter);
    arcOfCircle.setRadius(radius);
    arcOfCircle.setRange(startParam, endParam, true);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The arcOfCircle should be split into an arc now
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: Expect the end points of the resultant curve are coincident.
}

TEST_F(SketchObjectTest, testSplitArcOfConic)
{
    // Arrange
    // TODO: Define a parabola/hyperbola as reference
    Base::Vector3d splitPoint(1.0, -1.1, 0.0);
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double focal = 3.0;
    double startParam = -M_PI * 1.5, endParam = M_PI * 1.5;
    Part::GeomArcOfParabola arcOfConic;
    arcOfConic.setCenter(coordsCenter);
    arcOfConic.setFocal(focal);
    arcOfConic.setRange(startParam, endParam, true);
    int geoId = getObject()->addGeometry(&arcOfConic);

    // Act
    // TODO: Sample random points from both sides of the split
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // The arcOfConic should be split into two arcs of the same conic now
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: Expect the end points of the resultant curve are coincident.
}

TEST_F(SketchObjectTest, testSplitNonPeriodicBSpline)
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
    Base::Vector3d splitPoint(-0.5, 1.1, 0.0);
    int geoId = getObject()->addGeometry(&nonPeriodicBSpline);
    // TODO: Put a point on this

    // Act
    // TODO: sample before point(s) at a random parameter
    int result = getObject()->split(geoId, splitPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: confirm sampled point(s) is/are at the same place
}

TEST_F(SketchObjectTest, testSplitPeriodicBSpline)
{
    // Arrange

    // Act
    // TODO: sample before point(s) at a random parameter

    // Assert
    // TODO: confirm sampled point(s) is/are at the same place
}

TEST_F(SketchObjectTest, testTrimWithoutIntersection)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    int geoId = getObject()->addGeometry(&lineSeg);
    Base::Vector3d trimPoint(2.0, 3.1, 0.0);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // Once this line segment is trimmed, nothing should remain
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
}

TEST_F(SketchObjectTest, testTrimLineSegmentEnd)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(lineSeg.pointAtParameter(
        lineSeg.getFirstParameter()
        + (lineSeg.getLastParameter() - lineSeg.getFirstParameter()) * 0.2));
    Base::Vector3d p1(lineSeg.pointAtParameter(
        lineSeg.getFirstParameter()
        + (lineSeg.getLastParameter() - lineSeg.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this line segment is trimmed, the curve should be "smaller"
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimLineSegmentMid)
{
    // Arrange
    Part::GeomLineSegment lineSeg;
    setupLineSegment(lineSeg);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(lineSeg.pointAtParameter(
        lineSeg.getFirstParameter()
        + (lineSeg.getLastParameter() - lineSeg.getFirstParameter()) * 0.5));
    Base::Vector3d p1(lineSeg.pointAtParameter(
        lineSeg.getFirstParameter()
        + (lineSeg.getLastParameter() - lineSeg.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(lineSeg.pointAtParameter(
        lineSeg.getFirstParameter()
        + (lineSeg.getLastParameter() - lineSeg.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&lineSeg);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this line segment is trimmed, there should be two "smaller" curves in its place
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimCircleEnd)
{
    // Arrange
    Part::GeomCircle circle;
    setupCircle(circle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(
        circle.pointAtParameter(circle.getFirstParameter()
                                + (circle.getLastParameter() - circle.getFirstParameter()) * 0.2));
    Base::Vector3d p1(
        circle.pointAtParameter(circle.getFirstParameter()
                                + (circle.getLastParameter() - circle.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this circle is trimmed, the circle should be deleted.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimCircleMid)
{
    // Arrange
    Part::GeomCircle circle;
    setupCircle(circle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(
        circle.pointAtParameter(circle.getFirstParameter()
                                + (circle.getLastParameter() - circle.getFirstParameter()) * 0.5));
    Base::Vector3d p1(
        circle.pointAtParameter(circle.getFirstParameter()
                                + (circle.getLastParameter() - circle.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(
        circle.pointAtParameter(circle.getFirstParameter()
                                + (circle.getLastParameter() - circle.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&circle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this circle is trimmed, there should be one arc.
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimArcOfCircleEnd)
{
    // This should also cover as a representative of arc of conic

    // Arrange
    Part::GeomArcOfCircle arcOfCircle;
    setupArcOfCircle(arcOfCircle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(arcOfCircle.pointAtParameter(
        arcOfCircle.getFirstParameter()
        + (arcOfCircle.getLastParameter() - arcOfCircle.getFirstParameter()) * 0.2));
    Base::Vector3d p1(arcOfCircle.pointAtParameter(
        arcOfCircle.getFirstParameter()
        + (arcOfCircle.getLastParameter() - arcOfCircle.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimArcOfCircleMid)
{
    // Arrange
    Part::GeomArcOfCircle arcOfCircle;
    setupArcOfCircle(arcOfCircle);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(arcOfCircle.pointAtParameter(
        arcOfCircle.getFirstParameter()
        + (arcOfCircle.getLastParameter() - arcOfCircle.getFirstParameter()) * 0.5));
    Base::Vector3d p1(arcOfCircle.pointAtParameter(
        arcOfCircle.getFirstParameter()
        + (arcOfCircle.getLastParameter() - arcOfCircle.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(arcOfCircle.pointAtParameter(
        arcOfCircle.getFirstParameter()
        + (arcOfCircle.getLastParameter() - arcOfCircle.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&arcOfCircle);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimEllipseEnd)
{
    // Arrange
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(ellipse.pointAtParameter(
        ellipse.getFirstParameter()
        + (ellipse.getLastParameter() - ellipse.getFirstParameter()) * 0.2));
    Base::Vector3d p1(ellipse.pointAtParameter(
        ellipse.getFirstParameter()
        + (ellipse.getLastParameter() - ellipse.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this ellipse is trimmed, the ellipse should be deleted.
    // FIXME: This check fails because internal geometries get deleted.
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimEllipseMid)
{
    // Arrange
    Part::GeomEllipse ellipse;
    setupEllipse(ellipse);
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(ellipse.pointAtParameter(
        ellipse.getFirstParameter()
        + (ellipse.getLastParameter() - ellipse.getFirstParameter()) * 0.5));
    Base::Vector3d p1(ellipse.pointAtParameter(
        ellipse.getFirstParameter()
        + (ellipse.getLastParameter() - ellipse.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(ellipse.pointAtParameter(
        ellipse.getFirstParameter()
        + (ellipse.getLastParameter() - ellipse.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&ellipse);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // TODO: Once this ellipse is trimmed, there should be one arc.
    // FIXME: This check fails because internal geometries get deleted.
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimPeriodicBSplineEnd)
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
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(periodicBSpline.pointAtParameter(
        periodicBSpline.getFirstParameter()
        + (periodicBSpline.getLastParameter() - periodicBSpline.getFirstParameter()) * 0.2));
    Base::Vector3d p1(periodicBSpline.pointAtParameter(
        periodicBSpline.getFirstParameter()
        + (periodicBSpline.getLastParameter() - periodicBSpline.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&periodicBSpline);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // FIXME: This will fail because of deleted internal geometry
    // TODO: Once this periodicBSpline is trimmed, the periodicBSpline should be deleted.
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId - 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimPeriodicBSplineMid)
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
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(periodicBSpline.pointAtParameter(
        periodicBSpline.getFirstParameter()
        + (periodicBSpline.getLastParameter() - periodicBSpline.getFirstParameter()) * 0.5));
    Base::Vector3d p1(periodicBSpline.pointAtParameter(
        periodicBSpline.getFirstParameter()
        + (periodicBSpline.getLastParameter() - periodicBSpline.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(periodicBSpline.pointAtParameter(
        periodicBSpline.getFirstParameter()
        + (periodicBSpline.getLastParameter() - periodicBSpline.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&periodicBSpline);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // FIXME: This will fail because of deleted internal geometry
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
}

TEST_F(SketchObjectTest, testTrimNonPeriodicBSplineEnd)
{
    // This should also cover as a representative of arc of conic

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
    // create curves intersecting at the right spots
    Base::Vector3d trimPoint(nonPeriodicBSpline.pointAtParameter(
        nonPeriodicBSpline.getFirstParameter()
        + (nonPeriodicBSpline.getLastParameter() - nonPeriodicBSpline.getFirstParameter()) * 0.2));
    Base::Vector3d p1(nonPeriodicBSpline.pointAtParameter(
        nonPeriodicBSpline.getFirstParameter()
        + (nonPeriodicBSpline.getLastParameter() - nonPeriodicBSpline.getFirstParameter()) * 0.5));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    int geoId = getObject()->addGeometry(&nonPeriodicBSpline);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // FIXME: This will fail because of deleted internal geometry
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
}

TEST_F(SketchObjectTest, testTrimNonPeriodicBSplineMid)
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
    // TODO: create curves intersecting at the right spots
    Base::Vector3d trimPoint(nonPeriodicBSpline.pointAtParameter(
        nonPeriodicBSpline.getFirstParameter()
        + (nonPeriodicBSpline.getLastParameter() - nonPeriodicBSpline.getFirstParameter()) * 0.5));
    Base::Vector3d p1(nonPeriodicBSpline.pointAtParameter(
        nonPeriodicBSpline.getFirstParameter()
        + (nonPeriodicBSpline.getLastParameter() - nonPeriodicBSpline.getFirstParameter()) * 0.3));
    Base::Vector3d p2(p1.x + 0.1, p1.y + 0.1, p1.z);
    Part::GeomLineSegment lineSegCut1;
    lineSegCut1.setPoints(p1, p2);
    getObject()->addGeometry(&lineSegCut1);
    Base::Vector3d p3(nonPeriodicBSpline.pointAtParameter(
        nonPeriodicBSpline.getFirstParameter()
        + (nonPeriodicBSpline.getLastParameter() - nonPeriodicBSpline.getFirstParameter()) * 0.7));
    Base::Vector3d p4(p3.x + 0.1, p3.y + 0.1, p3.z);
    Part::GeomLineSegment lineSegCut2;
    lineSegCut2.setPoints(p3, p4);
    getObject()->addGeometry(&lineSegCut2);
    int geoId = getObject()->addGeometry(&nonPeriodicBSpline);

    // Act
    int result = getObject()->trim(geoId, trimPoint);

    // Assert
    EXPECT_EQ(result, 0);
    // FIXME: This will fail because of deleted internal geometry
    // EXPECT_EQ(getObject()->getHighestCurveIndex(), geoId + 1);
    // TODO: There should be a "point-on-object" constraint on the intersecting curves
    // TODO: Ensure shape is preserved
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

// FIXME: This test is "brittle" and sensitive to order of all tests, and thus invalid
// TEST_F(SketchObjectTest, testGetElementName)
// {
//     // Arrange
//     Base::Vector3d p1(0.0, 0.0, 0.0), p2(1.0, 0.0, 0.0);
//     std::unique_ptr<Part::Geometry> geoline(new Part::GeomLineSegment());
//     static_cast<Part::GeomLineSegment*>(geoline.get())->setPoints(p1, p2);
//     getObject()->addGeometry(geoline.get());
//     getObject()->recomputeFeature();  // or ->execute()
//     // Act
//     // unless it's Export, we are really just testing the superclass
//     App::GeoFeature::getElementName
//     // call.
//     auto forward_normal_name =
//         getObject()->getElementName("g39;SKT", App::GeoFeature::ElementNameType::Normal);
//     auto reverse_normal_name =
//         getObject()->getElementName("Vertex2", App::GeoFeature::ElementNameType::Normal);
//     auto reverse_export_name =
//         getObject()->getElementName("Vertex1", App::GeoFeature::ElementNameType::Export);
//     auto map = getObject()->Shape.getShape().getElementMap();
//     ASSERT_EQ(map.size(), 3);
//     EXPECT_STREQ(map[0].name.toString().c_str(), "g39;SKT");
//     EXPECT_EQ(map[0].index.toString(), "Edge1");
//     // Assert
// #ifndef FC_USE_TNP_FIX
//     EXPECT_STREQ(forward_normal_name.first.c_str(), "");
//     EXPECT_STREQ(forward_normal_name.second.c_str(), "g39;SKT");
//     EXPECT_STREQ(reverse_normal_name.first.c_str(), "");
//     EXPECT_STREQ(reverse_normal_name.second.c_str(), "Vertex2");
//     EXPECT_STREQ(reverse_export_name.first.c_str(), ";g39v1;SKT.Vertex1");
//     EXPECT_STREQ(reverse_export_name.second.c_str(), "Vertex1");
// #else
//     EXPECT_STREQ(forward_normal_name.first.c_str(), ";g39;SKT.Edge1");
//     EXPECT_STREQ(forward_normal_name.second.c_str(), "Edge1");
//     EXPECT_STREQ(reverse_normal_name.first.c_str(), ";g39v2;SKT.Vertex2");
//     EXPECT_STREQ(reverse_normal_name.second.c_str(), "Vertex2");
//     EXPECT_STREQ(reverse_export_name.first.c_str(), ";g39v1;SKT.Vertex1");
//     EXPECT_STREQ(reverse_export_name.second.c_str(), "Vertex1");
// #endif
// }
