// SPDX-License-Identifier: LGPL-2.1-or-later

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <src/App/InitApplication.h>
#include "SketcherTestHelpers.h"


void SketchObjectTest::SetUpTestSuite()
{
    tests::initApplication();
}

void SketchObjectTest::SetUp()
{
    _docName = App::GetApplication().getUniqueDocumentName("test");
    auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    // TODO: Do we add a body newName, or is just adding sketch sufficient for this test?
    _sketchobj = static_cast<Sketcher::SketchObject*>(_doc->addObject("Sketcher::SketchObject"));
}

void SketchObjectTest::TearDown()
{
    App::GetApplication().closeDocument(_docName.c_str());
}

Sketcher::SketchObject* SketchObjectTest::getObject()
{
    return _sketchobj;
}

namespace SketcherTestHelpers
{

using namespace Sketcher;

void setupLineSegment(Part::GeomLineSegment& lineSeg)
{
    Base::Vector3d coords1(1.0, 2.0, 0.0);
    Base::Vector3d coords2(3.0, 4.0, 0.0);
    lineSeg.setPoints(coords1, coords2);
}

void setupCircle(Part::GeomCircle& circle)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double radius = 3.0;
    circle.setCenter(coordsCenter);
    circle.setRadius(radius);
}

void setupArcOfCircle(Part::GeomArcOfCircle& arcOfCircle)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double radius = 3.0;
    double startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
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

void setupArcOfHyperbola(Part::GeomArcOfHyperbola& arcOfHyperbola)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double majorRadius = 4.0;
    double minorRadius = 3.0;
    double startParam = std::numbers::pi / 3, endParam = std::numbers::pi * 1.5;
    arcOfHyperbola.setCenter(coordsCenter);
    arcOfHyperbola.setMajorRadius(majorRadius);
    arcOfHyperbola.setMinorRadius(minorRadius);
    arcOfHyperbola.setRange(startParam, endParam, true);
}

void setupArcOfParabola(Part::GeomArcOfParabola& aop)
{
    Base::Vector3d coordsCenter(1.0, 2.0, 0.0);
    double focal = 3.0;
    double startParam = -std::numbers::pi * 1.5, endParam = std::numbers::pi * 1.5;
    aop.setCenter(coordsCenter);
    aop.setFocal(focal);
    aop.setRange(startParam, endParam, true);
}

std::unique_ptr<Part::GeomBSplineCurve> createTypicalNonPeriodicBSpline()
{
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
    return std::make_unique<Part::GeomBSplineCurve>(poles,
                                                    weights,
                                                    knotsNonPeriodic,
                                                    multiplicitiesNonPeriodic,
                                                    degree,
                                                    false);
}

std::unique_ptr<Part::GeomBSplineCurve> createTypicalPeriodicBSpline()
{
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
    return std::make_unique<Part::GeomBSplineCurve>(poles,
                                                    weights,
                                                    knotsPeriodic,
                                                    multiplicitiesPeriodic,
                                                    degree,
                                                    true);
}

int countConstraintsOfType(const Sketcher::SketchObject* obj, const Sketcher::ConstraintType cType)
{
    const std::vector<Sketcher::Constraint*>& constraints = obj->Constraints.getValues();

    int result = std::ranges::count(constraints, cType, &Constraint::Type);

    return result;
}

Base::Vector3d getPointAtNormalizedParameter(const Part::GeomCurve& curve, double param)
{
    return curve.pointAtParameter(curve.getFirstParameter()
                                  + (curve.getLastParameter() - curve.getFirstParameter()) * param);
}
}  // namespace SketcherTestHelpers
