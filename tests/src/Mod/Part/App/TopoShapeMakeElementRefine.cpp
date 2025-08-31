// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

class FeaturePartMakeElementRefineTest: public ::testing::Test,
                                        public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
    }

    void TearDown() override
    {}
};

TEST_F(FeaturePartMakeElementRefineTest, makeElementRefineBoxes)
{
    // Arrange
    auto _doc = App::GetApplication().getActiveDocument();
    auto _fuse = _doc->addObject<Part::Fuse>();
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[3]);
    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getShape();
    Part::TopoShape refined = ts.makeElementRefine();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double refinedVolume = PartTestHelpers::getVolume(refined.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();
    // Assert
    EXPECT_TRUE(bb.IsValid());
    EXPECT_DOUBLE_EQ(volume, 12.0);
    EXPECT_DOUBLE_EQ(refinedVolume, 12.0);            // Refine shouldn't change the volume
    EXPECT_EQ(ts.countSubElements("Face"), 10);       // Two boxes touching each loose one face
    EXPECT_EQ(ts.countSubElements("Edge"), 20);       // Two boxes touching loose 4 edges
    EXPECT_EQ(refined.countSubElements("Face"), 6);   // After refining it is one box
    EXPECT_EQ(refined.countSubElements("Edge"), 12);  // 12 edges in a box
    // Make sure that the number of elements in the elementMaps is correct.
    EXPECT_EQ(ts.getElementMapSize(), 42);
    EXPECT_EQ(refined.getElementMapSize(), 26);
    // TODO: Refine doesn't work on compounds, so we're going to need a binary operation or the
    // like, and those don't exist yet.  Once they do, this test can be expanded
}
