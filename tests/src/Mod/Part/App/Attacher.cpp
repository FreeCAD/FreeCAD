// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "PartTestHelpers.h"

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Mod/Part/App/Attacher.h>

using namespace Part;
using namespace Attacher;
using namespace PartTestHelpers;

/*
 * Testing note:  It looks like there are about 45 different attachment modes, and these tests all
 * only look at one of them - to prove that adding elementMap code doesn't break anything.  A
 * comprehensive test of the Attacher would definitely want to try many more code paths.
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
    std::vector<const TopoDS_Shape*> result;
    auto faces = _boxes[1]->Shape.getShape().getSubShapes(TopAbs_FACE);
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
