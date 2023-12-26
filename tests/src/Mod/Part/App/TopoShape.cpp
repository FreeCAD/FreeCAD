// SPDX-License-Identifier: LGPL-2.1-or-later

#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS.hxx>
#include "App/ComplexGeoData.h"
#include "gtest/gtest.h"
#include "Mod/Part/App/TopoShape.h"
#include "Base/BoundBox.h"

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

        gp_Pnt lowerLeftCornerOfBox(1.0, 2.0, 3.0);
        BRepPrimAPI_MakeBox boxMaker(lowerLeftCornerOfBox, 10, 11, 12);
        auto box = boxMaker.Shape();

        _topoShape = Part::TopoShape(box);
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

TEST_F(TopoShapeTest, impliedNamingOfSelf)
{
    // Arrange
    auto silent = false;

    // Act
    auto generalName = _topoShape.shapeName(silent);
    auto typeAndIndex = _topoShape.shapeTypeAndIndex("Solid");

    // Assert
    EXPECT_EQ(generalName, "Solid");
    EXPECT_EQ(typeAndIndex.first, TopAbs_SOLID);
    EXPECT_EQ(typeAndIndex.second, 0);
}

TEST_F(TopoShapeTest, pointsAndFaces)
{
    // Arrange
    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> faces;
    double accuracy(0.0);

    // Act
    _topoShape.getFaces(points, faces, accuracy); // box at 1,2,3 with dimensions 10,11,12

    // Assert
    auto thirdPoint = points[2];
    auto thirdFace = faces[2];
    EXPECT_EQ(points.size(), 8);
    EXPECT_EQ(thirdPoint.x, 1);
    EXPECT_EQ(thirdPoint.y, 11+2);
    EXPECT_EQ(thirdPoint.z, 3);
    EXPECT_EQ(faces.size(), 12);
    EXPECT_EQ(thirdFace.I1, 4);
    EXPECT_EQ(thirdFace.I2, 5);
    EXPECT_EQ(thirdFace.I3, 6);
}

TEST_F(TopoShapeTest, facesAndFacets)
{
    // Arrange
    std::vector<Base::Vector3d> points;
    std::vector<Data::ComplexGeoData::Facet> facets;
    double accuracy(0.0);
    std::string face = "Face"; // force to string to enable proper std:find matching
    std::string edge = "Edge";
    std::string vertex = "Vertex";
    _topoShape.getFaces(points, facets, accuracy); // box at 1,2,3 with dimensions 10,11,12

    // Act
    auto elementTypes = _topoShape.getElementTypes();
    auto faceSubShapes = _topoShape.getSubShapes(TopAbs_FACE);
    auto thirdFaceByType = faceSubShapes[2];

    // Assert
    EXPECT_TRUE(
        std::find(elementTypes.begin(), elementTypes.end(), face) != elementTypes.end());
    EXPECT_TRUE(
        std::find(elementTypes.begin(), elementTypes.end(), edge) != elementTypes.end());
    EXPECT_TRUE(
        std::find(elementTypes.begin(), elementTypes.end(), vertex) != elementTypes.end());
    EXPECT_EQ(faceSubShapes.size(), 6); // 6 faces (both in/out facets included)
    EXPECT_EQ(facets.size(), 6*2);  // two triangular facets per rectangular face
    EXPECT_EQ(points[facets[0].I1].x, 1);  // first facet of face0; the first point of the facet is origin (1, 2, 3)
    EXPECT_EQ(points[facets[0].I1].y, 2);
    EXPECT_EQ(points[facets[0].I1].z, 3);
}

TEST_F(TopoShapeTest, getCharacteristics)
{
    // Arrange
    Base::Vector3d cog;
    // NOTE: _topoShape is a box with lower-left corner at (1, 2, 3) of size (10, 11, 12), thus
    double expectedCogX = 1 + (10 / 2.0); // 6
    double expectedCogY = 2 + (11 / 2.0); // 7.5
    double expectedCogZ = 3 + (12 / 2.0); // 9

    // Act
    auto boundingBox = _topoShape.getBoundBox();
    _topoShape.getCenterOfGravity(cog);

    // Assert
    EXPECT_EQ(boundingBox.LengthX(), 10);
    EXPECT_EQ(boundingBox.LengthY(), 11);
    EXPECT_EQ(boundingBox.LengthZ(), 12);
    EXPECT_EQ(cog.x, expectedCogX);
    EXPECT_EQ(cog.y, expectedCogY);
    EXPECT_EQ(cog.z, expectedCogZ);
}
