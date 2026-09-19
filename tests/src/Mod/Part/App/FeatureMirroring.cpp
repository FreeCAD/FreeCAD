// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>
#include <Mod/Part/App/FeatureMirroring.h>

#include "PartTestHelpers.h"

using namespace PartTestHelpers;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
class FeatureMirroringTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _mirror = _doc->addObject<Part::Mirroring>();
        _mirror->Source.setValue(_boxes[0]);
        _mirror->Base.setValue(1, 0, 0);
        _mirror->execute();
    }

    void TearDown() override
    {}

    Part::Mirroring* _mirror = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeatureMirroringTest, testXMirror)
{
    // Arrange
    Base::BoundBox3d bb = _mirror->Shape.getShape().getBoundBox();
    // Assert size and position
    EXPECT_EQ(getVolume(_mirror->Shape.getShape().getShape()), 6);
    // Mirrored it around X from 0,0,0 -> 1,2,3  to  0,0,-3 -> 1,2,0
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, -3, 1, 2, 0)));
    // Assert correct element Map
    EXPECT_TRUE(allElementsMatch(
        _mirror->Shape.getShape(),
        {
            "Edge10;:M;MIR;:H70c:7,E",  "Edge11;:M;MIR;:H70c:7,E",  "Edge12;:M;MIR;:H70c:7,E",
            "Edge1;:M;MIR;:H70c:7,E",   "Edge2;:M;MIR;:H70c:7,E",   "Edge3;:M;MIR;:H70c:7,E",
            "Edge4;:M;MIR;:H70c:7,E",   "Edge5;:M;MIR;:H70c:7,E",   "Edge6;:M;MIR;:H70c:7,E",
            "Edge7;:M;MIR;:H70c:7,E",   "Edge8;:M;MIR;:H70c:7,E",   "Edge9;:M;MIR;:H70c:7,E",
            "Face1;:M;MIR;:H70c:7,F",   "Face2;:M;MIR;:H70c:7,F",   "Face3;:M;MIR;:H70c:7,F",
            "Face4;:M;MIR;:H70c:7,F",   "Face5;:M;MIR;:H70c:7,F",   "Face6;:M;MIR;:H70c:7,F",
            "Vertex1;:M;MIR;:H70c:7,V", "Vertex2;:M;MIR;:H70c:7,V", "Vertex3;:M;MIR;:H70c:7,V",
            "Vertex4;:M;MIR;:H70c:7,V", "Vertex5;:M;MIR;:H70c:7,V", "Vertex6;:M;MIR;:H70c:7,V",
            "Vertex7;:M;MIR;:H70c:7,V", "Vertex8;:M;MIR;:H70c:7,V",
        }
    ));
}

TEST_F(FeatureMirroringTest, testYMirrorWithExistingElementMap)
{
    // Arrange
    Part::Fuse* _fuse = nullptr;  // NOLINT Can't be private in a test framework
    _fuse = _doc->addObject<Part::Fuse>();
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[1]);
    _fuse->Refine.setValue(false);
    // Act
    _fuse->execute();
    _mirror->Source.setValue(_fuse);
    _mirror->Base.setValue(0, 1, 0);  // Y Axis
    _mirror->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = getVolume(ts.getShape());
    Base::BoundBox3d bb = _mirror->Shape.getShape().getBoundBox();
    // Assert size and position
    EXPECT_EQ(getVolume(_mirror->Shape.getShape().getShape()), volume);
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, -3, 1, 3, 0)));
    // Semantic mirroring history on every mapped sub-element
    const auto elements = _mirror->Shape.getShape().getElementMap();
    ASSERT_GE(elements.size(), 54U);
    for (const auto& element : elements) {
        const std::string name = element.name.toString();
        EXPECT_NE(name.find(";:M;MIR;"), std::string::npos) << name;
    }
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
