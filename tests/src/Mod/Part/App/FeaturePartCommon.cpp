// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Placement.h>
#include <Base/Precision.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/Part/App/FeaturePartCommon.h"
#include <src/App/InitApplication.h>

// #include <BRep_TVertex.hxx>

// Should some of this go into a FeaturePartBoolean.cpp test suite?

class FeaturePartCommonTest: public ::testing::Test
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
        _box2obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box3obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box4obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box5obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box6obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box1obj->Length.setValue(1);
        _box1obj->Width.setValue(2);
        _box1obj->Height.setValue(3);
        _box1obj->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));
        _box2obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, 0), Base::Rotation(), Base::Vector3d()));
        _box2obj->Length.setValue(1);
        _box2obj->Width.setValue(2);
        _box2obj->Height.setValue(3);
        _box3obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 3, 0), Base::Rotation(), Base::Vector3d()));
        _box3obj->Length.setValue(1);
        _box3obj->Width.setValue(2);
        _box3obj->Height.setValue(3);
        _box4obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2, 0), Base::Rotation(), Base::Vector3d()));
        _box4obj->Length.setValue(1);
        _box4obj->Width.setValue(2);
        _box4obj->Height.setValue(3);
        _box5obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 + Base::Precision::Confusion(), 0),
                            Base::Rotation(),
                            Base::Vector3d()));
        _box5obj->Length.setValue(1);
        _box5obj->Width.setValue(2);
        _box5obj->Height.setValue(3);
        _box6obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 - Base::Precision::Confusion() * 1000, 0),
                            Base::Rotation(),
                            Base::Vector3d()));
        _box6obj->Length.setValue(1);
        _box6obj->Width.setValue(2);
        _box6obj->Height.setValue(3);
        _common = static_cast<Part::Common*>(_doc->addObject("Part::Common"));
    }

    void TearDown() override
    {}


    Part::Box *_box1obj, *_box2obj, *_box3obj, *_box4obj, *_box5obj, *_box6obj;
    Part::Common* _common;
    App::Document* _doc;

private:
    std::string _docName;
};

TEST_F(FeaturePartCommonTest, testIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box2obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_EQ(bb.MinX, 0);
    EXPECT_EQ(bb.MinY, 1);
    EXPECT_EQ(bb.MinZ, 0);
    EXPECT_EQ(bb.MaxX, 1);
    EXPECT_EQ(bb.MaxY, 2);
    EXPECT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartCommonTest, testNonIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box3obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    // EXPECT_EQ(bb.Volume(),-1);
    // EXPECT_EQ(bb.Volume(),0);
}

TEST_F(FeaturePartCommonTest, testTouching)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box4obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    // EXPECT_EQ(bb.Volume(),0);
}

TEST_F(FeaturePartCommonTest, testAlmostTouching)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box5obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_FALSE(bb.IsValid());
    // EXPECT_EQ(bb.Volume(),-1);
    // EXPECT_EQ(bb.Volume(),0);
}

TEST_F(FeaturePartCommonTest, testBarelyIntersecting)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box6obj);

    // Act
    _common->execute();
    Part::TopoShape ts = _common->Shape.getValue();
    Base::BoundBox3d bb = ts.getBoundBox();

    // Assert
    EXPECT_EQ(bb.MinX, 0);
    EXPECT_EQ(bb.MinY, 1.9999);
    EXPECT_EQ(bb.MinZ, 0);
    EXPECT_EQ(bb.MaxX, 1);
    EXPECT_EQ(bb.MaxY, 2);
    EXPECT_EQ(bb.MaxZ, 3);
}

TEST_F(FeaturePartCommonTest, testMustExecute)
{
    // Act
    short mE = _common->mustExecute();
    // Assert
    EXPECT_FALSE(mE);
    _common->Base.setValue(_box1obj);
    // Assert
    mE = _common->mustExecute();
    EXPECT_FALSE(mE);
    // Act
    _common->Tool.setValue(_box2obj);
    // Assert
    mE = _common->mustExecute();
    EXPECT_TRUE(mE);
    _doc->recompute();
    mE = _common->mustExecute();
    EXPECT_FALSE(mE);
}

TEST_F(FeaturePartCommonTest, testGetProviderName)
{
    // Arrange

    // Act
    _common->execute();
    const char* name = _common->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderBoolean");
}

namespace Part
{
void PrintTo(ShapeHistory sh, std::ostream* os)
{
    const char* types[] =
        {"Compound", "CompSolid", "Solid", "Shell", "Face", "Wire", "Edge", "Vertex", "Shape"};
    *os << "History for " << types[sh.type] << " is ";
    for (const auto& it : sh.shapeMap) {
        int old_shape_index = it.first;
        *os << " " << old_shape_index << ": ";
        if (!it.second.empty()) {
            for (auto it2 : it.second) {
                *os << it2 << " ";
            }
        }
    }
    *os << std::endl;
}
}  // namespace Part

TEST_F(FeaturePartCommonTest, testHistory)
{
    // Arrange
    _common->Base.setValue(_box1obj);
    _common->Tool.setValue(_box2obj);

    // Act and Assert
    std::vector<Part::ShapeHistory> hist = _common->History.getValues();
    EXPECT_EQ(hist.size(), 0);

    // This creates the histories classically generated by FreeCAD for comparison
    using MapList = std::map<int, std::vector<int>>;
    using List = std::vector<int>;
    MapList compare1 =
        {{0, List {0}}, {1, List {5}}, {2, List()}, {3, List {2}}, {4, List {3}}, {5, List {1}}};
    MapList compare2 =
        {{0, List {0}}, {1, List {5}}, {2, List {4}}, {3, List()}, {4, List {3}}, {5, List {1}}};

    _common->execute();
    hist = _common->History.getValues();
    EXPECT_EQ(hist.size(), 2);
    EXPECT_EQ(hist[0].shapeMap, compare1);
    EXPECT_EQ(hist[1].shapeMap, compare2);
    _common->Base.setValue(_box2obj);
    _common->Tool.setValue(_box1obj);
    _common->execute();
    hist = _common->History.getValues();
    // std::cout << testing::PrintToString(hist[0]) << testing::PrintToString(hist[1]);
    EXPECT_EQ(hist.size(), 2);
    EXPECT_EQ(hist[1].shapeMap, compare1);
    EXPECT_EQ(hist[0].shapeMap, compare2);
}
