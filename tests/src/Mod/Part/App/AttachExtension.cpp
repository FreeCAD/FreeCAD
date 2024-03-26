// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Mod/Part/App/PrimitiveFeature.h>


class AttachExtensionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
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

    App::Document* getDocument() const
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc = nullptr;
};

TEST_F(AttachExtensionTest, testPlanePlane)
{
    auto plane1 = dynamic_cast<Part::Plane*>(getDocument()->addObject("Part::Plane", "Plane1"));
    auto plane2 = dynamic_cast<Part::Plane*>(getDocument()->addObject("Part::Plane", "Plane2"));

    ASSERT_TRUE(plane1);
    ASSERT_TRUE(plane2);

    getDocument()->recompute();

    plane2->MapReversed.setValue(false);
    plane2->AttachmentSupport.setValue(plane1);
    plane2->MapPathParameter.setValue(0.0);
    plane2->MapMode.setValue("FlatFace");

    getDocument()->recompute();
    EXPECT_TRUE(true);
}
