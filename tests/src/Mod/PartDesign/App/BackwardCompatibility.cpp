// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>
#include <Mod/PartDesign/App/FeaturePad.h>

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
        std::string(getTestPath() + "ModelFromV021.FCStd").c_str()
    );
    setDocument(doc);

    auto chamfer = dynamic_cast<PartDesign::Chamfer*>(doc->getObject("Chamfer"));
    auto chamferEdgesNames = chamfer->Base.getSubValues();

    std::vector<TopoDS_Shape> chamferOriginalEdges {};
    for (const auto& chamferEdgesName : chamferEdgesNames) {
        chamferOriginalEdges.push_back(
            chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesName.c_str()).getShape()
        );
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
        chamferOriginalEdges[0]
    ));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[1].c_str()).getShape(),
        chamferOriginalEdges[1]
    ));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[2].c_str()).getShape(),
        chamferOriginalEdges[2]
    ));
    EXPECT_TRUE(checkSameVertexes(
        chamfer->getBaseTopoShape().getSubTopoShape(chamferEdgesNames[3].c_str()).getShape(),
        chamferOriginalEdges[3]
    ));
}

TEST_F(BackwardCompatibilityTest, TestTwoLengthsPadWithExpression)
{
    // Regression test for https://github.com/FreeCAD/FreeCAD/issues/28690 -- a v1.0.2 file with a
    // TwoLengths pad where Length has an expression (=10mm) and Length2 is a plain value (5mm). The
    // migration must swap both the values and the expressions so the geometry is preserved.

    auto doc = App::GetApplication().openDocument(
        std::string(getTestPath() + "TwoLengthsPadWithExpression.FCStd").c_str()
    );
    setDocument(doc);

    auto pad = dynamic_cast<PartDesign::Pad*>(doc->getObject("Pad"));
    ASSERT_NE(pad, nullptr);

    EXPECT_DOUBLE_EQ(pad->Length.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(pad->Length2.getValue(), 10.0);

    App::ObjectIdentifier lengthPath(pad->Length);
    App::ObjectIdentifier length2Path(pad->Length2);
    auto exprLength = pad->getExpression(lengthPath);
    auto exprLength2 = pad->getExpression(length2Path);
    EXPECT_FALSE(exprLength.expression);
    EXPECT_TRUE(exprLength2.expression);

    doc->recompute();
    auto bbox = pad->Shape.getBoundingBox();
    EXPECT_NEAR(bbox.MaxZ, 10.0, 0.01);
    EXPECT_NEAR(bbox.MinZ, -5.0, 0.01);
}

TEST_F(BackwardCompatibilityTest, TestTwoLengthsPadCyclicExpression)
{
    // Regression test for https://github.com/FreeCAD/FreeCAD/issues/29233 -- a v1.0 file where
    // Length2 has expression "Length / 2". Without the fix this becomes cyclic on migrate and is
    // silently discarded.

    auto doc = App::GetApplication().openDocument(
        std::string(getTestPath() + "TwoLengthsPadCyclicExpr.FCStd").c_str()
    );
    setDocument(doc);

    auto pad = dynamic_cast<PartDesign::Pad*>(doc->getObject("Pad"));
    ASSERT_NE(pad, nullptr);

    EXPECT_DOUBLE_EQ(pad->Length.getValue(), 5.0);
    EXPECT_DOUBLE_EQ(pad->Length2.getValue(), 10.0);

    App::ObjectIdentifier lengthPath(pad->Length);
    App::ObjectIdentifier length2Path(pad->Length2);
    auto exprLength = pad->getExpression(lengthPath);
    auto exprLength2 = pad->getExpression(length2Path);
    ASSERT_TRUE(exprLength.expression);  // "Length2 / 2" -- must not be discarded
    // The original expression was "Length / 2" on Length2. After renaming Length<->Length2,
    // it becomes "Length2 / 2" on Length — which is correct and not cyclic.
    std::string exprStr = exprLength.expression->toString();
    EXPECT_NE(exprStr.find("Length2"), std::string::npos)
        << "Expression on Length should reference Length2 after rename: " << exprStr;
    EXPECT_FALSE(exprLength2.expression);
}

TEST_F(BackwardCompatibilityTest, TestTwoLengthsPadCrossObjectRefExpression)
{
    // Regression test for https://github.com/FreeCAD/FreeCAD/issues/29233 -- a v1.0 file where
    // Sketch001.AttachmentOffset references Pad.Length2. After migration the reference must be
    // renamed to Pad.Length so it still points to the correct value.

    auto doc = App::GetApplication().openDocument(
        std::string(getTestPath() + "TwoLengthsPadCrossObjectRef.FCStd").c_str()
    );
    setDocument(doc);

    auto pad = dynamic_cast<PartDesign::Pad*>(doc->getObject("Pad"));
    ASSERT_NE(pad, nullptr);

    EXPECT_DOUBLE_EQ(pad->Length.getValue(), 8.0);
    EXPECT_DOUBLE_EQ(pad->Length2.getValue(), 10.0);

    auto sketch = doc->getObject("Sketch001");
    ASSERT_NE(sketch, nullptr);

    // AttachmentOffset.Base.z must reference Pad.Length (= -8mm), not Pad.Length2 (= -10mm).
    auto exprs = sketch->ExpressionEngine.getExpressions();
    const App::Expression* attachExpr = nullptr;
    for (const auto& [path, expr] : exprs) {
        if (path.toString().find("AttachmentOffset") != std::string::npos) {
            attachExpr = expr;
            break;
        }
    }
    ASSERT_NE(attachExpr, nullptr);
    std::string exprStr = attachExpr->toString();
    EXPECT_EQ(exprStr.find("Length2"), std::string::npos)
        << "Expression still references Length2 after migration: " << exprStr;
    EXPECT_NE(exprStr.find("Length"), std::string::npos)
        << "Expression does not reference Length after migration: " << exprStr;

    doc->recompute();
    auto* placement = dynamic_cast<App::PropertyPlacement*>(
        sketch->getPropertyByName("AttachmentOffset")
    );
    ASSERT_NE(placement, nullptr);
    EXPECT_NEAR(placement->getValue().getPosition().z, -8.0, 0.01);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
