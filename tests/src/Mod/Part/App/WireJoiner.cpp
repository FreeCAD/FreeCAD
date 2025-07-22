// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include "Mod/Part/App/WireJoiner.h"
#include <Mod/Part/App/TopoShapeOpCode.h>

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
    // The wire in wjTS is open, therefore to see the effect of wjTS.addShape() we must call
    // wjTS.getOpenWires() and put the result in wireTS
    wjTS.getOpenWires(wireTS, nullptr, false);

    wjvTS.addShape({edge1TS, edge2TS});
    wjvTS.Build();
    // The wire in wjvTS is open, therefore to see the effect of wjvTS.addShape() we must call
    // wjvTS.getOpenWires() and put the result in wirevTS
    wjvTS.getOpenWires(wirevTS, nullptr, false);

    wjvTDS.addShape(edges);
    wjvTDS.Build();
    // The wire in wjvTDS is closed, therefore to see the effect of wjvTDS.addShape() we can simply
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
    // Calling wjNoTightBound.Build() will put all the edges inside the private object
    // WireJoiner::WireJoinerP::openWireCompound.
    wjNoTightBound.Build();
    wjNoTightBound.getOpenWires(wireNoTightBound, nullptr, false);

    wjTightBound.addShape(edgesTightBound);
    // same as wjTightBound.setTightBound(true);
    wjTightBound.setTightBound();
    // Calling wjTightBound.Build() will put inside the private object
    // WireJoiner::WireJoinerP::openWireCompound only the edges that don't contribute to the
    // creation of any closed wire.
    wjTightBound.Build();
    wjTightBound.getOpenWires(wireTightBound, nullptr, false);

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setTightBound()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // In this case the number of edges is equal to 6 because:
    // edge1 and edge2 aren't modified => 2 edges
    // edge3 and edge4 are both split in 2 edges => 4 edges
    // The split is made at the intersection point (0.5, 0.5, 0.0)
    EXPECT_EQ(wireNoTightBound.getSubTopoShapes(TopAbs_EDGE).size(), 6);

    // In this case the number of those edges is equal to 1 and that edge is the one with vertexes
    // at the coordinates (0.5, 0.5, 0.0) - (0.0, 1.0, 0.0)
    EXPECT_EQ(wireTightBound.getSubTopoShapes(TopAbs_EDGE).size(), 1);
}

TEST_F(WireJoinerTest, setSplitEdges)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 1.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoSplitEdges.addShape()
    std::vector<TopoDS_Shape> edgesNoSplitEdges {edge1, edge2};
    // A vector of edges used as argument for wjSplitEdges.addShape()
    std::vector<TopoDS_Shape> edgesSplitEdges {edge1, edge2};

    // A WireJoiner object where the value of setSplitEdges() will be changed but no shapes will be
    // built
    auto wjNoBuild {WireJoiner()};

    // A WireJoiner object where setSplitEdges() will be set to false
    auto wjNoSplitEdges {WireJoiner()};
    // To see it's effect it's necessary also to call setTightBound(false) otherwise
    // WireJoiner::WireJoinerP::splitEdges() will be called in any case
    wjNoSplitEdges.setTightBound(false);

    // A WireJoiner object where setSplitEdges() will be set to true
    auto wjSplitEdges {WireJoiner()};
    // To see it's effect it's necessary also to call setTightBound(false) otherwise
    // WireJoiner::WireJoinerP::splitEdges() will be called in any case
    wjSplitEdges.setTightBound(false);

    // An empty TopoShape that will contain the shapes returned by wjNoSplitEdges.getOpenWires()
    auto wireNoSplitEdges {TopoShape(1)};
    // An empty TopoShape that will contain the shapes returned by wjSplitEdges.getOpenWires()
    auto wireSplitEdges {TopoShape(2)};

    // Act

    // Changing only the value of setSplitEdges(). This should set wjNoBuild.IsDone() to false
    wjNoBuild.setSplitEdges(false);

    // To see the effect of setSplitEdges() we call WireJoiner::Build() and then
    // WireJoiner::getOpenWires()

    wjNoSplitEdges.addShape(edgesNoSplitEdges);
    wjNoSplitEdges.setSplitEdges(false);
    // Calling wjNoSplitEdges.Build() will put all the edges that don't contribute to the creation
    // of a closed wire inside the private object WireJoiner::WireJoinerP::openWireCompound.
    wjNoSplitEdges.Build();
    wjNoSplitEdges.getOpenWires(wireNoSplitEdges, nullptr, false);

    wjSplitEdges.addShape(edgesSplitEdges);
    // same as wjSplitEdges.setSplitEdges(true);
    wjSplitEdges.setSplitEdges();
    // Calling wjSplitEdges.Build() will put inside the private object
    // WireJoiner::WireJoinerP::openWireCompound all the edges processed by
    // WireJoiner::WireJoinerP::splitEdges().
    wjSplitEdges.Build();
    wjSplitEdges.getOpenWires(wireSplitEdges, nullptr, false);

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setSplitEdges()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // In this case the number of edges is equal to the number of edges added with
    // wjNoSplitEdges.addShape() because none of them is used for the creation of a closed wire.
    EXPECT_EQ(wireNoSplitEdges.getSubTopoShapes(TopAbs_EDGE).size(), 2);

    // In this case the number of those edges is equal to 4 because both the edges added with
    // wjSplitEdges.addShape() have been split at the intersection point (0.5, 0.5, 0.0)
    EXPECT_EQ(wireSplitEdges.getSubTopoShapes(TopAbs_EDGE).size(), 4);
}

