// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>

using namespace App;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class LinkTest: public ::testing::Test
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

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    std::string _docName {};
    App::Document* _doc {};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

TEST_F(LinkTest, getArrayIndexSimpleInteger)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("42"), 42);
}

TEST_F(LinkTest, getArrayIndexZero)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("0"), 0);
}

TEST_F(LinkTest, getArrayIndexWithDotSuffix)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("7.Face1"), 7);
}

TEST_F(LinkTest, getArrayIndexMultiDigitWithDot)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("123.Edge2"), 123);
}

TEST_F(LinkTest, getArrayIndexTrailingDotOnly)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("99."), 99);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForNull)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(nullptr), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForEmptyString)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(""), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForLeadingDot)
{
    // A dot at position 0 means the segment before the dot is empty
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(".Something"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForAlpha)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("abc"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForMixedAlphaDigit)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("12abc"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForAlphaThenDigit)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("abc12"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForMappedElement)
{
    // Mapped element names start with ";" (ELEMENT_MAP_PREFIX)
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(";Edge1"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForMappedElementWithDigits)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(";42"), -1);
}

TEST_F(LinkTest, getArrayIndexPsubnamePointsPastDot)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex("42.Face1", &rest);
    EXPECT_EQ(idx, 42);
    ASSERT_NE(rest, nullptr);
    EXPECT_STREQ(rest, "Face1");
}

TEST_F(LinkTest, getArrayIndexPsubnameAtEndWhenNoDot)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex("7", &rest);
    EXPECT_EQ(idx, 7);
    ASSERT_NE(rest, nullptr);
    EXPECT_STREQ(rest, "");
}

TEST_F(LinkTest, getArrayIndexPsubnameEmptyAfterTrailingDot)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex("55.", &rest);
    EXPECT_EQ(idx, 55);
    ASSERT_NE(rest, nullptr);
    EXPECT_STREQ(rest, "");
}

TEST_F(LinkTest, getArrayIndexPsubnameWithNestedDots)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex("3.Sub.Face1", &rest);
    EXPECT_EQ(idx, 3);
    ASSERT_NE(rest, nullptr);
    EXPECT_STREQ(rest, "Sub.Face1");
}

TEST_F(LinkTest, getArrayIndexPsubnameUntouchedOnFailure)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex("abc.xyz", &rest);
    EXPECT_EQ(idx, -1);
    // psubname should not have been written to on failure
    EXPECT_EQ(rest, nullptr);
}

TEST_F(LinkTest, getArrayIndexPsubnameUntouchedOnNull)
{
    const char* rest = nullptr;
    int idx = LinkBaseExtension::getArrayIndex(nullptr, &rest);
    EXPECT_EQ(idx, -1);
    EXPECT_EQ(rest, nullptr);
}

TEST_F(LinkTest, getArrayIndexLargeNumber)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("99999"), 99999);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForNegative)
{
    // Negative sign is not a digit
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("-1"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForSpace)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex(" 5"), -1);
}

TEST_F(LinkTest, getArrayIndexReturnsMinusOneForDigitWithSpace)
{
    EXPECT_EQ(LinkBaseExtension::getArrayIndex("5 "), -1);
}

// Helper function for the getScaleVector methods, which require a "real" link to work with.
static App::Link* addLink(App::Document* doc)
{
    auto* obj = doc->addObject("App::Link");
    EXPECT_NE(obj, nullptr);
    auto* link = dynamic_cast<App::Link*>(obj);
    EXPECT_NE(link, nullptr);
    return link;
}

// -- getScaleVector --

TEST_F(LinkTest, getScaleVectorDefaultIsUnity)
{
    auto* link = addLink(_doc);
    Base::Vector3d sv = link->getScaleVector();
    EXPECT_DOUBLE_EQ(sv.x, 1.0);
    EXPECT_DOUBLE_EQ(sv.y, 1.0);
    EXPECT_DOUBLE_EQ(sv.z, 1.0);
}

TEST_F(LinkTest, getScaleVectorReflectsUniformScale)
{
    auto* link = addLink(_doc);
    link->Scale.setValue(2.5);
    // ScaleVector property exists on Link, so it takes priority. The Scale->ScaleVector sync
    // happens in extensionOnChanged, so set ScaleVector explicitly to test the accessor.
    link->ScaleVector.setValue(2.5, 2.5, 2.5);
    Base::Vector3d sv = link->getScaleVector();
    EXPECT_DOUBLE_EQ(sv.x, 2.5);
    EXPECT_DOUBLE_EQ(sv.y, 2.5);
    EXPECT_DOUBLE_EQ(sv.z, 2.5);
}

TEST_F(LinkTest, getScaleVectorReflectsNonUniformScale)
{
    auto* link = addLink(_doc);
    link->ScaleVector.setValue(1.0, 2.0, 3.0);
    Base::Vector3d sv = link->getScaleVector();
    EXPECT_DOUBLE_EQ(sv.x, 1.0);
    EXPECT_DOUBLE_EQ(sv.y, 2.0);
    EXPECT_DOUBLE_EQ(sv.z, 3.0);
}

