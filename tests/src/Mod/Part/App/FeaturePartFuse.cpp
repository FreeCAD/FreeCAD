// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Mod/Part/App/FeaturePartFuse.h"
#include <src/App/InitApplication.h>
#include "Mod/Part/App/FeatureCompound.h"

#include "PartTestHelpers.h"


class FeaturePartFuseTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _fuse = dynamic_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
        _multiFuse = dynamic_cast<Part::MultiFuse*>(_doc->addObject("Part::MultiFuse"));
    }

    void TearDown() override
    {}

    Part::Fuse* _fuse = nullptr;            // NOLINT Can't be private in a test framework
    Part::MultiFuse* _multiFuse = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeaturePartFuseTest, testIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[1]);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 9.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 3.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testCompound)
{
    // Arrange
    Part::Compound* _compound = nullptr;
    _compound = dynamic_cast<Part::Compound*>(_doc->addObject("Part::Compound"));
    _compound->Links.setValues({_boxes[0], _boxes[1]});
    _multiFuse->Shapes.setValues({_compound});

    // Act
    _compound->execute();
    _multiFuse->execute();
    Part::TopoShape ts = _multiFuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 9.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 3.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}
TEST_F(FeaturePartFuseTest, testRecursiveCompound)
{
    // Arrange
    Part::Compound* _compound[3] = {nullptr};
    int t;
    for (t = 0; t < 3; t++) {
        _compound[t] = dynamic_cast<Part::Compound*>(_doc->addObject("Part::Compound"));
    }
    _compound[0]->Links.setValues({_boxes[0], _boxes[1]});
    _compound[1]->Links.setValues({_compound[0]});
    _compound[2]->Links.setValues({_compound[1]});
    _multiFuse->Shapes.setValues({_compound[2]});

    // Act
    for (t = 0; t < 3; t++) {
        _compound[t]->execute();
    }
    _multiFuse->execute();
    Part::TopoShape ts = _multiFuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 9.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 3.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testNonIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[2]);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 5.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testTouching)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[3]);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_DOUBLE_EQ(volume, 12.0);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 4.0);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testAlmostTouching)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[4]);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(volume, 12.0);  // Use FLOAT to limit precision to 1E07 rather than 1E15
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_FLOAT_EQ(bb.MaxY, 4.0);  // Use FLOAT to limit precision to 1E07 rather than 1E15
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testBarelyIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[5]);  // NOLINT magic number

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = PartTestHelpers::getVolume(ts.getShape());
    double target = 12 - PartTestHelpers::minimalDistance * 3;  // NOLINT 3 dimensions in a Volume
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    // Using FLOAT, not DOUBLE here so test library comparison is of reasonable precision 1e07
    // rather than 1e15 See
    // https://google.github.io/googletest/reference/assertions.html#floating-point
    EXPECT_FLOAT_EQ(volume, target);
    // double check using bounds:
    EXPECT_DOUBLE_EQ(bb.MinX, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinY, 0.0);
    EXPECT_DOUBLE_EQ(bb.MinZ, 0.0);
    EXPECT_DOUBLE_EQ(bb.MaxX, 1.0);
    EXPECT_DOUBLE_EQ(bb.MaxY, 4.0 - PartTestHelpers::minimalDistance);
    EXPECT_DOUBLE_EQ(bb.MaxZ, 3.0);
}

TEST_F(FeaturePartFuseTest, testMustExecute)
{
    // Assert initially we don't need to execute
    EXPECT_FALSE(_fuse->mustExecute());
    // Act to change one property
    _fuse->Base.setValue(_boxes[0]);
    // Assert we still can't execute
    EXPECT_FALSE(_fuse->mustExecute());
    // Act to complete the properties we need
    _fuse->Tool.setValue(_boxes[1]);
    // Assert that we now must execute
    EXPECT_TRUE(_fuse->mustExecute());
    // Act to execute
    _doc->recompute();
    // Assert we don't need to execute anymore
    EXPECT_FALSE(_fuse->mustExecute());
}

TEST_F(FeaturePartFuseTest, testGetProviderName)
{
    // Act
    _fuse->execute();
    const char* name = _fuse->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

TEST_F(FeaturePartFuseTest, testRefine)
{
    // Arrange
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[1]);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    std::vector<Part::TopoShape> subs =
        ts.getSubTopoShapes(TopAbs_FACE);  // TopAbs_WIRE alternate approach
    // Assert two boxes, plus redundant faces at the joint.
    EXPECT_EQ(subs.size(), 14);
    // 14 Faces
    // 28 Edges
    // 16 Vertices
    // -----------
    // 58 Elements
    EXPECT_EQ(_fuse->Shape.getShape().getElementMapSize(), 58);
    // Act
    _fuse->Refine.setValue(true);
    _fuse->execute();
    ts = _fuse->Shape.getValue();
    subs = ts.getSubTopoShapes(TopAbs_FACE);
    // Assert we now just have one big box
    EXPECT_EQ(subs.size(), 6);
    // 6 Faces
    // 12 Edges
    // 8 Vertices
    // -----------
    // 58 Elements
    EXPECT_EQ(_fuse->Shape.getShape().getElementMapSize(), 26);
}

// See FeaturePartCommon.cpp for a history test.  It would be exactly the same and redundant here.
