// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Placement.h>
#include <Base/Precision.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/Part/App/FeaturePartFuse.h"
#include <src/App/InitApplication.h>

#include <BRep_TVertex.hxx>

// MOST OF THIS SHOULD GO INTO A FeaturePartBoolean.cpp test suite!

class FeaturePartFuseTest: public ::testing::Test
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
        _box1obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box1obj->Length.setValue(1);
        _box1obj->Width.setValue(2);
        _box1obj->Height.setValue(3);
        _box1obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 0, 0), Base::Rotation(), Base::Vector3d(0, 0, 0)));
        _box2obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box2obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, 0), Base::Rotation(), Base::Vector3d(0, 0, 0)));
        _box2obj->Length.setValue(1);
        _box2obj->Width.setValue(2);
        _box2obj->Height.setValue(3);
        _box3obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box3obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 3, 0), Base::Rotation(), Base::Vector3d(0, 0, 0)));
        _box3obj->Length.setValue(1);
        _box3obj->Width.setValue(2);
        _box3obj->Height.setValue(3);
        _box4obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box4obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2, 0), Base::Rotation(), Base::Vector3d(0, 0, 0)));
        _box4obj->Length.setValue(1);
        _box4obj->Width.setValue(2);
        _box4obj->Height.setValue(3);
        _box5obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box5obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 + Base::Precision::Confusion(), 0),
                            Base::Rotation(),
                            Base::Vector3d()));
        _box5obj->Length.setValue(1);
        _box5obj->Width.setValue(2);
        _box5obj->Height.setValue(3);
        _box6obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box6obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 - Base::Precision::Confusion() * 1000, 0),
                            Base::Rotation(),
                            Base::Vector3d()));
        _box6obj->Length.setValue(1);
        _box6obj->Width.setValue(2);
        _box6obj->Height.setValue(3);
        _fuse = static_cast<Part::Fuse*>(_doc->addObject("Part::Fuse"));
    }

    void TearDown() override
    {}


    Part::Box *_box1obj, *_box2obj, *_box3obj, *_box4obj, *_box5obj, *_box6obj;
    Part::Fuse* _fuse;
    App::Document* _doc;

private:
    std::string _docName;
};

TEST_F(FeaturePartFuseTest, testIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box2obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(bb.MinX, 0);
    EXPECT_FLOAT_EQ(bb.MinY, 0);
    EXPECT_FLOAT_EQ(bb.MinZ, 0);
    EXPECT_FLOAT_EQ(bb.MaxX, 1);
    EXPECT_FLOAT_EQ(bb.MaxY, 3);
    EXPECT_FLOAT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartFuseTest, testNonIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box3obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(bb.MinX, 0);
    EXPECT_FLOAT_EQ(bb.MinY, 0);
    EXPECT_FLOAT_EQ(bb.MinZ, 0);
    EXPECT_FLOAT_EQ(bb.MaxX, 1);
    EXPECT_FLOAT_EQ(bb.MaxY, 5);
    EXPECT_FLOAT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartFuseTest, testTouching)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box4obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(bb.MinX, 0);
    EXPECT_FLOAT_EQ(bb.MinY, 0);
    EXPECT_FLOAT_EQ(bb.MinZ, 0);
    EXPECT_FLOAT_EQ(bb.MaxX, 1);
    EXPECT_FLOAT_EQ(bb.MaxY, 4);
    EXPECT_FLOAT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartFuseTest, testAlmostTouching)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box5obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(bb.MinX, 0);
    EXPECT_FLOAT_EQ(bb.MinY, 0);
    EXPECT_FLOAT_EQ(bb.MinZ, 0);
    EXPECT_FLOAT_EQ(bb.MaxX, 1);
    EXPECT_FLOAT_EQ(bb.MaxY, 4);
    EXPECT_FLOAT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartFuseTest, testBarelyIntersecting)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box6obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FLOAT_EQ(bb.MinX, 0);
    EXPECT_FLOAT_EQ(bb.MinY, 0);
    EXPECT_FLOAT_EQ(bb.MinZ, 0);
    EXPECT_FLOAT_EQ(bb.MaxX, 1);
    EXPECT_FLOAT_EQ(bb.MaxY, 3.9999);
    EXPECT_FLOAT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartFuseTest, testMustExecute)
{
    // Act
    short mE = _fuse->mustExecute();
    // Assert
    EXPECT_FALSE(mE);
    _fuse->Base.setValue(_box1obj);
    // Assert
    mE = _fuse->mustExecute();
    EXPECT_FALSE(mE);
    // Act
    _fuse->Tool.setValue(_box2obj);
    // Assert
    mE = _fuse->mustExecute();
    EXPECT_TRUE(mE);
    _doc->recompute();
    mE = _fuse->mustExecute();
    EXPECT_FALSE(mE);
}

TEST_F(FeaturePartFuseTest, testGetProviderName)
{
    // Arrange

    // Act
    _fuse->execute();
    const char* name = _fuse->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

TEST_F(FeaturePartFuseTest, testRefine)
{
    // Arrange
    _fuse->Base.setValue(_box1obj);
    _fuse->Tool.setValue(_box2obj);

    // Act
    _fuse->execute();
    Part::TopoShape ts = _fuse->Shape.getValue();
    // what's the wire or face count here?
    std::vector<Part::TopoShape> subs = ts.getSubTopoShapes(TopAbs_FACE);  // WIRE
    EXPECT_EQ(subs.size(), 14);
    // Assert
    _fuse->Refine.setValue(true);
    _fuse->execute();
    ts = _fuse->Shape.getValue();
    subs = ts.getSubTopoShapes(TopAbs_FACE);  // WIRE
    EXPECT_EQ(subs.size(), 6);
}
