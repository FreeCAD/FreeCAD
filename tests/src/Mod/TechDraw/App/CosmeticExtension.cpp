// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/LineFormat.h>

class TestCosmeticExtension: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        Base::Interpreter().loadModule("TechDraw");
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _view = _doc->addObject<TechDraw::DrawViewPart>("View");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    std::string addFormat(int index)
    {
        TechDraw::GeomFormat format(index, TechDraw::LineFormat());
        return _view->addGeomFormat(&format);
    }

    TechDraw::DrawViewPart* view() const
    {
        return _view;
    }

private:
    std::string _docName;
    App::Document* _doc {};
    TechDraw::DrawViewPart* _view {};
};

TEST_F(TestCosmeticExtension, removeGeomFormatKeepsTheOthers)
{
    std::vector<std::string> tags {addFormat(1), addFormat(2), addFormat(3)};

    view()->removeGeomFormat(tags[1]);

    EXPECT_EQ(view()->GeomFormats.getSize(), 2);
    EXPECT_NE(view()->getGeomFormat(tags[0]), nullptr);
    EXPECT_EQ(view()->getGeomFormat(tags[1]), nullptr);
    EXPECT_NE(view()->getGeomFormat(tags[2]), nullptr);
}

TEST_F(TestCosmeticExtension, removeGeomFormatOneByOne)
{
    std::vector<std::string> tags {addFormat(1), addFormat(2), addFormat(3)};

    for (const auto& tag : tags) {
        view()->removeGeomFormat(tag);
        EXPECT_EQ(view()->getGeomFormat(tag), nullptr);
    }

    EXPECT_EQ(view()->GeomFormats.getSize(), 0);
}

TEST_F(TestCosmeticExtension, removeGeomFormatUnknownTag)
{
    std::string tag = addFormat(1);

    view()->removeGeomFormat("not-a-tag");

    EXPECT_EQ(view()->GeomFormats.getSize(), 1);
    EXPECT_NE(view()->getGeomFormat(tag), nullptr);
}