TEST_F(WireJoinerTest, setMergeEdges)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 0.0, 0.0), gp_Pnt(1.1, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, -0.1, 0.0), gp_Pnt(1.0, 1.1, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoMergeEdges.addShape()
    std::vector<TopoDS_Shape> edgesNoMergeEdges {edge1, edge2, edge3};
    // A vector of edges used as argument for wjMergeEdges.addShape()
    std::vector<TopoDS_Shape> edgesMergeEdges {edge1, edge2, edge3};

    // A WireJoiner object where the value of setMergeEdges() will be changed but no shapes will be
    // built
    auto wjNoBuild {WireJoiner()};

    // A WireJoiner object where setMergeEdges() will be set to false
    auto wjNoMergeEdges {WireJoiner()};
    // To see it's effect it's necessary also to call setTightBound(false) otherwise
    // WireJoiner::WireJoinerP::MergeEdges() will be called in any case
    wjNoMergeEdges.setTightBound(false);

    // A WireJoiner object where setMergeEdges() will be set to true
    auto wjMergeEdges {WireJoiner()};
    // To see it's effect it's necessary also to call setTightBound(false) otherwise
    // WireJoiner::WireJoinerP::MergeEdges() will be called in any case
    wjMergeEdges.setTightBound(false);

    // An empty TopoShape that will contain the shapes returned by wjNoMergeEdges.getOpenWires()
    auto wireNoMergeEdges {TopoShape(1)};
    // An empty TopoShape that will contain the shapes returned by wjMergeEdges.getOpenWires()
    auto wireMergeEdges {TopoShape(2)};

    // Act

    // Changing only the value of setMergeEdges(). This should set wjNoBuild.IsDone() to false
    wjNoBuild.setMergeEdges(false);

    // To see the effect of setMergeEdges() we call WireJoiner::Build() and then
    // WireJoiner::getOpenWires()

    wjNoMergeEdges.addShape(edgesNoMergeEdges);
    wjNoMergeEdges.setMergeEdges(false);
    // Calling wjNoMergeEdges.Build() will put all the edges produced by
    // WireJoiner::WireJoinerP::splitEdges() in the private object
    // WireJoiner::WireJoinerP::openWireCompound.
    wjNoMergeEdges.Build();
    wjNoMergeEdges.getOpenWires(wireNoMergeEdges, nullptr, false);

    wjMergeEdges.addShape(edgesMergeEdges);
    // same as wjMergeEdges.setMergeEdges(true);
    wjMergeEdges.setMergeEdges();
    // Calling wjMergeEdges.Build() will put, among the edges produced by
    // WireJoiner::WireJoinerP::splitEdges(), only the edges that are connected to only one of the
    // others by a single vertex in the private object WireJoiner::WireJoinerP::openWireCompound.
    // In the code those are called SuperEdges and are
    // processed by WireJoiner::WireJoinerP::findSuperEdges().
    wjMergeEdges.Build();
    wjMergeEdges.getOpenWires(wireMergeEdges, nullptr, false);

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setMergeEdges()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // In this case the number of edges is equal to 9 because all the 3 edges intersect the other 2
    // and are therefore split in 3 edges each.
    EXPECT_EQ(wireNoMergeEdges.getSubTopoShapes(TopAbs_EDGE).size(), 9);

    // In this case the number of edges is equal to 6 because, among the 9 produced by
    // WireJoiner::WireJoinerP::splitEdges(), 3 of them are connected to more than one other edge
    // and therefore aren't added by WireJoiner::WireJoinerP::findSuperEdges()
    EXPECT_EQ(wireMergeEdges.getSubTopoShapes(TopAbs_EDGE).size(), 6);
}

