// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

class GeoFeatureGroupTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { tests::initApplication(); }

    void SetUp() override {
        _docName = App::GetApplication().getUniqueDocumentName("GeoTest");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* _doc = nullptr;
    std::string _docName;
};

TEST_F(GeoFeatureGroupTest, testIsLinkValidForDocumentObjectGroup) {
    auto group = _doc->addObject<App::DocumentObjectGroup>("Group");

    auto body1 = _doc->addObject<PartDesign::Body>("Body1");
    auto body2 = _doc->addObject<PartDesign::Body>("Body2");

    group->addObject(body1);
    group->addObject(body2);

    auto prop = group->getPropertyByName("Group");
    ASSERT_TRUE(prop != nullptr);

    EXPECT_TRUE(App::GeoFeatureGroupExtension::isLinkValid(prop));
}

TEST_F(GeoFeatureGroupTest, testIsLinkValidCrossFailure) {
    auto body1 = _doc->addObject<PartDesign::Body>("Body1");
    auto body2 = _doc->addObject<PartDesign::Body>("Body2");

    auto feature = _doc->addObject<PartDesign::Feature>("Feature");
    body2->addObject(feature);

    auto linkProp = new App::PropertyLink();
    linkProp->setContainer(body1);
    linkProp->setValue(feature);

    bool valid = App::GeoFeatureGroupExtension::isLinkValid(linkProp);
    EXPECT_FALSE(valid);
}