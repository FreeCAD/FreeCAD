// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Property.h>
#include <App/SuppressibleExtension.h>
#include <gp_Dir.hxx>
#include <Precision.hxx>

#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/LinkArrayLinear.h>
#include <Mod/Part/App/LinkArrayPolar.h>
#include <src/App/InitApplication.h>

class LinkArrayPolarTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Part::LinearPatternExtension::init();
        Part::PolarPatternExtension::init();
        Part::LinkArray::init();
        Part::LinkArrayLinear::init();
        Part::LinkArrayPolar::init();
        Part::AttachExtension::init();
        Part::Primitive::init();
        Part::Box::init();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* _doc {};
    std::string _docName;
};

TEST_F(LinkArrayPolarTest, objectAxes)
{
    Part::LinkArrayPolar array;

    const auto checkAxis = [&array](const char* role, const gp_Dir& expected) {
        array.Axis.setValue(nullptr, std::vector<std::string> {role});
        EXPECT_TRUE(array.getRotation().Direction().IsEqual(expected, Precision::Angular()));
    };

    checkAxis("X_Axis", gp_Dir(1.0, 0.0, 0.0));
    checkAxis("Y_Axis", gp_Dir(0.0, 1.0, 0.0));
    checkAxis("Z_Axis", gp_Dir(0.0, 0.0, 1.0));
}

TEST_F(LinkArrayPolarTest, showElementDefaultsToExpandedAndEditable)
{
    Part::LinkArray array;

    EXPECT_TRUE(array.ShowElement.getValue());
    EXPECT_FALSE(array.ShowElement.testStatus(App::Property::Immutable));
    EXPECT_FALSE(array.ShowElement.testStatus(App::Property::Hidden));
}

TEST_F(LinkArrayPolarTest, expandedElementsFollowPatternPlacementChanges)
{
    auto* box = _doc->addObject<Part::Box>("Box");
    auto* array = _doc->addObject<Part::LinkArrayLinear>("LinearArray");
    array->LinkedObject.setValue(box);
    array->Occurrences.setValue(2);
    array->Occurrences2.setValue(1);
    array->Length.setValue(10.0);

    array->execute();

    auto elements = array->ElementList.getValues();
    ASSERT_EQ(2, elements.size());
    auto* secondPlacement = dynamic_cast<App::PropertyPlacement*>(
        elements[1]->getPropertyByName("Placement")
    );
    ASSERT_NE(secondPlacement, nullptr);
    EXPECT_DOUBLE_EQ(secondPlacement->getValue().getPosition().x, 10.0);

    array->Length.setValue(20.0);
    array->execute();

    elements = array->ElementList.getValues();
    ASSERT_EQ(2, elements.size());
    EXPECT_EQ(
        dynamic_cast<App::PropertyPlacement*>(elements[1]->getPropertyByName("Placement")),
        secondPlacement
    );
    EXPECT_DOUBLE_EQ(secondPlacement->getValue().getPosition().x, 20.0);
}

TEST_F(LinkArrayPolarTest, suppressedExpandedElementIsOmittedFromSubObjects)
{
    auto* box = _doc->addObject<Part::Box>("Box");
    auto* array = _doc->addObject<Part::LinkArrayLinear>("LinearArray");
    array->LinkedObject.setValue(box);
    array->Occurrences.setValue(2);
    array->Occurrences2.setValue(1);

    array->execute();

    auto elements = array->ElementList.getValues();
    ASSERT_EQ(2, elements.size());

    auto* suppressible = elements[1]->getExtensionByType<App::SuppressibleExtension>(true);
    ASSERT_NE(suppressible, nullptr);
    suppressible->Suppressed.setValue(true);

    auto subObjects = array->getSubObjects();
    ASSERT_EQ(1, subObjects.size());
    EXPECT_EQ("0.", subObjects[0]);
    EXPECT_EQ(elements[1], array->getSubObject("1."));
    EXPECT_EQ(nullptr, array->getSubObject("1.Face1"));
}
