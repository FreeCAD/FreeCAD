// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "Mod/Part/App/TopoShape.h"

class TopoShapeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
    }

    void SetUp() override {
        _declaredLoc1234 = gp_Trsf();
        _declaredLoc1234.SetScale(gp_Pnt(1.0, 2.0, 3.0), 4.0);
        _testLocation1234 = TopLoc_Location(_declaredLoc1234);

        _topoShape = Part::TopoShape();
    }

    void TearDown() override {
        _topoShape = Part::TopoShape();
    }
public:
    Part::TopoShape _topoShape;
    TopLoc_Location _testLocation1234;
    gp_Trsf _declaredLoc1234;
};


TEST_F(TopoShapeTest, setShapeGetShape)
{
    // Arrange
    auto newShape = TopoDS_Shape();
    newShape.Location(_testLocation1234);

    // Act
    _topoShape.setShape(newShape);          // set
    auto shapeSeen = _topoShape.getShape(); // get

    // Assert
    EXPECT_EQ(shapeSeen.Location(), _testLocation1234);
    EXPECT_EQ(shapeSeen.Location().Transformation().ScaleFactor(), 4.0);
}

