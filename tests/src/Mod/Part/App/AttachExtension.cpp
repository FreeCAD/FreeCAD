// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <Base/Precision.h>
#include <Base/Tools.h>
#include <Mod/Part/App/Datums.h>
#include <Mod/Part/App/PrimitiveFeature.h>


class AttachExtensionTest: public ::testing::Test
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

    App::Document* getDocument() const
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc = nullptr;
};

TEST_F(AttachExtensionTest, testPlanePlane)
{
    auto plane1 = getDocument()->addObject<Part::Plane>("Plane1");
    auto plane2 = getDocument()->addObject<Part::Plane>("Plane2");

    ASSERT_TRUE(plane1);
    ASSERT_TRUE(plane2);

    getDocument()->recompute();

    plane2->MapReversed.setValue(false);
    plane2->AttachmentSupport.setValue(plane1);
    plane2->MapPathParameter.setValue(0.0);
    plane2->MapMode.setValue("FlatFace");

    getDocument()->recompute();
    EXPECT_TRUE(true);
}

TEST_F(AttachExtensionTest, testTranslateAttachmentOffsetKeepsRotation)
{
    auto part = getDocument()->addObject<App::Part>("Part");
    auto lcs = getDocument()->addObject<Part::LocalCoordinateSystem>("LCS");

    ASSERT_TRUE(part);
    ASSERT_TRUE(lcs);

    part->addObject(lcs);

    auto origin = part->getOrigin();
    ASSERT_TRUE(origin);
    auto originPoint = origin->getOrigin();
    ASSERT_TRUE(originPoint);

    const std::string originPointSubName = std::string(originPoint->getNameInDocument()) + ".";
    lcs->AttachmentSupport.setValue(origin, originPointSubName.c_str());
    lcs->MapMode.setValue("Translate");
    lcs->AttachmentOffset.setValue(Base::Placement(
        Base::Vector3d(100.0, 0.0, 0.0),
        Base::Rotation(Base::Vector3d::UnitZ, Base::toRadians(90.0))
    ));

    getDocument()->recompute();

    const auto placement = lcs->Placement.getValue();
    EXPECT_NEAR(placement.getPosition().x, 100.0, Base::Precision::Confusion());
    EXPECT_NEAR(placement.getPosition().y, 0.0, Base::Precision::Confusion());
    EXPECT_NEAR(placement.getPosition().z, 0.0, Base::Precision::Confusion());

    Base::Vector3d rotatedXAxis;
    placement.getRotation().multVec(Base::Vector3d::UnitX, rotatedXAxis);
    EXPECT_NEAR(rotatedXAxis.x, 0.0, Base::Precision::Confusion());
    EXPECT_NEAR(rotatedXAxis.y, 1.0, Base::Precision::Confusion());
    EXPECT_NEAR(rotatedXAxis.z, 0.0, Base::Precision::Confusion());
}

TEST_F(AttachExtensionTest, testAttacherEngineType)
{
    auto plane = getDocument()->addObject<Part::Plane>("Plane");
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngine3D");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");

    plane->AttacherEngine.setValue(1L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEnginePlane");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Plane");

    plane->AttacherEngine.setValue(2L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngineLine");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Line");

    plane->AttacherEngine.setValue(3L);
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEnginePoint");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Point");
}

TEST_F(AttachExtensionTest, testAttacherTypeEngine)
{
    auto plane = getDocument()->addObject<Part::Plane>("Plane");
    EXPECT_STREQ(plane->AttacherType.getValue(), "Attacher::AttachEngine3D");
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");

    plane->AttacherType.setValue("Attacher::AttachEnginePlane");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Plane");

    plane->AttacherType.setValue("Attacher::AttachEngineLine");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Line");

    plane->AttacherType.setValue("Attacher::AttachEnginePoint");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine Point");

    plane->AttacherType.setValue("Attacher::AttachEngine3D");
    plane->onExtendedDocumentRestored();
    EXPECT_STREQ(plane->AttacherEngine.getValueAsString(), "Engine 3D");
}