TEST_F(WireJoinerTest, setTolerance)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto pi {acos(-1)};

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.1, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.9, 0.0, 0.0)).Edge()};
    auto edge5 {
        BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0),
                                gp_Pnt(0.9 * std::cos(pi / 18), 0.9 * std::sin(pi / 18), 0.0))
            .Edge()};

    // A vector of edges used as argument for wjNegtol.addShape()
    std::vector<TopoDS_Shape> edgesNegtol {edge1, edge2, edge3};
    // A vector of edges used as argument for wjtol.addShape()
    std::vector<TopoDS_Shape> edgestol {edge1, edge2, edge3};
    // A vector of edges used as argument for wjNegatol.addShape()
    std::vector<TopoDS_Shape> edgesNegatol {edge2, edge3, edge4, edge5};
    // A vector of edges used as argument for wjatol.addShape()
    std::vector<TopoDS_Shape> edgesatol {edge2, edge3, edge4, edge5};

    // A WireJoiner object where the value of setTolerance() will be changed but no shapes will be
    // built
    auto wjNoBuild {WireJoiner()};

    // A WireJoiner object where setTolerance() will be called passing to the argument tol a
    // negative value.
    auto wjNegtol {WireJoiner()};
    // A WireJoiner object where setTolerance() will be called passing to the argument tol a
    // value both positive and not equal to WireJoiner::WireJoinerP::myTol
    auto wjtol {WireJoiner()};

    // A WireJoiner object where setTolerance() will be called passing to the argument atol a
    // negative value.
    auto wjNegatol {WireJoiner()};
    // A WireJoiner object where setTolerance() will be called passing to the argument atol a
    // value both positive and not equal to WireJoiner::WireJoinerP::myAngularTol
    auto wjatol {WireJoiner()};

    // An empty TopoShape that will contain the shapes returned by wjNegtol.getOpenWires()
    auto wireNegtol {TopoShape(1)};
    // An empty TopoShape that will contain the shapes returned by wjtol.getOpenWires()
    auto wiretol {TopoShape(2)};

    // An empty TopoShape that will contain the shapes returned by wjNegatol.getOpenWires()
    auto wireNegatol {TopoShape(3)};
    // An empty TopoShape that will contain the shapes returned by wjatol.getOpenWires()
    auto wireatol {TopoShape(4)};

    // Act

    // Changing only the value of setTolerance(). This should set wjNoBuild.IsDone() to false
    wjNoBuild.setTolerance(0.1);

    // To see the effect of setTolerance() we call WireJoiner::Build() and then
    // WireJoiner::getOpenWires() to get the edges, if any, that aren't used to create a closed wire

    wjNegtol.addShape(edgesNegtol);
    // Setting tol to a negative value won't have effect and therefore wjNegtol.pimpl->myTol will
    // keep the default value.
    // It's better also to give a negative value for the argument atol otherwise setTolerance()
    // will set it to 0.0
    wjNegtol.setTolerance(-0.1, -pi);
    wjNegtol.Build();
    wjNegtol.getOpenWires(wireNegtol, nullptr, false);

    wjtol.addShape(edgestol);
    // Setting tol to a value that will change wjNegtol.pimpl->myTol.
    // It's better also to give a negative value for the argument atol otherwise setTolerance()
    // will set it to 0.0
    wjtol.setTolerance(0.2, -pi);
    wjtol.Build();
    wjtol.getOpenWires(wiretol, nullptr, false);

    wjNegatol.addShape(edgesNegatol);
    // Setting atol to a negative value won't have effect and therefore
    // wjNegatol.pimpl->myAngularTol will keep the default value. The tol value must be given in any
    // case.
    wjNegatol.setTolerance(-0.1, -pi);
    wjNegatol.Build();
    wjNegatol.getOpenWires(wireNegatol, nullptr, false);

    wjatol.addShape(edgesatol);
    // Setting atol to a negative value won't have effect and therefore
    // wjNegatol.pimpl->myAngularTol will keep the default value. We give also the tol value so that
    // a closed wire can be created.
    wjatol.setTolerance(0.2, pi / 9);
    wjatol.Build();
    wjatol.getOpenWires(wireatol, nullptr, false);

    // Assert

    // Without calling wjNoBuild.Build() the value of wjNoBuild.IsDone() should be false even if we
    // only changed the value of setTolerance()
    EXPECT_FALSE(wjNoBuild.IsDone());

    // In this case, as there's a gap between edge1 and edge3, no closed wires are created.
    EXPECT_TRUE(wjNegtol.Shape().IsNull());
    // All the edges added with wjNegtol.addShape() can be extracted with wjNegtol.getOpenWires()
    EXPECT_EQ(wireNegtol.getSubTopoShapes(TopAbs_EDGE).size(), 3);

    // In this case, as the gap between edge1 and edge3 is smaller than tol, a closed wire can be
    // created and it contains all the edges added with wjtol.addShape().
    EXPECT_EQ(TopoShape(wjtol.Shape()).getSubTopoShapes(TopAbs_EDGE).size(), 3);
    // There are no open wires and therefore no edges that create them
    EXPECT_EQ(wiretol.getSubTopoShapes(TopAbs_EDGE).size(), 0);

    // In this case, as there's a gap between edge2, edge4 and edge5, no closed wires are created.
    EXPECT_TRUE(wjNegatol.Shape().IsNull());
    // All the edges added with wjNegtol.addShape() can be extracted with wjNegatol.getOpenWires()
    EXPECT_EQ(wireNegatol.getSubTopoShapes(TopAbs_EDGE).size(), 4);

    // In this case, as the gap between edge2, edge4 and edge5 is smaller than tol, a closed wire
    // can be created.
    // Because of atol, edge4 and edge5 are considerated as duplicates and therefore one of them is
    // removed by WireJoiner::WireJoinerP::add().
    // The closed wire is then created using all the edges added with wjatol.addShape() except the
    // removed one
    EXPECT_EQ(TopoShape(wjatol.Shape()).getSubTopoShapes(TopAbs_EDGE).size(), 3);
    // There are no open wires and therefore no edges that create them
    EXPECT_EQ(wireatol.getSubTopoShapes(TopAbs_EDGE).size(), 0);
}

