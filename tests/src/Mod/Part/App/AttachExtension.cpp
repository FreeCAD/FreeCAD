// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include <src/App/InitApplication.h>
#include <App/Document.h>
#include <Mod/Part/App/FeaturePartBox.h>
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

TEST_F(AttachExtensionTest, testMidPlane)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    secondPlane->Placement.setValue(
        Base::Placement(
            Base::Vector3d(0, 100, 20),
            Base::Rotation(Base::Vector3d(1, 0, 0), std::numbers::pi)
        )
    );
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");
    EXPECT_STREQ(midPlane->MapMode.getValueAsString(), "MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 50.0, 1e-7);
    EXPECT_NEAR(position.z, 10.0, 1e-7);

    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, 0.0, 1e-7);
    EXPECT_NEAR(normal.y, 0.0, 1e-7);
    EXPECT_NEAR(normal.z, 1.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsFaceAngle)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    const Base::Placement secondPlacement(
        Base::Vector3d(0, 0, 10),
        Base::Rotation(Base::Vector3d(1, 0, 0), std::numbers::pi / 2.0)
    );
    secondPlane->Placement.setValue(secondPlacement);
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Vector3d position = midPlane->Placement.getValue().getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 27.5, 1e-7);
    EXPECT_NEAR(position.z, 27.5, 1e-7);

    Base::Vector3d secondNormal;
    secondPlacement.getRotation().multVec(Base::Vector3d(0, 0, 1), secondNormal);
    Base::Vector3d expectedNormal(0, 0, 1);
    expectedNormal += secondNormal;
    expectedNormal.Normalize();

    Base::Vector3d normal;
    midPlane->Placement.getValue().getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, expectedNormal.x, 1e-7);
    EXPECT_NEAR(normal.y, expectedNormal.y, 1e-7);
    EXPECT_NEAR(normal.z, expectedNormal.z, 1e-7);

    // The faces are z = 0 and y = 0, so they intersect on the X axis.
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 0, 0) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(10, 0, 0) - position), 0.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsConnectedFaces)
{
    auto box = getDocument()->addObject<Part::Box>("Box");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(box);
    ASSERT_TRUE(midPlane);

    box->Length.setValue(100.0);
    box->Width.setValue(60.0);
    box->Height.setValue(20.0);
    midPlane->AttachmentSupport.setValues({box, box}, {"Face6", "Face4"});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    EXPECT_NEAR(position.x, 50.0, 1e-7);
    EXPECT_NEAR(position.y, 50.0, 1e-7);
    EXPECT_NEAR(position.z, 10.0, 1e-7);

    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);
    EXPECT_NEAR(normal.x, 0.0, 1e-7);
    EXPECT_NEAR(normal.y, -std::sqrt(0.5), 1e-7);
    EXPECT_NEAR(normal.z, std::sqrt(0.5), 1e-7);

    // Face6 (z = Height) and Face4 (y = Width) meet at the edge y = 60, z = 20.
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 60, 20) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(100, 60, 20) - position), 0.0, 1e-7);
}

TEST_F(AttachExtensionTest, testMidPlaneBisectsObliqueFaceAngle)
{
    auto firstPlane = getDocument()->addObject<Part::Plane>("FirstPlane");
    auto secondPlane = getDocument()->addObject<Part::Plane>("SecondPlane");
    auto midPlane = getDocument()->addObject<Part::Plane>("MidPlane");

    ASSERT_TRUE(firstPlane);
    ASSERT_TRUE(secondPlane);
    ASSERT_TRUE(midPlane);

    // Rotating the second plane about the Y axis through the origin keeps both planes on the
    // Y axis, so the faces intersect there at a 50 degree dihedral angle. The differing sizes
    // put the face centers off the bisector, so the base point has to be corrected onto it.
    const double angle = 50.0 * std::numbers::pi / 180.0;
    secondPlane->Length.setValue(60.0);
    secondPlane->Width.setValue(60.0);
    secondPlane->Placement.setValue(
        Base::Placement(Base::Vector3d(0, 0, 0), Base::Rotation(Base::Vector3d(0, 1, 0), angle))
    );
    midPlane->AttachmentSupport.setValues({firstPlane, secondPlane}, {"", ""});
    midPlane->MapMode.setValue("MidPlane");

    getDocument()->recompute();

    const Base::Placement placement = midPlane->Placement.getValue();
    const Base::Vector3d position = placement.getPosition();
    Base::Vector3d normal;
    placement.getRotation().multVec(Base::Vector3d(0, 0, 1), normal);

    EXPECT_NEAR(normal.x, std::sin(angle / 2.0), 1e-7);
    EXPECT_NEAR(normal.y, 0.0, 1e-7);
    EXPECT_NEAR(normal.z, std::cos(angle / 2.0), 1e-7);

    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 0, 0) - position), 0.0, 1e-7);
    EXPECT_NEAR(normal.Dot(Base::Vector3d(0, 10, 0) - position), 0.0, 1e-7);
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
