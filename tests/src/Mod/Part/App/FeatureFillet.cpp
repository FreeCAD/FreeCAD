// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
class FeatureFilletTest: public ::testing::Test, public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _boxes[0]->Length.setValue(4);
        _boxes[0]->Width.setValue(5);
        _boxes[0]->Height.setValue(6);
        _boxes[0]->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, 6), Base::Rotation(), Base::Vector3d()));
        _boxes[1]->Length.setValue(1);
        _boxes[1]->Width.setValue(2);
        _boxes[1]->Height.setValue(3);
        _fused = _doc->addObject<Part::Fuse>();
        _fused->Base.setValue(_boxes[0]);
        _fused->Tool.setValue(_boxes[1]);
        _fused->execute();
        _fillet = _doc->addObject<Part::Fillet>();
    }

    void TearDown() override
    {}

    Part::Fuse* _fused = nullptr;     // NOLINT Can't be private in a test framework
    Part::Fillet* _fillet = nullptr;  // NOLINT Can't be private in a test framework
};

// Unfortunately for these next two tests, there are upstream errors in OCCT
// at least until 7.5.2 that cause some fillets that intersect each other to
// fail.  Until that's fixed, test subsets of the complete fillet list.

TEST_F(FeatureFilletTest, testOtherEdges)
{
    const double baseVolume =
        _boxes[0]->Length.getValue() * _boxes[0]->Width.getValue() * _boxes[0]->Height.getValue()
        + _boxes[1]->Length.getValue() * _boxes[1]->Width.getValue() * _boxes[1]->Height.getValue();
    // Arrange
    _fillet->Base.setValue(_fused);
    Part::TopoShape ts = _fused->Shape.getValue();
    unsigned long sec = ts.countSubElements("Edge");
    // Assert
    EXPECT_EQ(sec, 25);
    // Act
    _fused->Refine.setValue(true);
    _fused->execute();
    ts = _fused->Shape.getValue();
    sec = ts.countSubElements("Edge");
    // Assert
    EXPECT_EQ(sec, 24);

    // Act
    _fillet->Edges.setValues(PartTestHelpers::_getFilletEdges({15, 17}, 0.5, 0.5));
    double fusedVolume = PartTestHelpers::getVolume(_fused->Shape.getValue());
    double filletVolume = PartTestHelpers::getVolume(_fillet->Shape.getValue());
    // Assert
    EXPECT_DOUBLE_EQ(fusedVolume, baseVolume);
    EXPECT_DOUBLE_EQ(filletVolume, 0.0);
    // Act
    _fillet->execute();
    filletVolume = PartTestHelpers::getVolume(_fillet->Shape.getValue());
    // Assert
    EXPECT_NEAR(filletVolume, 125.57079, 1e-5);
}

TEST_F(FeatureFilletTest, testMostEdges)
{
    // Arrange
    _fused->Refine.setValue(true);
    // _fused->execute();
    _fillet->Base.setValue(_fused);
    _fillet->Edges.setValues(PartTestHelpers::_getFilletEdges(
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 19, 20, 22, 23, 24},
        0.4,
        0.4));
    // Act
    _fillet->execute();
    double filletVolume = PartTestHelpers::getVolume(_fillet->Shape.getValue());
    // Assert
    EXPECT_NEAR(filletVolume, 118.38763, 1e-5);
}

// Worth noting that FeaturePartCommon with insufficient parameters says MustExecute false,
// but FeatureFillet says MustExecute true.  Not a condition that should ever really be hit.

TEST_F(FeatureFilletTest, testMustExecute)
{
    // Assert
    EXPECT_TRUE(_fillet->mustExecute());
    // Act
    _fillet->Base.setValue(_boxes[0]);
    // Assert
    EXPECT_TRUE(_fillet->mustExecute());
    // Act
    _fillet->Edges.setValues(PartTestHelpers::_getFilletEdges({1}, 0.5, 0.5));
    // Assert
    EXPECT_TRUE(_fillet->mustExecute());
    // Act
    _doc->recompute();
    // Assert
    EXPECT_FALSE(_fillet->mustExecute());
}

TEST_F(FeatureFilletTest, testGetProviderName)
{
    // Act
    _fillet->execute();
    const char* name = _fillet->getViewProviderName();
    // Assert
    EXPECT_STREQ(name, "PartGui::ViewProviderFillet");
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

// void PrintTo(const TopoDS_Shape& ds, std::ostream* os)
// {
//     *os << "TopoDS_Shape ";
//     for (TopExp_Explorer ex(ds, TopAbs_VERTEX); ex.More(); ex.Next()) {
//         gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(ex.Current()));
//         *os << "(" << point.X() << "," << point.Y() << ","  << point.Z() << ") ";
//     }
//     *os << std::endl;
// }

// void edgesAtBusyVertexes(Part::TopoShape ts)
// {
//     const char* categories[] = { "Face", "Wire", "Edge", "Vertex" };

//     for (auto category : categories ) {
//         int count =  ts.countSubShapes(category);
//         for ( int index=1; index <= count; index++ ) {
//             std::string name = category + std::to_string(index);
//             const char * cname = name.c_str();
//             TopoDS_Shape ss = ts.getSubShape(cname);
//             os << cname << ": ";
//             for (TopExp_Explorer ex(ss, TopAbs_VERTEX); ex.More(); ex.Next()) {
//                 gp_Pnt point = BRep_Tool::Pnt(TopoDS::Vertex(ex.Current()));
//                 os << "(" << point.X() << "," << point.Y() << ","  << point.Z() << ") ";
//             }
//             os << std::endl;
//         }
//     }
// }