TEST_F(WireJoinerTest, getOpenWires)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.5, 0.5, 0.0), gp_Pnt(1.5, 1.5, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoOpenWires.addShape()
    std::vector<TopoDS_Shape> edgesNoOpenWires {edge1, edge2, edge3};
    // A vector of edges used as argument for wjOriginal.addShape()
    std::vector<TopoDS_Shape> edgesOriginal {edge1, edge2, edge4};
    // A vector of edges used as argument for wjNoOriginal.addShape()
    std::vector<TopoDS_Shape> edgesNoOriginal {edge1, edge2, edge4};
    // A vector of TopoShape edges used as argument for wjNoOp.addShape(). A Tag is needed for every
    // TopoShape, otherwise no element map will be created and no op can be found
    std::vector<TopoShape> edgesNoOp {TopoShape(edge2, 6), TopoShape(edge4, 7)};
    // A vector of TopoShape edges used as argument for wjOp.addShape(). A Tag is needed for every
    // TopoShape, otherwise no element map will be created and no op can be found
    std::vector<TopoShape> edgesOp {TopoShape(edge2, 8), TopoShape(edge4, 9)};

    // A WireJoiner object that will create a closed wire and no open wires
    auto wjNoOpenWires {WireJoiner()};

    // A WireJoiner object where the argument noOriginal will be set to false and that will create
    // an open wire
    auto wjOriginal {WireJoiner()};
    // A WireJoiner object where the argument noOriginal will be set to true and that will create an
    // open wire
    auto wjNoOriginal {WireJoiner()};

    // A WireJoiner object where the argument op won't be set and that will create an open wire
    auto wjNoOp {WireJoiner()};
    // A WireJoiner object where the argument op will be set and that will create an open wire
    auto wjOp {WireJoiner()};

    // An empty TopoShape that will contain the shapes returned by wjNoOpenWires.getOpenWires()
    auto wireNoOpenWires {TopoShape(1)};

    // An empty TopoShape that will contain the shapes returned by wjOriginal.getOpenWires()
    auto wireOriginal {TopoShape(2)};
    // An empty TopoShape that will contain the shapes returned by wjNoOriginal.getOpenWires()
    auto wireNoOriginal {TopoShape(3)};

    // An empty TopoShape that will contain the shapes returned by wjNoOp.getOpenWires()
    auto wireNoOp {TopoShape(4)};
    // An empty TopoShape that will contain the shapes returned by wjOp.getOpenWires()
    auto wireOp {TopoShape(5)};

    // Act

    wjNoOpenWires.addShape(edgesNoOpenWires);
    // wjNoOpenWires.Build() is called by wjNoOpenWires.getOpenWires()
    wjNoOpenWires.getOpenWires(wireNoOpenWires);

    wjOriginal.addShape(edgesOriginal);
    // wjOriginal.Build() is called by wjOriginal.getOpenWires()
    wjOriginal.getOpenWires(wireOriginal, nullptr, false);

    wjNoOriginal.addShape(edgesNoOriginal);
    // wjNoOriginal.Build() is called by wjNoOriginal.getOpenWires()
    wjNoOriginal.getOpenWires(wireNoOriginal);

    wjNoOp.addShape(edgesNoOp);
    // wjNoOp.Build() is called by wjNoOp.getOpenWires()
    wjNoOp.getOpenWires(wireNoOp, nullptr, false);

    wjOp.addShape(edgesOp);
    // wjOp.Build() is called by wjOp.getOpenWires()
    wjOp.getOpenWires(wireOp, "getOpenWires", false);

    // Assert

    // All the edges added with wjNoOpenWires.addShape() are used to create a closed wire, therefore
    // wireNoOpenWires should be null
    EXPECT_TRUE(wireNoOpenWires.isNull());

    // In this case wireOriginal should contain all the edges added with wjOriginal.addShape(),
    // except those ones that are split, and all the edges generated by splitting an edge with
    // another one.
    // edge1 and edge2 are left untouched, while edge4 is split in two at the intersection point
    // (1.0, 1.0, 0.0), therefore 4 edges.
    EXPECT_EQ(wireOriginal.getSubTopoShapes(TopAbs_EDGE).size(), 4);

    // In this case wireNoOriginal should contain only the edges generated by splitting one of them
    // with another one.
    // As edge1 and edge2 are left untouched, the only edges we should find are the ones generated
    // by splitting edge4 in two at the intersection point (1.0, 1.0, 0.0)
    EXPECT_EQ(wireNoOriginal.getSubTopoShapes(TopAbs_EDGE).size(), 2);

    // In this case, as we haven't set a value for op, WireJoiner::WireJoinerP::getOpenWires() will
    // call TopoShape::makeShapeWithElementMap() which, without a value for op, will use
    // Part::OpCodes::Maker as value for the various element maps
    // TODO  no longer works
    //    EXPECT_NE(wireNoOp.getElementMap()[0].name.find(Part::OpCodes::Maker), -1);
    // In this case WireJoiner::WireJoinerP::getOpenWires() will call
    // TopoShape::makeShapeWithElementMap() giving "getOpenWires" as value for the op argument.
    // That value should be found in the various element maps instead of Part::OpCodes::Maker
    EXPECT_EQ(wireOp.getElementMap()[0].name.find(Part::OpCodes::Maker), -1);
    // TODO  no longer works
    //    EXPECT_NE(wireOp.getElementMap()[0].name.find("getOpenWires"), -1);
}

