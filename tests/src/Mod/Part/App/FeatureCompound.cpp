// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyLinks.h>
#include "Mod/Part/App/FeatureCompound.h"
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeatureCompoundTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _compound = _doc->addObject<Part::Compound>();
    }

    void TearDown() override
    {}

    Part::Compound* _compound = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureCompoundTest, testIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0], _boxes[1]});
    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 3.0, 3.0)));
    EXPECT_EQ(ts.countSubShapes(TopAbs_SHAPE), 2);
}

TEST_F(FeatureCompoundTest, testNonIntersecting)
{
    // Arrange
    _compound->Links.setValues({_boxes[0], _boxes[2]});
    // Act
    _compound->execute();
    Part::TopoShape ts = _compound->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 5.0, 3.0)));
    EXPECT_EQ(ts.countSubShapes(TopAbs_SHAPE), 2);
}

TEST_F(FeatureCompoundTest, xlinkFromOpenDocumentKeepsMappedNameWhenTargetReopens)
{
    // Arrange
    auto& app = App::GetApplication();
    auto compound = _doc->addObject<Part::Compound2>();
    compound->Links.setValues({_boxes[0]});
    _doc->recompute();
    _doc->saveAs(App::Application::getTempFileName().c_str());
    const std::string targetPath = _doc->getFileName();
    auto source = app.newDocument(app.getUniqueDocumentName("Source").c_str());
    source->saveAs(App::Application::getTempFileName().c_str());
    auto link = static_cast<App::PropertyXLinkSub*>(
        source->addObject<Part::Feature>()->addDynamicProperty("App::PropertyXLinkSub", "Ref")
    );
    link->setValue(compound, "Face6");
    app.closeDocument(_docName.c_str());

    // Act
    app.openDocument(targetPath.c_str());

    // Assert
    EXPECT_FALSE(link->getShadowSubs().front().newName.empty());
}

TEST_F(FeatureCompoundTest, xlinkKeepsMappedNameWhenOpenedWithTarget)
{
    // Arrange
    auto& app = App::GetApplication();
    auto compound = _doc->addObject<Part::Compound2>();
    compound->Links.setValues({_boxes[0]});
    _doc->recompute();
    _doc->saveAs(App::Application::getTempFileName().c_str());
    const std::string sourceName = app.getUniqueDocumentName("Source");
    auto source = app.newDocument(sourceName.c_str());
    source->saveAs(App::Application::getTempFileName().c_str());
    auto holder = source->addObject<Part::Feature>();
    auto link = static_cast<App::PropertyXLinkSub*>(
        holder->addDynamicProperty("App::PropertyXLinkSub", "Ref")
    );
    link->setValue(compound, "Face6");
    source->saveAs(App::Application::getTempFileName().c_str());
    const std::string sourcePath = source->getFileName();
    const std::string holderName = holder->getNameInDocument();
    app.closeDocument(sourceName.c_str());
    app.closeDocument(_docName.c_str());

    // Act
    auto reopened = app.openDocument(sourcePath.c_str());

    // Assert
    auto reopenedLink = static_cast<App::PropertyXLinkSub*>(
        reopened->getObject(holderName.c_str())->getPropertyByName("Ref")
    );
    EXPECT_FALSE(reopenedLink->getShadowSubs().front().newName.empty());
}
