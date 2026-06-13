// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <memory>

#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/GeoFeatureGroupExtension.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/Sketcher/App/SketchObject.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class GeoFeatureGroupTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("GeoTest");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* _doc = nullptr;
    std::string _docName;
};

void createRectangle(Sketcher::SketchObject* sketch, double width, double height)
{
    auto line1 = std::make_unique<Part::GeomLineSegment>();
    line1->setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(width, 0, 0));
    sketch->addGeometry(line1.release(), false);

    auto line2 = std::make_unique<Part::GeomLineSegment>();
    line2->setPoints(Base::Vector3d(width, 0, 0), Base::Vector3d(width, height, 0));
    sketch->addGeometry(line2.release(), false);

    auto line3 = std::make_unique<Part::GeomLineSegment>();
    line3->setPoints(Base::Vector3d(width, height, 0), Base::Vector3d(0, height, 0));
    sketch->addGeometry(line3.release(), false);

    auto line4 = std::make_unique<Part::GeomLineSegment>();
    line4->setPoints(Base::Vector3d(0, height, 0), Base::Vector3d(0, 0, 0));
    sketch->addGeometry(line4.release(), false);

    auto createConstraint = [](Sketcher::ConstraintType type,
                               int geo1,
                               Sketcher::PointPos pos1,
                               int geo2,
                               Sketcher::PointPos pos2) {
        auto c = std::make_unique<Sketcher::Constraint>();

        c->Type = type;

        c->setGeoId(0, geo1);
        c->setPosId(0, pos1);
        c->setGeoId(1, geo2);
        c->setPosId(1, pos2);

        return c;
    };

    sketch->addConstraint(createConstraint(
                              Sketcher::ConstraintType::Coincident,
                              0,
                              Sketcher::PointPos::end,
                              1,
                              Sketcher::PointPos::start
    )
                              .release());

    sketch->addConstraint(createConstraint(
                              Sketcher::ConstraintType::Coincident,
                              1,
                              Sketcher::PointPos::end,
                              2,
                              Sketcher::PointPos::start
    )
                              .release());

    sketch->addConstraint(createConstraint(
                              Sketcher::ConstraintType::Coincident,
                              2,
                              Sketcher::PointPos::end,
                              3,
                              Sketcher::PointPos::start
    )
                              .release());

    sketch->addConstraint(createConstraint(
                              Sketcher::ConstraintType::Coincident,
                              3,
                              Sketcher::PointPos::end,
                              0,
                              Sketcher::PointPos::start
    )
                              .release());
}

TEST_F(GeoFeatureGroupTest, testDocumentObjectGroupAcceptsBoolean)
{
    auto group = _doc->addObject<App::DocumentObjectGroup>("Group");

    auto body1 = _doc->addObject<PartDesign::Body>("Body10x10");
    group->addObject(body1);

    auto sketch1 = _doc->addObject<Sketcher::SketchObject>("Sketch1");
    body1->addObject(sketch1);
    createRectangle(sketch1, 10.0, 10.0);

    auto pad1 = _doc->addObject<PartDesign::Pad>("Pad10");
    body1->addObject(pad1);
    pad1->Profile.setValue(sketch1);
    pad1->Length.setValue(10.0);
    body1->Group.setValue({sketch1, pad1});

    auto body2 = _doc->addObject<PartDesign::Body>("Body10x20");
    group->addObject(body2);

    auto sketch2 = _doc->addObject<Sketcher::SketchObject>("Sketch2");
    body2->addObject(sketch2);
    createRectangle(sketch2, 10.0, 20.0);

    auto pad2 = _doc->addObject<PartDesign::Pad>("Pad20");
    body2->addObject(pad2);
    pad2->Profile.setValue(sketch2);
    pad2->Length.setValue(20.0);  // 20 in height difference
    body2->Group.setValue({sketch2, pad2});

    auto boolOp = _doc->addObject<PartDesign::Boolean>("BooleanOp");
    body1->addObject(boolOp);

    boolOp->Type.setValue("Fuse");

    boolOp->Group.setValue({body2});
    _doc->recompute();

    std::vector<App::Property*> list;
    group->getPropertyList(list);
    for (App::Property* prop : list) {
        EXPECT_TRUE(App::GeoFeatureGroupExtension::isLinkValid(prop));
    }
}

TEST_F(GeoFeatureGroupTest, testIsLinkValidCrossFailure)
{
    auto body1 = _doc->addObject<PartDesign::Body>("Body1");
    auto body2 = _doc->addObject<PartDesign::Body>("Body2");

    auto feature = _doc->addObject<PartDesign::Feature>("Feature");
    body2->addObject(feature);

    App::PropertyLink linkProp;
    linkProp.setContainer(body1);
    linkProp.setValue(feature);

    bool valid = App::GeoFeatureGroupExtension::isLinkValid(&linkProp);
    EXPECT_FALSE(valid);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
