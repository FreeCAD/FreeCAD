// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <limits>
#include <sstream>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepTools_ShapeSet.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <gp_Ax3.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include "PartTestHelpers.h"
#include <Mod/Part/App/TopoShape.h>
#include "src/App/InitApplication.h"


class TopoShapeTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        Base::Interpreter().runString("import Part");
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _hasher = Base::Reference<App::StringHasher>(new App::StringHasher);
        ASSERT_EQ(_hasher.getRefCount(), 1);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }


private:
    std::string _docName;
    Data::ElementIDRefs _sid;
    App::StringHasherRef _hasher;
};

// clang-format off
TEST_F(TopoShapeTest, TestElementTypeFace1)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Face1"),
              std::make_pair(std::string("Face"), 1UL));
}

TEST_F(TopoShapeTest, TestElementTypeEdge12)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Edge12"),
              std::make_pair(std::string("Edge"), 12UL));
}

TEST_F(TopoShapeTest, TestElementTypeVertex3)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Vertex3"),
              std::make_pair(std::string("Vertex"), 3UL));
}

TEST_F(TopoShapeTest, TestElementTypeFacer)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Facer"),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestElementTypeVertex)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Vertex"),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestElementTypeEmpty)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex(""),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestElementTypeNull)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex(nullptr),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestElementTypeWithHash)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex(";#7:1;:G0;XTR;:H11a6:8,F.Face3"),
              std::make_pair(std::string("Face"), 3UL));
}

TEST_F(TopoShapeTest, TestElementTypeWithSubelements)
{
    EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex("Part.Body.Pad.Face3"),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestElementTypeNonMatching)
{
    for (std::array elements = {"Face0", "Face01", "XFace3", "Face3extra"};
         const auto& element : elements) {
        EXPECT_EQ(Part::TopoShape::getElementTypeAndIndex(element),
                  std::make_pair(std::string(), 0UL));
    }
}

TEST_F(TopoShapeTest, TestTypeFace1)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex("Face1"),
              std::make_pair(std::string("Face"), 1UL));
}

TEST_F(TopoShapeTest, TestTypeEdge12)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex("Edge12"),
              std::make_pair(std::string("Edge"), 12UL));
}

TEST_F(TopoShapeTest, TestTypeVertex3)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex("Vertex3"),
              std::make_pair(std::string("Vertex"), 3UL));
}

TEST_F(TopoShapeTest, TestTypeFacer)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex("Facer"),
              std::make_pair(std::string("Facer"), 0UL));
}

TEST_F(TopoShapeTest, TestTypeVertex)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex("Vertex"),
              std::make_pair(std::string("Vertex"), 0UL));
}

TEST_F(TopoShapeTest, TestTypeEmpty)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex(""),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestTypeNull)
{
    EXPECT_EQ(Part::TopoShape::getTypeAndIndex(nullptr),
              std::make_pair(std::string(), 0UL));
}

TEST_F(TopoShapeTest, TestGetSubshape)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoTopoShapeCubes();
    // Act
    auto face = cube1.getSubShape("Face2");
    auto vertex = cube2.getSubShape(TopAbs_VERTEX,2);
    auto silentFail = cube1.getSubShape("NotThere", true);
    // Assert
    EXPECT_EQ(face.ShapeType(), TopAbs_FACE);
    EXPECT_EQ(vertex.ShapeType(), TopAbs_VERTEX);
    EXPECT_TRUE(silentFail.IsNull());
    EXPECT_THROW(cube1.getSubShape("Face7"), Base::IndexError);          // Out of range
    EXPECT_THROW(cube1.getSubShape("WOOHOO", false), Base::ValueError);  // Invalid
}

// clang-format on

namespace
{
// A face on a cylinder trimmed at +-Precision::Infinite(), saved as a 2e100 bound
TopoDS_Shape makeInfiniteFace()
{
    Handle(Geom_CylindricalSurface) cylinder = new Geom_CylindricalSurface(gp_Ax3(), 3.0);
    Handle(Geom_RectangularTrimmedSurface) surface = new Geom_RectangularTrimmedSurface(
        cylinder,
        0.5,
        2.0,
        -Precision::Infinite(),
        Precision::Infinite()
    );
    return BRepBuilderAPI_MakeFace(surface, Precision::Confusion()).Face();
}

// The fixed notation Base::ZipWriter uses
void setFixedNotation(std::ostream& out)
{
    out.precision(std::numeric_limits<double>::digits10 + 1);
    out.setf(std::ios::fixed, std::ios::floatfield);
}

std::size_t longestToken(const std::string& text)
{
    std::istringstream in(text);
    std::size_t longest = 0;
    for (std::string token; in >> token;) {
        longest = std::max(longest, token.size());
    }
    return longest;
}

double upperVBound(const Part::TopoShape& shape)
{
    double u1 {}, u2 {}, v1 {}, v2 {};
    BRep_Tool::Surface(TopoDS::Face(shape.getShape()))->Bounds(u1, u2, v1, v2);
    return v2;
}
}  // namespace

TEST_F(TopoShapeTest, TestImportBrepWithLongNumbers)
{
    // Arrange: write the face the way older versions saved it
    TopoDS_Shape face = makeInfiniteFace();
    std::stringstream stream;
    setFixedNotation(stream);
    BRepTools_ShapeSet shapeSet(false);
    shapeSet.SetFormatNb(1);
    shapeSet.Add(face);
    shapeSet.Write(stream);
    shapeSet.Write(face, stream);
    ASSERT_GT(longestToken(stream.str()), 100U);
    Part::TopoShape shape;
    // Act
    shape.importBrep(stream);
    // Assert
    ASSERT_FALSE(shape.isNull());
    EXPECT_DOUBLE_EQ(upperVBound(shape), Precision::Infinite());
}

TEST_F(TopoShapeTest, TestExportBrepWithoutLongNumbers)
{
    // Arrange
    std::stringstream stream;
    setFixedNotation(stream);
    Part::TopoShape shape;
    // Act
    Part::TopoShape(makeInfiniteFace()).exportBrep(stream);
    shape.importBrep(stream);
    // Assert
    EXPECT_LE(longestToken(stream.str()), 31U);
    EXPECT_EQ(stream.flags() & std::ios::floatfield, std::ios::fixed);
    ASSERT_FALSE(shape.isNull());
    EXPECT_DOUBLE_EQ(upperVBound(shape), Precision::Infinite());
}

TEST_F(TopoShapeTest, TestImportBrepBlank)
{
    // Arrange
    std::istringstream stream(" \n");
    Part::TopoShape shape(makeInfiniteFace());
    // Act
    EXPECT_NO_THROW(shape.importBrep(stream));
    // Assert
    EXPECT_TRUE(shape.isNull());
}

TEST_F(TopoShapeTest, TestImportBrepBroken)
{
    // Arrange
    std::istringstream stream("CASCADE Topology V1, (c) Matra-Datavision\nLocations 1\n1\nx\n");
    Part::TopoShape shape;
    // Act and Assert
    EXPECT_THROW(shape.importBrep(stream), Base::CADKernelError);
}