TEST_F(LinkTest, getTransformNoTransformDefaultIsIdentity)
{
    auto* link = addLink(_doc);
    Base::Matrix4D mat = link->getTransform(false);
    EXPECT_TRUE(mat.isUnity());
}

TEST_F(LinkTest, getTransformWithTransformDefaultIsIdentity)
{
    auto* link = addLink(_doc);
    Base::Matrix4D mat = link->getTransform(true);
    EXPECT_TRUE(mat.isUnity());
}

TEST_F(LinkTest, getTransformNoTransformAppliesScaleOnly)
{
    auto* link = addLink(_doc);
    link->ScaleVector.setValue(2.0, 3.0, 4.0);

    Base::Matrix4D mat = link->getTransform(false);

    // Diagonal should contain the scale factors
    EXPECT_DOUBLE_EQ(mat[0][0], 2.0);
    EXPECT_DOUBLE_EQ(mat[1][1], 3.0);
    EXPECT_DOUBLE_EQ(mat[2][2], 4.0);
    // Translation should be zero
    EXPECT_DOUBLE_EQ(mat[0][3], 0.0);
    EXPECT_DOUBLE_EQ(mat[1][3], 0.0);
    EXPECT_DOUBLE_EQ(mat[2][3], 0.0);
}

TEST_F(LinkTest, getTransformNoTransformIgnoresPlacement)
{
    auto* link = addLink(_doc);
    Base::Placement pl(Base::Vector3d(10, 20, 30), Base::Rotation());
    link->LinkPlacement.setValue(pl);

    Base::Matrix4D mat = link->getTransform(false);

    // With transform=false, placement is ignored; only scale matters. Default scale is 1, so
    // the matrix should be identity.
    EXPECT_TRUE(mat.isUnity());
}

TEST_F(LinkTest, getTransformWithTransformIncludesLinkPlacement)
{
    auto* link = addLink(_doc);
    Base::Placement pl(Base::Vector3d(10, 20, 30), Base::Rotation());
    link->LinkPlacement.setValue(pl);

    Base::Matrix4D mat = link->getTransform(true);

    // Translation column should reflect the placement
    EXPECT_DOUBLE_EQ(mat[0][3], 10.0);
    EXPECT_DOUBLE_EQ(mat[1][3], 20.0);
    EXPECT_DOUBLE_EQ(mat[2][3], 30.0);
}

TEST_F(LinkTest, getTransformWithTransformAndScale)
{
    auto* link = addLink(_doc);
    Base::Placement pl(Base::Vector3d(5, 0, 0), Base::Rotation());
    link->LinkPlacement.setValue(pl);
    link->ScaleVector.setValue(2.0, 2.0, 2.0);

    Base::Matrix4D mat = link->getTransform(true);

    // The result is placement * scale. With no rotation, the diagonal carries the scale and
    // the last column carries the translation.
    EXPECT_DOUBLE_EQ(mat[0][0], 2.0);
    EXPECT_DOUBLE_EQ(mat[1][1], 2.0);
    EXPECT_DOUBLE_EQ(mat[2][2], 2.0);
    EXPECT_DOUBLE_EQ(mat[0][3], 5.0);
    EXPECT_DOUBLE_EQ(mat[1][3], 0.0);
    EXPECT_DOUBLE_EQ(mat[2][3], 0.0);
}

TEST_F(LinkTest, getTransformUsesLinkPlacementWhenPropertyExists)
{
    // On App::Link, both LinkPlacement and Placement properties exist. getTransform() checks
    // for the *existence* of LinkPlacement first (not its value), so it always reads from
    // LinkPlacement when both properties are defined.
    auto* link = addLink(_doc);
    Base::Placement pl(Base::Vector3d(1, 2, 3), Base::Rotation());
    link->LinkPlacement.setValue(pl);

    Base::Matrix4D mat = link->getTransform(true);

    EXPECT_DOUBLE_EQ(mat[0][3], 1.0);
    EXPECT_DOUBLE_EQ(mat[1][3], 2.0);
    EXPECT_DOUBLE_EQ(mat[2][3], 3.0);
}

TEST_F(LinkTest, linkPlacementAndPlacementStayInSync)
{
    // LinkPlacement and Placement are aliased: changing one updates the other.
    auto* link = addLink(_doc);
    Base::Placement pl(Base::Vector3d(10, 20, 30), Base::Rotation());
    link->LinkPlacement.setValue(pl);

    // Placement should have been synced to the same value
    auto synced = link->Placement.getValue();
    EXPECT_DOUBLE_EQ(synced.getPosition().x, 10.0);
    EXPECT_DOUBLE_EQ(synced.getPosition().y, 20.0);
    EXPECT_DOUBLE_EQ(synced.getPosition().z, 30.0);

    // Setting Placement should sync back to LinkPlacement
    Base::Placement pl2(Base::Vector3d(7, 8, 9), Base::Rotation());
    link->Placement.setValue(pl2);

    auto synced2 = link->LinkPlacement.getValue();
    EXPECT_DOUBLE_EQ(synced2.getPosition().x, 7.0);
    EXPECT_DOUBLE_EQ(synced2.getPosition().y, 8.0);
    EXPECT_DOUBLE_EQ(synced2.getPosition().z, 9.0);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
