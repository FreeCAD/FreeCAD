// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QComboBox>
#include <QCoreApplication>
#include <QPointer>
#include <QPushButton>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyContainer.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/PartDesign/Gui/TaskPipeParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderPipe.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingAdditivePipeTest: public PartDesign::AdditivePipe
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingAdditivePipeTest);

public:
    BlockingAdditivePipeTest() = default;
    ~BlockingAdditivePipeTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace PartDesign

namespace
{
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingPipeState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

QComboBox* findTransitionCombo(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<QComboBox*>(QStringLiteral("comboBoxTransition")) : nullptr;
}

QComboBox* findOrientationModeCombo(PartDesignGui::TaskDlgPipeParameters* dialog)
{
    auto* orientationBox = findTaskBox<PartDesignGui::TaskPipeOrientation>(dialog);
    return orientationBox ? orientationBox->findChild<QComboBox*>(QStringLiteral("comboBoxMode"))
                          : nullptr;
}

std::string describeObjectState(const App::Document* doc, const App::DocumentObject* object)
{
    if (!object) {
        return "null object";
    }

    std::string description = object->getStatusString();
    if (doc) {
        const char* error = doc->getErrorDescription(object);
        if (error && *error) {
            description += ": ";
            description += error;
        }
    }
    return description;
}

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingAdditivePipeTest,
    PartDesign::AdditivePipe,
    blockingPipeState
)

class testTaskPipeParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingAdditivePipeTest::init}));

        asyncParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Document"
        );
        oldAsyncEnabled = asyncParams->GetBool("EnableAsyncRecompute", true);
        asyncParams->SetBool("EnableAsyncRecompute", true);

        gizmoParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Gui/Gizmos"
        );
        oldGizmosEnabled = gizmoParams->GetBool("EnableGizmos", true);
        gizmoParams->SetBool("EnableGizmos", false);
    }

    void cleanupTestCase()  // NOLINT
    {
        if (gizmoParams) {
            gizmoParams->SetBool("EnableGizmos", oldGizmosEnabled);
            gizmoParams = nullptr;
        }
        if (asyncParams) {
            asyncParams->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            asyncParams = nullptr;
        }
    }

    void init()  // NOLINT
    {
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingAdditivePipeTest::init}));

        PartDesign::BlockingAdditivePipeTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_pipe_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);

        profileSketch = doc->addObject<Sketcher::SketchObject>("ProfileSketch");
        QVERIFY(profileSketch != nullptr);
        body->addObject(profileSketch);
        profileSketch->AttachmentSupport.setValue(doc->getObject("XY_Plane"), "");
        profileSketch->MapMode.setValue("FlatFace");

        Base::Interpreter().runString(
            ("import FreeCAD as App\n"
             "import Part\n"
             "import Sketcher\n"
             "doc = App.getDocument('"
             + docName
             + "')\n"
               "profile = doc.getObject('ProfileSketch')\n"
               "profile.addGeometry(Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 1), "
               "False)\n"
               "profile.addConstraint(Sketcher.Constraint('Radius', 0, 1))\n"
               "profile.addConstraint(Sketcher.Constraint('DistanceX', 0, 3, 0))\n"
               "profile.addConstraint(Sketcher.Constraint('DistanceY', 0, 3, 0))\n")
                .c_str()
        );

        doc->recompute();
        QVERIFY2(profileSketch->isValid(), describeObjectState(doc, profileSketch).c_str());
        QVERIFY(!profileSketch->Shape.getValue().IsNull());

        spineSketch = doc->addObject<Sketcher::SketchObject>("SpineSketch");
        QVERIFY(spineSketch != nullptr);
        body->addObject(spineSketch);
        spineSketch->AttachmentSupport.setValue(doc->getObject("XZ_Plane"), "");
        spineSketch->MapMode.setValue("FlatFace");

        doc->recompute();
        QVERIFY2(spineSketch->isValid(), describeObjectState(doc, spineSketch).c_str());

        Base::Interpreter().runString(
            ("import FreeCAD as App\n"
             "import Part\n"
             "import Sketcher\n"
             "doc = App.getDocument('"
             + docName
             + "')\n"
               "spine = doc.getObject('SpineSketch')\n"
               "spine.addGeometry(Part.LineSegment(App.Vector(0.0, 0.0, 0), App.Vector(0, 1, 0)), "
               "False)\n"
               "spine.addConstraint(Sketcher.Constraint('Coincident', 0, 1, -1, 1))\n"
               "spine.addConstraint(Sketcher.Constraint('PointOnObject', 0, 2, -2))\n"
               "spine.addConstraint(Sketcher.Constraint('DistanceY', 0, 1, 0, 2, 1))\n")
                .c_str()
        );

        doc->recompute();
        QVERIFY2(spineSketch->isValid(), describeObjectState(doc, spineSketch).c_str());
        QVERIFY(!spineSketch->Shape.getValue().IsNull());

        pipe = doc->addObject<PartDesign::BlockingAdditivePipeTest>("BlockingPipe");
        QVERIFY(pipe != nullptr);
        body->addObject(pipe);
        pipe->Profile.setValue(profileSketch);
        pipe->Spine.setValue(spineSketch);
        pipe->Transition.setValue(0L);

        doc->recompute();
        QVERIFY2(pipe->isValid(), describeObjectState(doc, pipe).c_str());

        pipeView = dynamic_cast<PartDesignGui::ViewProviderPipe*>(guiDoc->getViewProvider(pipe));
        QVERIFY(pipeView != nullptr);

        guiDoc->openCommand("Edit blocking pipe");
        PartDesign::BlockingAdditivePipeTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingAdditivePipeTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        pipeView = nullptr;
        pipe = nullptr;
        spineSketch = nullptr;
        profileSketch = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void pipeRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPipeParameters(pipeView);
        QPointer<PartDesignGui::TaskDlgPipeParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPipeParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* transition = findTransitionCombo(taskBox);
        QVERIFY(transition != nullptr);
        QVERIFY(transition->count() >= 2);

        PartDesign::BlockingAdditivePipeTest::armBlocker();
        transition->setCurrentIndex(1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditivePipeTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingAdditivePipeTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void pipeCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPipeParameters(pipeView);
        QPointer<PartDesignGui::TaskDlgPipeParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPipeParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* transition = findTransitionCombo(taskBox);
        QVERIFY(transition != nullptr);
        QVERIFY(transition->count() >= 2);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingAdditivePipeTest::armBlocker();
        transition->setCurrentIndex(1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditivePipeTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingAdditivePipeTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingAdditivePipeTest::getExecutionCount(), 1);
    }

    void pipePreviewModeRoundTripRestoresValidity()  // NOLINT
    {
        QVERIFY2(pipe->isValid(), pipe->getStatusString());

        auto* dialog = new PartDesignGui::TaskDlgPipeParameters(pipeView);
        QPointer<PartDesignGui::TaskDlgPipeParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPipeParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        auto* mode = findOrientationModeCombo(dialog);
        QVERIFY(mode != nullptr);
        QVERIFY(mode->count() >= 2);

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY2(pipe->isValid(), pipe->getStatusString());

        mode->setCurrentIndex(1);
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY2(pipe->isValid(), pipe->getStatusString());

        mode->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY2(pipe->isValid(), pipe->getStatusString());

        Gui::Control().reject(doc);
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
    }

    void pipeAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPipeParameters(pipeView);
        QPointer<PartDesignGui::TaskDlgPipeParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPipeParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* transition = findTransitionCombo(taskBox);
        QVERIFY(transition != nullptr);
        QVERIFY(transition->count() >= 2);

        PartDesign::BlockingAdditivePipeTest::armBlocker();
        transition->setCurrentIndex(1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditivePipeTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingAdditivePipeTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        transition->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingAdditivePipeTest::releaseBlocker();
        });

        std::string acceptError;
        try {
            Gui::Control().accept(doc);
        }
        catch (const Base::Exception& e) {
            acceptError = e.what();
        }
        catch (const std::exception& e) {
            acceptError = e.what();
        }
        catch (...) {
            acceptError = "unknown exception";
        }
        releaser.join();
        QVERIFY2(acceptError.empty(), acceptError.c_str());

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(PartDesign::BlockingAdditivePipeTest::getTotalExecutionCount(), 2);
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    Base::Reference<ParameterGrp> gizmoParams;
    bool oldGizmosEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::Body* body = nullptr;
    Sketcher::SketchObject* profileSketch = nullptr;
    Sketcher::SketchObject* spineSketch = nullptr;
    PartDesign::BlockingAdditivePipeTest* pipe = nullptr;
    PartDesignGui::ViewProviderPipe* pipeView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskPipeParametersTest(int argc, char** argv)
{
    testTaskPipeParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskPipeParametersTest.moc"
