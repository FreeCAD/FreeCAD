// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QCheckBox>
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
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskPadParameters.h>
#include <Mod/PartDesign/Gui/TaskPocketParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderPad.h>
#include <Mod/PartDesign/Gui/ViewProviderPocket.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingPadTest: public PartDesign::Pad
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingPadTest);

public:
    BlockingPadTest() = default;
    ~BlockingPadTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingPocketTest: public PartDesign::Pocket
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingPocketTest);

public:
    BlockingPocketTest() = default;
    ~BlockingPocketTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingPadState)
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingPocketState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(PartDesign::BlockingPadTest, PartDesign::Pad, blockingPadState)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingPocketTest,
    PartDesign::Pocket,
    blockingPocketState
)

class testTaskExtrudeParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness(
            {&PartDesign::BlockingPadTest::init, &PartDesign::BlockingPocketTest::init}
        ));

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
        Q_UNUSED(ensureGuiHarness(
            {&PartDesign::BlockingPadTest::init, &PartDesign::BlockingPocketTest::init}
        ));

        PartDesign::BlockingPadTest::resetBlocker();
        PartDesign::BlockingPocketTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_pad_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);

        sketch = doc->addObject<Sketcher::SketchObject>("Sketch");
        QVERIFY(sketch != nullptr);
        body->addObject(sketch);
        sketch->AttachmentSupport.setValue(doc->getObject("XY_Plane"), "");
        sketch->MapMode.setValue("FlatFace");

        Part::GeomCircle circle;
        circle.setCenter(Base::Vector3d(2.0, 0.0, 0.0));
        circle.setRadius(1.0);
        sketch->addGeometry(&circle, false);

        doc->recompute();

        pad = doc->addObject<PartDesign::BlockingPadTest>("BlockingPad");
        QVERIFY(pad != nullptr);
        body->addObject(pad);
        pad->Profile.setValue(sketch, {""});
        pad->Type.setValue("Length");
        pad->SideType.setValue("One side");
        pad->Length.setValue(6.0);
        pad->Reversed.setValue(false);
        pad->Midplane.setValue(false);
        pad->AlongSketchNormal.setValue(true);

        doc->recompute();

        padView = dynamic_cast<PartDesignGui::ViewProviderPad*>(guiDoc->getViewProvider(pad));
        QVERIFY(padView != nullptr);

        pocketBody = doc->addObject<PartDesign::Body>("PocketBody");
        QVERIFY(pocketBody != nullptr);

        pocketBaseBox = doc->addObject<PartDesign::AdditiveBox>("PocketBaseBox");
        QVERIFY(pocketBaseBox != nullptr);
        pocketBody->addObject(pocketBaseBox);
        pocketBaseBox->Length.setValue(10.0);
        pocketBaseBox->Width.setValue(10.0);
        pocketBaseBox->Height.setValue(10.0);

        doc->recompute();

        pocketSketch = doc->addObject<Sketcher::SketchObject>("PocketSketch");
        QVERIFY(pocketSketch != nullptr);
        pocketSketch->AttachmentSupport.setValue(pocketBaseBox, "Face6");
        pocketSketch->MapMode.setValue("FlatFace");
        pocketSketch->MapReversed.setValue(true);
        pocketBody->addObject(pocketSketch);

        Part::GeomCircle pocketCircle;
        pocketCircle.setCenter(Base::Vector3d(2.0, 2.0, 0.0));
        pocketCircle.setRadius(1.0);
        pocketSketch->addGeometry(&pocketCircle, false);

        pocket = doc->addObject<PartDesign::BlockingPocketTest>("BlockingPocket");
        QVERIFY(pocket != nullptr);
        pocketBody->addObject(pocket);
        pocket->Profile.setValue(pocketSketch, {""});
        pocket->Type.setValue("Length");
        pocket->SideType.setValue("One side");
        pocket->Length.setValue(6.0);
        pocket->Reversed.setValue(false);
        pocket->Midplane.setValue(false);
        pocket->AlongSketchNormal.setValue(true);

        doc->recompute();

        pocketView = dynamic_cast<PartDesignGui::ViewProviderPocket*>(guiDoc->getViewProvider(pocket));
        QVERIFY(pocketView != nullptr);

        guiDoc->openCommand("Edit blocking extrude");
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingPadTest::releaseBlocker();
        PartDesign::BlockingPocketTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        padView = nullptr;
        pad = nullptr;
        pocketView = nullptr;
        pocket = nullptr;
        pocketSketch = nullptr;
        pocketBaseBox = nullptr;
        pocketBody = nullptr;
        sketch = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void padRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPadParameters(padView);
        QPointer<PartDesignGui::TaskDlgPadParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPadParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingPadTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPadTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingPadTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void padCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPadParameters(padView);
        QPointer<PartDesignGui::TaskDlgPadParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPadParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingPadTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPadTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingPadTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingPadTest::getExecutionCount(), 1);
    }

    void padAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPadParameters(padView);
        QPointer<PartDesignGui::TaskDlgPadParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPadParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingPadTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPadTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingPadTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        reversed->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingPadTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(PartDesign::BlockingPadTest::getTotalExecutionCount(), 2);
    }

    void pocketRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPocketParameters(pocketView);
        QPointer<PartDesignGui::TaskDlgPocketParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPocketParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingPocketTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPocketTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingPocketTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void pocketCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPocketParameters(pocketView);
        QPointer<PartDesignGui::TaskDlgPocketParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPocketParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingPocketTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPocketTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingPocketTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingPocketTest::getExecutionCount(), 1);
    }

    void pocketAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPocketParameters(pocketView);
        QPointer<PartDesignGui::TaskDlgPocketParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPocketParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingPocketTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPocketTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingPocketTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        reversed->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingPocketTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(PartDesign::BlockingPocketTest::getTotalExecutionCount(), 2);
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
    Sketcher::SketchObject* sketch = nullptr;
    PartDesign::BlockingPadTest* pad = nullptr;
    PartDesignGui::ViewProviderPad* padView = nullptr;
    PartDesign::Body* pocketBody = nullptr;
    PartDesign::AdditiveBox* pocketBaseBox = nullptr;
    Sketcher::SketchObject* pocketSketch = nullptr;
    PartDesign::BlockingPocketTest* pocket = nullptr;
    PartDesignGui::ViewProviderPocket* pocketView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskExtrudeParametersTest(int argc, char** argv)
{
    testTaskExtrudeParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskExtrudeParametersTest.moc"
