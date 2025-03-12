// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "PartTestHelpers.h"

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Mod/Part/App/Attacher.h>

using namespace Part;
using namespace Attacher;
using namespace PartTestHelpers;

/*
 * Testing note:  It looks like there are about 45 different attachment modes, and these tests
 * mostly only look at some of them - to prove that adding elementMap code doesn't break anything.
 * While a trivial bounding box test is used to ensure no hard crashes in any of the modes, any
 * mode that requires additional shapes beyond a couple of boxes would need a more comprehensive
 * test.
 */

class AttacherTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _boxes[1]->MapReversed.setValue(false);
        _boxes[1]->AttachmentSupport.setValue(_boxes[0]);
        _boxes[1]->MapPathParameter.setValue(0.0);
        _boxes[1]->MapMode.setValue("ObjectXY");  // There are lots of attachment modes!
        _boxes[1]->recomputeFeature();
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* getDocument() const
    {
        return _doc;
    }

protected:
};

TEST_F(AttacherTest, TestSetReferences)
{
    auto& attacher = _boxes[1]->attacher();
    EXPECT_EQ(attacher.getRefObjects().size(), 1);
}

TEST_F(AttacherTest, TestSuggestMapModes)
{
    auto& attacher = _boxes[1]->attacher();
    SuggestResult result;
    attacher.suggestMapModes(result);
    EXPECT_EQ(result.allApplicableModes.size(), 4);
    EXPECT_EQ(result.allApplicableModes[0], mmObjectXY);
    EXPECT_EQ(result.allApplicableModes[1], mmObjectXZ);
    EXPECT_EQ(result.allApplicableModes[2], mmObjectYZ);
    EXPECT_EQ(result.allApplicableModes[3], mmInertialCS);
}

TEST_F(AttacherTest, TestGetShapeType)
{
    auto& attacher = _boxes[1]->attacher();
    auto subObjects = _boxes[1]->getSubObjects();
    auto shapeType = attacher.getShapeType(_boxes[1], "Vertex2");
    EXPECT_EQ(shapeType, TopAbs_COMPSOLID);
}

TEST_F(AttacherTest, TestGetInertialPropsOfShape)
{
    auto& attacher = _boxes[1]->attacher();
    std::vector<const TopoShape*> result;
    auto faces = _boxes[1]->Shape.getShape().getSubTopoShapes(TopAbs_FACE);
    result.emplace_back(&faces[0]);
    auto shapeType = attacher.getInertialPropsOfShape(result);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(shapeType.Mass(), 6.0);
}

TEST_F(AttacherTest, TestGetRefObjects)
{
    auto& attacher = _boxes[1]->attacher();
    auto docObjects = attacher.getRefObjects();
    EXPECT_EQ(docObjects.size(), 1);
    EXPECT_STREQ(docObjects.front()->getNameInDocument(), "Part__Box");
}

TEST_F(AttacherTest, TestCalculateAttachedPlacement)
{
    auto& attacher = _boxes[1]->attacher();
    const Base::Placement orig;
    auto placement = attacher.calculateAttachedPlacement(orig);
    EXPECT_EQ(orig.getPosition().x, 0);
    EXPECT_EQ(orig.getPosition().y, 0);
    EXPECT_EQ(orig.getPosition().z, 0);
    EXPECT_EQ(placement.getPosition().x, 0);
    EXPECT_EQ(placement.getPosition().y, 0);
    EXPECT_EQ(placement.getPosition().z, 0);
}

TEST_F(AttacherTest, TestAllStringModesValid)
{
    // Arrange
    const char* modes[] = {
        "Deactivated",
        "Translate",
        "ObjectXY",
        "ObjectXZ",
        "ObjectYZ",
        "FlatFace",
        "TangentPlane",
        "NormalToEdge",
        "FrenetNB",
        "FrenetTN",
        "FrenetTB",
        "Concentric",
        "SectionOfRevolution",
        "ThreePointsPlane",
        "ThreePointsNormal",
        "Folding",

        "ObjectX",
        "ObjectY",
        "ObjectZ",
        "AxisOfCurvature",
        "Directrix1",
        "Directrix2",
        "Asymptote1",
        "Asymptote2",
        "Tangent",
        "Normal",
        "Binormal",
        "TangentU",
        "TangentV",
        "TwoPointLine",
        "IntersectionLine",
        "ProximityLine",

        "ObjectOrigin",
        "Focus1",
        "Focus2",
        "OnEdge",
        "CenterOfCurvature",
        "CenterOfMass",
        "IntersectionPoint",
        "Vertex",
        "ProximityPoint1",
        "ProximityPoint2",

        "AxisOfInertia1",
        "AxisOfInertia2",
        "AxisOfInertia3",

        "InertialCS",

        "FaceNormal",

        "OZX",
        "OZY",
        "OXY",
        "OXZ",
        "OYZ",
        "OYX",

        "ParallelPlane",
    };
    int index = 0;
    for (auto mode : modes) {
        _boxes[1]->MapMode.setValue(mode);  // There are lots of attachment modes!
        _boxes[1]->recomputeFeature();
        EXPECT_STREQ(_boxes[1]->MapMode.getValueAsString(), mode);
        EXPECT_EQ(_boxes[1]->MapMode.getValue(), index);
        index++;
    }
}

TEST_F(AttacherTest, TestAllModesBoundaries)
{
    _boxes[1]->MapMode.setValue(mmTranslate);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 1, 2, 3)));
    _boxes[1]->MapMode.setValue(mmObjectXY);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 1, 2, 3)));
    _boxes[1]->MapMode.setValue(mmObjectXZ);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, -3, 0, 1, 0, 2)));
    _boxes[1]->MapMode.setValue(mmObjectYZ);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mmParallelPlane);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmFlatFace);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmTangentPlane);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Normal);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mmFrenetNB);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmFrenetTN);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmFrenetTB);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmConcentric);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmRevolutionSection);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmThreePointsNormal);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmThreePointsPlane);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mmFolding);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mm1AxisX);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1AxisY);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1AxisZ);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1AxisCurv);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Directrix1);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Directrix2);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Asymptote1);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Asymptote2);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Tangent);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1TangentU);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1TangentV);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1TwoPoints);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Intersection);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Proximity);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mm0Origin);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0Focus1);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0Focus2);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0OnEdge);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0CenterOfCurvature);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0CenterOfMass);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1Intersection);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0Vertex);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0ProximityPoint1);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm0ProximityPoint2);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mm1AxisInertia1);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1AxisInertia2);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));
    _boxes[1]->MapMode.setValue(mm1AxisInertia3);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0, 0, 0, 3, 1, 2)));

    _boxes[1]->MapMode.setValue(mmInertialCS);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));

    _boxes[1]->MapMode.setValue(mm1FaceNormal);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));

    _boxes[1]->MapMode.setValue(mmOZX);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
    _boxes[1]->MapMode.setValue(mmOZY);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
    _boxes[1]->MapMode.setValue(mmOXY);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
    _boxes[1]->MapMode.setValue(mmOXZ);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
    _boxes[1]->MapMode.setValue(mmOYZ);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
    _boxes[1]->MapMode.setValue(mmOYX);
    _boxes[1]->recomputeFeature();
    EXPECT_TRUE(
        boxesMatch(_boxes[1]->Shape.getBoundingBox(), Base::BoundBox3d(0.5, 1, 1.5, 3.5, 2, 3.5)));
}