TEST_F(WireJoinerTest, getResultWires)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests
    // Unlike calling WireJoiner::Build(), WireJoiner::getResultWires() returns a shape with edges
    // only if those given as inputs crosses each others

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 0.0, 0.0), gp_Pnt(1.1, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, -0.1, 0.0), gp_Pnt(1.0, 1.1, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(0.1, 0.1, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjNoResultWires.addShape()
    std::vector<TopoDS_Shape> edgesNoResultWires {edge1, edge2, edge3};
    // A vector of TopoShape edges used as argument for wjNoOp.addShape(). A Tag is needed for every
    // TopoShape, otherwise no element map will be created and no op can be found
    std::vector<TopoShape> edgesNoOp {TopoShape(edge1, 4),
                                      TopoShape(edge2, 5),
                                      TopoShape(edge4, 6)};
    // A vector of TopoShape edges used as argument for wjOp.addShape(). A Tag is needed for every
    // TopoShape, otherwise no element map will be created and no op can be found
    std::vector<TopoShape> edgesOp {TopoShape(edge1, 7), TopoShape(edge2, 8), TopoShape(edge4, 9)};

    // A WireJoiner object that will create no closed wires
    auto wjNoResultWires {WireJoiner()};

    // A WireJoiner object where the argument op won't be set and that will create a closed wire
    auto wjNoOp {WireJoiner()};
    // A WireJoiner object where the argument op will be set and that will create a closed wire
    auto wjOp {WireJoiner()};

    // An empty TopoShape that will contain the shapes returned by
    // wireNoResultWires.getResultWires()
    auto wireNoResultWires {TopoShape(1)};

    // An empty TopoShape that will contain the shapes returned by wjNoOp.getResultWires()
    auto wireNoOp {TopoShape(2)};
    // An empty TopoShape that will contain the shapes returned by wjOp.getResultWires()
    auto wireOp {TopoShape(3)};


    // Act

    wjNoResultWires.addShape(edgesNoResultWires);
    // wjNoResultWires.Build() is called by wjNoResultWires.getResultWires()
    wjNoResultWires.getResultWires(wireNoResultWires);

    wjNoOp.addShape(edgesNoOp);
    // wjNoOp.Build() is called by wjNoOp.getResultWires()
    wjNoOp.getResultWires(wireNoOp, nullptr);

    wjOp.addShape(edgesOp);
    // wjOp.Build() is called by wjOp.getResultWires()
    wjOp.getResultWires(wireOp, "getResultWires");

    // Assert

    // All the edges added with wjNoResultWires.addShape() can't create a closed wire, therefore
    // wireNoResultWires shouldn't have any edges
    // It's not possible to get an useful result from wireNoResultWires.isNull() because
    // WireJoiner::WireJoinerP::compound is always created by
    // WireJoiner::WireJoinerP::builder.MakeCompound(), which doesn't create a null compound
    EXPECT_EQ(wireNoResultWires.getSubTopoShapes(TopAbs_EDGE).size(), 0);

    // In this case, as we haven't set a value for op, WireJoiner::WireJoinerP::getResultWires()
    // will call TopoShape::makeShapeWithElementMap() which, without a value for op, will use
    // Part::OpCodes::Maker as value for the various element maps
    EXPECT_NE(wireNoOp.getElementMap()[0].name.find(Part::OpCodes::Maker), -1);

    // In this case WireJoiner::WireJoinerP::getResultWires() will call
    // TopoShape::makeShapeWithElementMap() giving "getResultWires" as value for the op argument.
    // That value should be found in the various element maps instead of Part::OpCodes::Maker
    EXPECT_EQ(wireOp.getElementMap()[0].name.find(Part::OpCodes::Maker), -1);
    EXPECT_NE(wireOp.getElementMap()[0].name.find("getResultWires"), -1);
}

#if OCC_VERSION_HEX >= 0x070600
// WireJoiner::Build() has already been tested indirectly in the other tests.
// Here we check only the difference with OCCT versions >= 7.6.0 that add the parameter theRange
TEST_F(WireJoinerTest, Build)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 0.0, 0.0), gp_Pnt(1.1, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, -0.1, 0.0), gp_Pnt(1.0, 1.1, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjtheRange.addShape()
    std::vector<TopoDS_Shape> edgestheRange {edge1, edge2, edge3};

    // A WireJoiner object that will create no closed wires
    auto wjtheRange {WireJoiner()};

    // A Message_ProgressRange object that will be used as argument for wjtheRange.Build()
    auto mpr {Message_ProgressRange()};

    // Act

    wjtheRange.addShape(edgestheRange);
    wjtheRange.Build(mpr);

    // Assert

    // theRange isn't used in WireJoiner::Build() and therefore not attached to any indicator.
    // For more reference see
    // https://dev.opencascade.org/doc/occt-7.6.0/refman/html/class_message___progress_range.html
    // and
    // https://dev.opencascade.org/doc/occt-7.6.0/refman/html/class_message___progress_indicator.html
    EXPECT_FALSE(mpr.IsActive());
}
#endif

