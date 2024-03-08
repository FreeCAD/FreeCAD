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
    std::vector<TopoDS_Shape> edges {edge1, edge2, edge3, edge4};

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

TEST_F(WireJoinerTest, setOutline)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};

    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, -0.1, 0.0), gp_Pnt(-0.1, 1.1, 0.0)).Edge()};
    auto edge5 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 1.1, 0.0), gp_Pnt(1.1, 1.1, 0.0)).Edge()};
    auto edge6 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoOutline.addShape()
    std::vector<TopoDS_Shape> edgesNoOutline {edge1, edge2, edge3, edge4, edge5, edge6};
    // A vector of edges used as argument for wjOutline.addShape()
    std::vector<TopoDS_Shape> edgesOutline {edge1, edge2, edge3, edge4, edge5, edge6};

    // To see the effect of setOutline() it is necessary to set the user parameter "Iteration"
    // to a value, in this case, less than zero before the WireObjects are initialized.
    // To see the correct value for this parameter refer to method WireJoinerP::canShowShape()

    auto hParam {App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/WireJoiner")};
    auto catchIteration {hParam->GetInt("Iteration", 0)};
    hParam->SetInt("Iteration", -1);

    // A document where there will WireJoiner.Build() will be called having setOutline() set to
    // false
    auto docNoOutline {App::GetApplication().getActiveDocument()};

    auto _docNameOutline {App::GetApplication().getUniqueDocumentName("docOutline")};
    App::GetApplication().newDocument(_docNameOutline.c_str(), "docOutlineUser");
    // A document where there will WireJoiner.Build() will be called having setOutline() set to true
    auto docOutline {App::GetApplication().getActiveDocument()};

    // A WireJoiner object where the value of setOutline() will be changed but no shapes will be
    // built
    auto wjNoBuild {WireJoiner()};
    // A WireJoiner object where setOutline() will be set to false
    auto wjNoOutline {WireJoiner()};
    // A WireJoiner object where setOutline() will be set to true
    auto wjOutline {WireJoiner()};

    // Reset the parameter to its previous value.
    hParam->SetInt("Iteration", catchIteration);

    // Act

    // Changing only the value of setOutline(). This should set wjNoBuild.IsDone() to false
    wjNoBuild.setOutline(true);

    // We can see the effect of setOutline by searching, among all the DocumentObjects created by
    // the logic, if there are any with a label containing "removed"

    // Testing first the Document where WireJoiner::Build() will be executed with setOutline set to
    // false

    App::GetApplication().setActiveDocument(docNoOutline);

    wjNoOutline.addShape(edgesNoOutline);
    wjNoOutline.setOutline(false);
    wjNoOutline.Build();

    auto objsInDoc {docNoOutline->getObjects()};

    auto foundRemovedNoOutline {false};
    for (const auto& obj : objsInDoc) {
        foundRemovedNoOutline =
            (std::string(obj->Label.getValue()).find("removed") != std::string::npos);
        if (foundRemovedNoOutline) {
            break;
        }
    }

    // Then testing the Document where WireJoiner::Build() will be executed with setOutline set to
    // true

    App::GetApplication().setActiveDocument(docOutline);

    wjOutline.addShape(edgesOutline);
    // same as wjOutline.setOutline(true);
    wjOutline.setOutline();
    wjOutline.Build();

    objsInDoc = docOutline->getObjects();

    auto foundRemovedOutline {false};
    for (const auto& obj : objsInDoc) {
        foundRemovedOutline =
            (std::string(obj->Label.getValue()).find("removed") != std::string::npos);
        if (foundRemovedOutline) {
            break;
        }
    }

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setOutline()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // In a document where WireJoiner::Build() is executed with setOutline() set to false there
    // shouldn't be DocumentObjects with "removed" in their label
    EXPECT_FALSE(foundRemovedNoOutline);

    // In a document where WireJoiner::Build() is executed with setOutline() set to true there
    // should be at least a DocumentObject with "removed" in its label
    EXPECT_TRUE(foundRemovedOutline);
}

TEST_F(WireJoinerTest, setTightBound)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 1.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoTightBound.addShape()
    std::vector<TopoDS_Shape> edgesNoTightBound {edge1, edge2, edge3, edge4};
    // A vector of edges used as argument for wjTightBound.addShape()
    std::vector<TopoDS_Shape> edgesTightBound {edge1, edge2, edge3, edge4};

    // A WireJoiner object where the value of setTightBound() will be changed but no shapes will be
    // built
    auto wjNoBuild {WireJoiner()};
    // A WireJoiner object where setTightBound() will be set to false
    auto wjNoTightBound {WireJoiner()};
    // A WireJoiner object where setTightBound() will be set to true
    auto wjTightBound {WireJoiner()};

    // An empty TopoShape that will contain the shapes returned by wjNoTightBound.getOpenWires()
    auto wireNoTightBound {TopoShape(1)};
    // An empty TopoShape that will contain the shapes returned by wjTightBound.getOpenWires()
    auto wireTightBound {TopoShape(2)};

    // Act

    // Changing only the value of setTightBound(). This should set wjNoBuild.IsDone() to false
    wjNoBuild.setTightBound(false);

    // To see the effect of setTightBound() we call WireJoiner::Build() and then
    // WireJoiner::getOpenWires()

    wjNoTightBound.addShape(edgesNoTightBound);
    wjNoTightBound.setTightBound(false);
    wjNoTightBound.Build();
    wjNoTightBound.getOpenWires(wireNoTightBound, nullptr, false);

    wjTightBound.addShape(edgesTightBound);
    // same as wjTightBound.setTightBound(true);
    wjTightBound.setTightBound();
    wjTightBound.Build();
    wjTightBound.getOpenWires(wireTightBound, nullptr, false);

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setTightBound()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // Calling wjNoTightBound.Build() will put all the edges inside the private object
    // WireJoiner::WireJoinerP::openWireCompound.
    // In this case the number of edges is equal to 6 because:
    // edge1 and edge2 aren't modified => 2 edges
    // edge3 and edge4 are both split in 2 edges => 4 edges
    // The split is made at the intersection point (0.5, 0.5, 0.0)
    EXPECT_EQ(wireNoTightBound.getSubTopoShapes(TopAbs_EDGE).size(), 6);

    // Calling wjTightBound.Build() will put inside the private object
    // WireJoiner::WireJoinerP::openWireCompound only the edges that don't contribute to the
    // creation of any closed wire.
    // In this case the number of those edges is equal to 1 and that edge is the one with vertexes
    // at the coordinates (0.5, 0.5, 0.0) - (0.0, 1.0, 0.0)
    EXPECT_EQ(wireTightBound.getSubTopoShapes(TopAbs_EDGE).size(), 1);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
