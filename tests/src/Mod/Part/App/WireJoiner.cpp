// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include "Mod/Part/App/WireJoiner.h"

#include "PartTestHelpers.h"

#include <BRepBuilderAPI_MakeShape.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

using namespace Part;
using namespace PartTestHelpers;

class WireJoinerTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
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

TEST_F(WireJoinerTest, addShape)
{
    // Arrange

    // Create various edges that will be used to define the arguments of the various
    // WireJoiner::addShape() calls

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};

    // A vector of TopoDS_Shape used as argument for wjvTDS.addShape()
    std::vector<TopoDS_Shape> edges = {edge1, edge2, edge3, edge4};

    // Create various TopoShapes used as arguments for wjvTS.addShape()

    auto edge1TS {TopoShape(edge1, 1)};
    auto edge2TS {TopoShape(edge2, 2)};

    // A wire TopoShape used as argument for wjTS.addShape()
    auto wireTS {TopoShape(BRepBuilderAPI_MakeWire(edge1, edge2, edge3).Wire(), 3)};
    // An empty TopoShape that will contain the shapes added by wjvTS.addShape()
    auto wirevTS {TopoShape(4)};
    // An empty TopoShape that will contain the shapes added by wjvTDS.addShape()
    auto wirevTDS {TopoShape(5)};

    // Create 3 WireJoiner objects, one for every definition of WireJoiner::addShape()

    // A WireJoiner object where it will be added a TopoShape without calling WireJoiner::Build()
    // afterwards
    auto wjTSNotDone {WireJoiner()};
    // A WireJoiner object where it will be added a TopoShape
    auto wjTS {WireJoiner()};
    // A WireJoiner object where it will be added a vector of TopoShapes
    auto wjvTS {WireJoiner()};
    // A WireJoiner object where it will be added a vector of TopoDS_Shapes
    auto wjvTDS {WireJoiner()};

    // Act

    // Calling only WireJoiner::addShape(). Expected result is that wjTSNotDone.Done() is false
    wjTSNotDone.addShape(wireTS);

    // Calling WireJoiner::addShape() and then WireJoiner::Build().
    // If we don't call WireJoiner::Build() we can't see the effect of WireJoiner::addShape() as it
    // adds the shapes in the private member WireJoinerP::sourceEdgeArray

    wjTS.addShape(wireTS);
    wjTS.Build();
    // The wire in wjTS is open, therefor to see the effect of wjTS.addShape() we must call
    // wjTS.getOpenWires() and put the result in wireTS
    wjTS.getOpenWires(wireTS, nullptr, false);

    wjvTS.addShape({edge1TS, edge2TS});
    wjvTS.Build();
    // The wire in wjvTS is open, therefor to see the effect of wjvTS.addShape() we must call
    // wjvTS.getOpenWires() and put the result in wirevTS
    wjvTS.getOpenWires(wirevTS, nullptr, false);

    wjvTDS.addShape(edges);
    wjvTDS.Build();
    // The wire in wjvTDS is closed, therefor to see the effect of wjvTDS.addShape() we can smply
    // call wjvTDS.Shape() to replace the shape in wirevTDS
    wirevTDS.setShape(wjvTDS.Shape());

    // Assert

    // Check the output of WireJoiner::IsDone().
    // It should be true for all the objects that executed also WireJoiner::Build()
    // (All except wjTSNotDone)
    EXPECT_FALSE(wjTSNotDone.IsDone());
    EXPECT_TRUE(wjTS.IsDone());
    EXPECT_TRUE(wjvTS.IsDone());
    EXPECT_TRUE(wjvTDS.IsDone());

    // wireTS is build with 3 edges. The same quantity should be in the shape built
    EXPECT_EQ(wireTS.getSubTopoShapes(TopAbs_EDGE).size(), 3);
    // wirevTS is build with 2 edges. The same quantity should be in the shape built
    EXPECT_EQ(wirevTS.getSubTopoShapes(TopAbs_EDGE).size(), 2);
    // wirevTDS is build with 4 edges. The same quantity should be in the shape built
    EXPECT_EQ(wirevTDS.getSubTopoShapes(TopAbs_EDGE).size(), 4);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