TEST_F(WireJoinerTest, Modified)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 0.0, 0.0), gp_Pnt(1.1, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, -0.1, 0.0), gp_Pnt(1.0, 1.1, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjModified.addShape()
    std::vector<TopoDS_Shape> edges {edge1, edge2, edge3};

    // A WireJoiner object that will have shapes modified by the added edges
    auto wjModified {WireJoiner()};

    // Act

    wjModified.addShape(edges);
    wjModified.Build();

    // Assert

    // In this case, every edge added with wjModified.addShape() modifies other shapes 3 times

    // edge1 modifies the edges with vertexes:
    // (0.0, 0.0, 0.0) - (1.0, 1.0, 0.0)
    // (-0.1, -0.1, 0.0) - (0.0, 0.0, 0.0)
    // (1.0, -0.1, 0.0) - (1.0, 0.0, 0.0)
    EXPECT_EQ(wjModified.Modified(edge1).Size(), 3);

    // edge2 modifies the edges with vertexes:
    // (1.0, 1.0, 0.0) - (1.1, 1.1, 0.0)
    // (0.0, 0.0, 0.0) - (1.0, 0.0, 0.0)
    // (1.0, 0.0, 0.0) - (1.1, 0.0, 0.0)
    EXPECT_EQ(wjModified.Modified(edge2).Size(), 3);

    // edge3 modifies the edges with vertexes:
    // (1.0, 0.0, 0.0) - (1.0, 1.0, 0.0)
    // (1.0, 1.0, 0.0) - (1.0, 1.1, 0.0)
    // (-0.1, 0.0, 0.0) - (0.0, 0.0, 0.0)
    EXPECT_EQ(wjModified.Modified(edge3).Size(), 3);
}

