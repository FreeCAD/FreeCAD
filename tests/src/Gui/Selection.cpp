// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeatureTest.h>
#include <Base/Exception.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Selection/SelectionObject.h>

#include <src/App/InitApplication.h>


namespace
{

class TestSelectionSingleton: public Gui::SelectionSingleton
{
public:
    TestSelectionSingleton()
    {
        if (!_pcSingleton) {
            _pcSingleton = this;
        }
    }

    bool addSelectionDescription(
        const char* pDocName,
        const char* pObjectName,
        const char* pSubName,
        float x,
        float y,
        float z
    )
    {
        auto context = getSelectionContext(pDocName);
        if (!context.info) {
            return false;
        }

        const char* subName = pSubName;
        SelectionDescription sel;
        auto checkResult
            = checkSelection(pDocName, pObjectName, subName, Gui::ResolveMode::NoResolve, sel);
        if (checkResult != SelectionCheckResult::Available) {
            return false;
        }

        sel.x = x;
        sel.y = y;
        sel.z = z;
        context.info->selList.push_back(sel);
        return true;
    }
};

TestSelectionSingleton& testSelection()
{
    static auto* selection = new TestSelectionSingleton;
    return *selection;
}

class SelectionInTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        docName = App::GetApplication().getUniqueDocumentName("selection_in");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        App::GetApplication().setActiveDocument(doc);

        root = doc->addObject<App::DocumentObjectGroup>("Root");
        middle = doc->addObject<App::DocumentObjectGroup>("Middle");
        leaf = doc->addObject<App::FeatureTest>("Leaf");

        root->addObject(middle);
        middle->addObject(leaf);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(docName.c_str());
    }

    [[nodiscard]] std::string selectedSubName() const
    {
        return std::string(middle->getNameInDocument()) + "." + leaf->getNameInDocument() + ".";
    }

    [[nodiscard]] std::string leafSubName() const
    {
        return std::string(leaf->getNameInDocument()) + ".";
    }

    [[nodiscard]] bool selectLeafThroughRoot() const
    {
        return testSelection().addSelectionDescription(
            doc->getName(),
            root->getNameInDocument(),
            selectedSubName().c_str(),
            1.0F,
            2.0F,
            3.0F
        );
    }

    std::string docName;
    App::Document* doc {nullptr};
    App::DocumentObjectGroup* root {nullptr};
    App::DocumentObjectGroup* middle {nullptr};
    App::FeatureTest* leaf {nullptr};
};

class SelectionFilterTest: public SelectionInTest
{
};

}  // namespace

TEST_F(SelectionInTest, ReturnsDirectChildAndRemainingSubNameInsideContainer)
{
    ASSERT_TRUE(selectLeafThroughRoot());

    auto selections = testSelection().getSelectionIn(root, App::FeatureTest::getClassTypeId());

    ASSERT_EQ(selections.size(), 1);
    EXPECT_EQ(selections.front().getObject(), middle);
    EXPECT_EQ(selections.front().getSubNames(), std::vector<std::string>({leafSubName()}));

    auto pickedPoints = selections.front().getPickedPoints();
    ASSERT_EQ(pickedPoints.size(), 1);
    EXPECT_DOUBLE_EQ(pickedPoints.front().x, 1.0);
    EXPECT_DOUBLE_EQ(pickedPoints.front().y, 2.0);
    EXPECT_DOUBLE_EQ(pickedPoints.front().z, 3.0);
}

TEST_F(SelectionInTest, ReturnsSelectedChildAfterContainer)
{
    ASSERT_TRUE(selectLeafThroughRoot());

    auto selections = testSelection().getSelectionIn(middle, App::FeatureTest::getClassTypeId());

    ASSERT_EQ(selections.size(), 1);
    EXPECT_EQ(selections.front().getObject(), leaf);
    EXPECT_TRUE(selections.front().getSubNames().empty());
    EXPECT_TRUE(selections.front().getPickedPoints().empty());
}

