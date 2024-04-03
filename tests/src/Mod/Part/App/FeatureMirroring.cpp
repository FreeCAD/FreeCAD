// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

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
        _mirror = dynamic_cast<Part::Mirroring*>(_doc->addObject("Part::Mirroring"));
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
#ifdef FC_USE_TNP_FIX
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
        }));
#else
    EXPECT_EQ(_mirror->Shape.getShape().getElementMapSize(), 0);
#endif
}

TEST_F(FeatureMirroringTest, testYMirrorWithExistingElementMap)
{
    // Arrange
    Part::Fuse* _fuse = nullptr;  // NOLINT Can't be private in a test framework
    _fuse = dynamic_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
    _fuse->Base.setValue(_boxes[0]);
    _fuse->Tool.setValue(_boxes[1]);
    // Act
    _fuse->execute();
    _mirror->Source.setValue(_fuse);
    _mirror->Base.setValue(0, 1, 0);  // Y Axis
    Part::TopoShape ts = _fuse->Shape.getValue();
    double volume = getVolume(ts.getShape());
    Base::BoundBox3d bb = _mirror->Shape.getShape().getBoundBox();
    // Assert size and position
    EXPECT_EQ(getVolume(_mirror->Shape.getShape().getShape()), volume);
    // Mirrored it around X from 0,0,0 -> 1,2,3  to  0,0,-3 -> 1,2,0
    EXPECT_TRUE(boxesMatch(bb, Base::BoundBox3d(0, 0, -3, 1, 3, 0)));
    // Assert correct element Map
#ifdef FC_USE_TNP_FIX
    EXPECT_TRUE(elementsMatch(_mirror->Shape.getShape(),
                              {
                                  "Edge10;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge10;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge11;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge11;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge12;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge12;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge1;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge1;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge2;:M2(Edge2;:Hfd7,E);FUS;:Hfd6:17,E;:M;MIR;:Hfdd:7,E",
                                  "Edge2;:M2;FUS;:Hfd7:8,E;:M;MIR;:Hfdd:7,E",
                                  "Edge2;:M;FUS;:Hfd6:7,E;:M;MIR;:Hfdd:7,E",
                                  "Edge3;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge3;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge4;:M2(Edge4;:Hfd7,E);FUS;:Hfd6:17,E;:M;MIR;:Hfdd:7,E",
                                  "Edge4;:M2;FUS;:Hfd7:8,E;:M;MIR;:Hfdd:7,E",
                                  "Edge4;:M;FUS;:Hfd6:7,E;:M;MIR;:Hfdd:7,E",
                                  "Edge5;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge5;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge6;:M2(Edge6;:Hfd7,E);FUS;:Hfd6:17,E;:M;MIR;:Hfdd:7,E",
                                  "Edge6;:M2;FUS;:Hfd7:8,E;:M;MIR;:Hfdd:7,E",
                                  "Edge6;:M;FUS;:Hfd6:7,E;:M;MIR;:Hfdd:7,E",
                                  "Edge7;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge7;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge8;:M2(Edge8;:Hfd7,E);FUS;:Hfd6:17,E;:M;MIR;:Hfdd:7,E",
                                  "Edge8;:M2;FUS;:Hfd7:8,E;:M;MIR;:Hfdd:7,E",
                                  "Edge8;:M;FUS;:Hfd6:7,E;:M;MIR;:Hfdd:7,E",
                                  "Edge9;FUS;:Hfd6:4,E;:M;MIR;:Hfdd:7,E",
                                  "Edge9;FUS;:Hfd7:4,E;:M;MIR;:Hfdd:7,E",
                                  "Face1;:M2(Face1;:Hfd7,F);FUS;:Hfd6:17,F;:M;MIR;:Hfdd:7,F",
                                  "Face1;:M2;FUS;:Hfd7:8,F;:M;MIR;:Hfdd:7,F",
                                  "Face1;:M;FUS;:Hfd6:7,F;:M;MIR;:Hfdd:7,F",
                                  "Face2;:M2(Face2;:Hfd7,F);FUS;:Hfd6:17,F;:M;MIR;:Hfdd:7,F",
                                  "Face2;:M2;FUS;:Hfd7:8,F;:M;MIR;:Hfdd:7,F",
                                  "Face2;:M;FUS;:Hfd6:7,F;:M;MIR;:Hfdd:7,F",
                                  "Face3;FUS;:Hfd6:4,F;:M;MIR;:Hfdd:7,F",
                                  "Face4;FUS;:Hfd7:4,F;:M;MIR;:Hfdd:7,F",
                                  "Face5;:M2(Face5;:Hfd7,F);FUS;:Hfd6:17,F;:M;MIR;:Hfdd:7,F",
                                  "Face5;:M2;FUS;:Hfd7:8,F;:M;MIR;:Hfdd:7,F",
                                  "Face5;:M;FUS;:Hfd6:7,F;:M;MIR;:Hfdd:7,F",
                                  "Face6;:M2(Face6;:Hfd7,F);FUS;:Hfd6:17,F;:M;MIR;:Hfdd:7,F",
                                  "Face6;:M2;FUS;:Hfd7:8,F;:M;MIR;:Hfdd:7,F",
                                  "Face6;:M;FUS;:Hfd6:7,F;:M;MIR;:Hfdd:7,F",
                                  "Vertex1;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex1;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex2;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex2;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex3;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex3;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex4;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex4;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex5;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex5;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex6;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex6;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex7;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex7;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex8;FUS;:Hfd6:4,V;:M;MIR;:Hfdd:7,V",
                                  "Vertex8;FUS;:Hfd7:4,V;:M;MIR;:Hfdd:7,V",
                              }));
#else
    EXPECT_EQ(_mirror->Shape.getShape().getElementMapSize(), 0);
#endif
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
