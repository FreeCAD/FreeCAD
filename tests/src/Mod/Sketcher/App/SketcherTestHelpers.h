// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <FCConfig.h>


#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/SketchObject.h>


class SketchObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite();
    void SetUp() override;
    void TearDown() override;
    Sketcher::SketchObject* getObject();

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

namespace SketcherTestHelpers
{

using namespace Sketcher;

void setupLineSegment(Part::GeomLineSegment& lineSeg);

void setupCircle(Part::GeomCircle& circle);

void setupArcOfCircle(Part::GeomArcOfCircle& arcOfCircle);

void setupEllipse(Part::GeomEllipse& ellipse);

void setupArcOfHyperbola(Part::GeomArcOfHyperbola& arcOfHyperbola);

void setupArcOfParabola(Part::GeomArcOfParabola& aop);

std::unique_ptr<Part::GeomBSplineCurve> createTypicalNonPeriodicBSpline();

std::unique_ptr<Part::GeomBSplineCurve> createTypicalPeriodicBSpline();

int countConstraintsOfType(const Sketcher::SketchObject* obj, const Sketcher::ConstraintType cType);

// Get point at the parameter after scaling the range to [0, 1].
Base::Vector3d getPointAtNormalizedParameter(const Part::GeomCurve& curve, double param);

// TODO: How to set up B-splines here?
// It's not straightforward to change everything from a "default" one.
}  // namespace SketcherTestHelpers