TEST_F(SelectionFilterTest, TestAcceptsMatchingTypeAndSubElementPrefix)
{
    Gui::SelectionFilter filter("SELECT App::FeatureTest SUBELEMENT Edge");

    EXPECT_TRUE(filter.test(leaf, nullptr));
    EXPECT_TRUE(filter.test(leaf, "Edge1"));
    EXPECT_FALSE(filter.test(leaf, "Face1"));
    EXPECT_FALSE(filter.test(middle, "Edge1"));
}

TEST_F(SelectionFilterTest, ValidFilterParses)
{
    Gui::SelectionFilter filter("SELECT App::FeatureTest COUNT 1");

    EXPECT_TRUE(filter.isValid());
    EXPECT_EQ(filter.getFilter(), "SELECT App::FeatureTest COUNT 1");
}

TEST_F(SelectionFilterTest, NullAndEmptyFilterResetToInvalid)
{
    Gui::SelectionFilter filter("SELECT App::FeatureTest COUNT 1");
    ASSERT_TRUE(filter.isValid());

    filter.setFilter(static_cast<const char*>(nullptr));

    EXPECT_FALSE(filter.isValid());
    EXPECT_TRUE(filter.getFilter().empty());

    filter.setFilter("SELECT App::FeatureTest COUNT 1");
    ASSERT_TRUE(filter.isValid());

    filter.setFilter("");

    EXPECT_FALSE(filter.isValid());
    EXPECT_TRUE(filter.getFilter().empty());
}

TEST_F(SelectionFilterTest, MalformedFilterThrowsParserError)
{
    EXPECT_THROW(Gui::SelectionFilter filter("SELECT"), Base::ParserError);
}

TEST_F(SelectionFilterTest, SetFilterRecoversFromEmptyToValid)
{
    Gui::SelectionFilter filter("");
    ASSERT_FALSE(filter.isValid());

    filter.setFilter("SELECT App::FeatureTest COUNT 1");

    EXPECT_TRUE(filter.isValid());
    EXPECT_EQ(filter.getFilter(), "SELECT App::FeatureTest COUNT 1");
}

TEST_F(SelectionFilterTest, MatchAcceptsSelectedObjectCount)
{
    ASSERT_TRUE(
        testSelection()
            .addSelectionDescription(doc->getName(), leaf->getNameInDocument(), nullptr, 1.0F, 2.0F, 3.0F)
    );

    Gui::SelectionFilter filter("SELECT App::FeatureTest COUNT 1");

    EXPECT_TRUE(filter.match());
    ASSERT_EQ(filter.Result.size(), 1);
    ASSERT_EQ(filter.Result.front().size(), 1);
    EXPECT_EQ(filter.Result.front().front().getObject(), leaf);
}

TEST_F(SelectionFilterTest, MatchRejectsSelectionCountOutsideSlice)
{
    ASSERT_TRUE(
        testSelection()
            .addSelectionDescription(doc->getName(), leaf->getNameInDocument(), nullptr, 1.0F, 2.0F, 3.0F)
    );

    Gui::SelectionFilter filter("SELECT App::FeatureTest COUNT 2");

    EXPECT_FALSE(filter.match());
    EXPECT_TRUE(filter.Result.empty());
}

TEST_F(SelectionFilterTest, MatchUsesContainerScopedSelection)
{
    ASSERT_TRUE(selectLeafThroughRoot());

    Gui::SelectionFilter filter("SELECT App::FeatureTest SUBELEMENT Leaf COUNT 1", root);

    EXPECT_TRUE(filter.match());
    ASSERT_EQ(filter.Result.size(), 1);
    ASSERT_EQ(filter.Result.front().size(), 1);
    EXPECT_EQ(filter.Result.front().front().getObject(), middle);
    EXPECT_EQ(filter.Result.front().front().getSubNames(), std::vector<std::string>({leafSubName()}));
}
