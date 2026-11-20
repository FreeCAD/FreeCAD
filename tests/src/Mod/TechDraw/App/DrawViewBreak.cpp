// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>
#include <Mod/TechDraw/App/DrawViewBreak.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

class TestDrawViewBreak: public ::testing::Test
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
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* document() const
    {
        return _doc;
    }

    static TechDraw::DrawViewBreak* addBreak(TechDraw::DrawViewPart* view)
    {
        return view->addBreak(Base::Vector3d(3.0, 0.0, 0.0),
                              Base::Vector3d(7.0, 0.0, 0.0),
                              Base::Vector3d(1.0, 0.0, 0.0),
                              1.0,
                              TechDraw::DrawViewPart::BreakType::ZIGZAG);
    }

private:
    std::string _docName;
    App::Document* _doc {};
};

TEST_F(TestDrawViewBreak, partViewTakesBreaks)
{
    auto* view = document()->addObject<TechDraw::DrawViewPart>("View");

    EXPECT_TRUE(view->supportsViewBreaks());
    EXPECT_NE(addBreak(view), nullptr);
    EXPECT_EQ(view->getBreakCount(), 1U);
}

// Sections, details and legacy broken views build their geometry without
// applying view breaks, so they must neither take nor report any.
TEST_F(TestDrawViewBreak, viewsWithoutBreakSupportIgnoreBreaks)
{
    auto* part = document()->addObject<TechDraw::DrawViewPart>("View");
    auto* viewBreak = addBreak(part);
    ASSERT_NE(viewBreak, nullptr);

    for (TechDraw::DrawViewPart* view :
         {static_cast<TechDraw::DrawViewPart*>(
              document()->addObject<TechDraw::DrawViewSection>("Section")),
          static_cast<TechDraw::DrawViewPart*>(
              document()->addObject<TechDraw::DrawViewDetail>("Detail")),
          static_cast<TechDraw::DrawViewPart*>(
              document()->addObject<TechDraw::DrawBrokenView>("BrokenView"))}) {
        EXPECT_FALSE(view->supportsViewBreaks()) << view->getNameInDocument();
        const auto objectCount = document()->countObjects();
        EXPECT_EQ(addBreak(view), nullptr) << view->getNameInDocument();
        EXPECT_EQ(document()->countObjects(), objectCount);

        view->Breaks.setValues({viewBreak});
        EXPECT_EQ(view->getBreakCount(), 0U) << view->getNameInDocument();
        EXPECT_EQ(view->getBreakObject(0), nullptr);
    }
}