TEST_F(WireJoinerTest, Generated)
{
    // Arrange

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-0.1, 0.0, 0.0), gp_Pnt(1.1, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, -0.1, 0.0), gp_Pnt(1.0, 1.1, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.1, 1.1, 0.0), gp_Pnt(-0.1, -0.1, 0.0)).Edge()};

    // A vector of edges used as argument for wjGenerated.addShape()
    std::vector<TopoDS_Shape> edges {edge1, edge2, edge3};

    // A WireJoiner object that will have shapes generated by the added edges
    auto wjGenerated {WireJoiner()};

    // Act

    wjGenerated.addShape(edges);
    wjGenerated.Build();

    // Assert

    // There aren't calls to WireJoiner::WireJoinerP::aHistory->AddGenerated() or similar methods in
    // WireJoiner::WireJoinerP, therefore nothing is returned by calling
    // WireJoiner::WireJoinerP::aHistory->Generated().
    // There's a call to WireJoiner::WireJoinerP::aHistory->Merge() that uses the history produced
    // by a ShapeFix_Wire object in WireJoiner::WireJoinerP::makeCleanWire() that however looks
    // always empty as no methods in ShapeFix_Wire call AddGenerated() or similar methods
    // (checked in OCCT 7.3.0 source
    // https://git.dev.opencascade.org/gitweb/?p=occt.git;a=snapshot;h=42da0d5115bff683c6b596e66cdeaff957f81e7d;sf=tgz)
    EXPECT_EQ(wjGenerated.Generated(edge1).Size(), 0);
    EXPECT_EQ(wjGenerated.Generated(edge2).Size(), 0);
    EXPECT_EQ(wjGenerated.Generated(edge3).Size(), 0);
}

TEST_F(WireJoinerTest, IsDeleted)
{

    // Create various edges that will be used for the WireJoiner objects tests

    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge4 {BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge()};
    auto edge5 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.49, 0.0, 0.0), gp_Pnt(0.51, 0.0, 0.0)).Edge()};

    // A vector of edges used as argument for wjIsDeleted.addShape()
    std::vector<TopoDS_Shape> edges {edge1, edge2, edge3, edge4, edge5};

    // A WireJoiner object that will have shapes deleted by the added edges
    auto wjIsDeleted {WireJoiner()};
    // To get all the deleted shapes in this case we also need to set the tolerance value
    wjIsDeleted.setTolerance(0.03);

    // Act

    wjIsDeleted.addShape(edges);
    wjIsDeleted.Build();

    // Assert

    // In this case, edge1, edge2 and edge3 don't meet the conditions for deletion
    EXPECT_FALSE(wjIsDeleted.IsDeleted(edge1));
    EXPECT_FALSE(wjIsDeleted.IsDeleted(edge2));
    EXPECT_FALSE(wjIsDeleted.IsDeleted(edge3));

    // edge4 is a duplicate of edge3 and therefore deleted
    EXPECT_TRUE(wjIsDeleted.IsDeleted(edge4));

    // edge5 is smaller that the smallest shape that can be considered with the given value of
    // tolerance and therefore deleted
    EXPECT_TRUE(wjIsDeleted.IsDeleted(edge5));
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
