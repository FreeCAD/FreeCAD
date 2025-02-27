// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class BackwardCompatibilityTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _testPath = App::Application::getHomePath() + "tests/TestModels/";
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc->getName());
    }

    const App::Document* getDocument() const
    {
        return _doc;
    }

    void setDocument(App::Document* doc)
    {
        _doc = doc;
    }

    std::string getTestPath() const
    {
        return _testPath;
    }

private:
    App::Document* _doc = nullptr;
    std::string _testPath;
};

TEST_F(BackwardCompatibilityTest, TestOpenV021Model)
{

    // arrange

    auto doc = App::GetApplication().openDocument(
        std::string(getTestPath() + "ModelFromV021.FCStd").c_str());
    setDocument(doc);

    auto chamfer = dynamic_cast<PartDesign::Chamfer*>(doc->getObject("Chamfer"));
    auto chamferEdgesNames = chamfer->Base.getSubValues();

    std::vector<TopoDS_Shape> chamferOriginalEdges {};
    for (const auto& chamferEdgesName : chamferEdgesNames) {
        chamferOriginalEdges.push_back(
            chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesName.c_str()).getShape());
    }

    // act

    doc->recompute();
    chamfer = dynamic_cast<PartDesign::Chamfer*>(doc->getObject("Chamfer"));
    chamferEdgesNames = chamfer->Base.getSubValues();

    // assert

    auto checkSameVertexes = [](const TopoDS_Shape& e1, const TopoDS_Shape& e2) {
        TopExp_Explorer e1Vertexes(e1, TopAbs_VERTEX);
        TopExp_Explorer e2Vertexes(e2, TopAbs_VERTEX);

        bool sameCoords {true};
        bool moreToCheck {e1Vertexes.More() && e2Vertexes.More()};

        // If one of the vertexes doesn't have the same coordinates of the other one then it'll be
        // useless to continue
        while (moreToCheck && sameCoords) {
            auto p1 = BRep_Tool::Pnt(TopoDS::Vertex(e1Vertexes.Current()));
            auto p2 = BRep_Tool::Pnt(TopoDS::Vertex(e2Vertexes.Current()));

            sameCoords &= (p1.X() == p2.X() && p1.Y() == p2.Y() && p1.Z() == p2.Z());
            e1Vertexes.Next();
            e2Vertexes.Next();

            moreToCheck = (e1Vertexes.More() && e2Vertexes.More());

            // Extra check: both edges should have the same number of vertexes (e*Vertexes.More()
            // should be true or false at the same time for both the edges).
            // If this doesn't happen then one of the edges won't have enough vertexes to perform a
            // full coordinates comparison.
            // This shouldn't happen, so it's here just in case
            sameCoords &= (e1Vertexes.More() == e2Vertexes.More());
        }

        return sameCoords;
    };

    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[0].c_str()).getShape(),
        chamferOriginalEdges[0]));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[1].c_str()).getShape(),
        chamferOriginalEdges[1]));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[2].c_str()).getShape(),
        chamferOriginalEdges[2]));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[3].c_str()).getShape(),
        chamferOriginalEdges[3]));
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
