// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include <QApplication>

#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Mod/Measure/App/MeasureDistance.h>
#include <Mod/Measure/Gui/ViewProviderMeasureDistance.h>
#include <Mod/Part/App/PartFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include <Base/Interpreter.h>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <gp_Pnt.hxx>

#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>

namespace
{

// GUI test fixture for `Measure::MeasureDistance`.
// Verifies annotation geometry (arrowheads and lines) for two-point measurements.
class MeasureDistanceGui: public ::testing::Test
{
protected:
    static QApplication* qapp;
    static Gui::Application* guiApp;

    static void SetUpTestSuite()
    {
        tests::initApplication();

        // Use offscreen platform when no display is available (e.g. CI).
        // Qt aborts with SIGABRT if QApplication is created without a display
        // and QT_QPA_PLATFORM is unset.
        if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }

        static int argc = 1;
        static char programName[] = "Gui_tests_run";
        static char* argv[] = {programName};
        qapp = new QApplication(argc, argv);

        guiApp = new Gui::Application(true);
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        Base::Interpreter().loadModule("MeasureGui");
    }

    static void TearDownTestSuite()
    {
        delete guiApp;
        guiApp = nullptr;

        delete qapp;
        qapp = nullptr;
    }
};

QApplication* MeasureDistanceGui::qapp = nullptr;
Gui::Application* MeasureDistanceGui::guiApp = nullptr;

// Walks the scenegraph to count SoCallback nodes (used for GL arrowhead rendering)
void countCallbacks(const SoNode* node, int& count)
{
    if (!node) {
        return;
    }

    if (dynamic_cast<const SoCallback*>(node)) {
        ++count;
        return;
    }

    if (const auto* group = dynamic_cast<const SoGroup*>(node)) {
        for (int i = 0; i < group->getNumChildren(); ++i) {
            countCallbacks(group->getChild(i), count);
        }
    }
}

// Walks the scenegraph to count line segment nodes in the annotation
void countIndexedLineSets(const SoNode* node, int& lineCount)
{
    if (!node) {
        return;
    }

    if (dynamic_cast<const SoIndexedLineSet*>(node)) {
        ++lineCount;
        return;
    }

    if (const auto* group = dynamic_cast<const SoGroup*>(node)) {
        for (int i = 0; i < group->getNumChildren(); ++i) {
            countIndexedLineSets(group->getChild(i), lineCount);
        }
    }
}

// Creates a point feature at the given coordinate for distance measurement
Part::Feature* makePointFeature(App::Document* doc, const char* name, const gp_Pnt& point)
{
    auto* feature = doc->addObject<Part::Feature>(name);
    feature->Shape.setValue(BRepBuilderAPI_MakeVertex(point).Vertex());
    return feature;
}

}  // namespace

// Validates that arrowheads and lines are rendered for a two-point distance measurement
TEST_F(MeasureDistanceGui, arrowsAreInstantiatedForTwoPoints)
{
    App::DocumentInitFlags flags;
    flags.createView = false;

    App::Document* doc = App::GetApplication().newDocument("MeasureDistanceGui", nullptr, flags);
    ASSERT_NE(doc, nullptr);

    auto* point1 = makePointFeature(doc, "Point1", gp_Pnt(0.0, 0.0, 0.0));
    auto* point2 = makePointFeature(doc, "Point2", gp_Pnt(20.0, 0.0, 0.0));
    ASSERT_NE(point1, nullptr);
    ASSERT_NE(point2, nullptr);

    doc->recompute();

    App::MeasureSelection selection;
    selection.push_back({App::SubObjectT {point1, ""}, Base::Vector3d(0.0, 0.0, 0.0)});
    selection.push_back({App::SubObjectT {point2, ""}, Base::Vector3d(20.0, 0.0, 0.0)});

    ASSERT_TRUE(Measure::MeasureDistance::isValidSelection(selection));

    auto measureTypes = App::MeasureManager::getValidMeasureTypes(selection, "");
    ASSERT_FALSE(measureTypes.empty());
    EXPECT_EQ(measureTypes.front()->measureObject, "Measure::MeasureDistance");

    auto* measure = doc->addObject<Measure::MeasureDistance>("Distance");
    ASSERT_NE(measure, nullptr);

    measure->parseSelection(selection);
    doc->recompute();

    Gui::Document* guiDoc = guiApp->getDocument(doc);
    ASSERT_NE(guiDoc, nullptr);

    auto* viewProvider = guiApp->getViewProvider<MeasureGui::ViewProviderMeasureDistance>(measure);
    ASSERT_NE(viewProvider, nullptr);

    viewProvider->redrawAnnotation();

    SoSeparator* root = viewProvider->getRoot();
    ASSERT_NE(root, nullptr);

    // Arrowheads are drawn via a GL render callback rather than cone scene-graph nodes.
    EXPECT_TRUE(viewProvider->ShowArrows.getValue());
    int callbackCount = 0;
    countCallbacks(root, callbackCount);
    EXPECT_GE(callbackCount, 1);

    int lineCount = 0;
    countIndexedLineSets(root, lineCount);
    EXPECT_GE(lineCount, 2);

    App::GetApplication().closeDocument(doc->getName());
}
