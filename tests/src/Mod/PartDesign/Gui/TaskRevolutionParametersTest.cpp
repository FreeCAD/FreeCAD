// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
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
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/Gui/TaskRevolutionParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderGroove.h>
#include <Mod/PartDesign/Gui/ViewProviderRevolution.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingRevolutionTest: public PartDesign::Revolution
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingRevolutionTest);

public:
    BlockingRevolutionTest() = default;
    ~BlockingRevolutionTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingGrooveTest: public PartDesign::Groove
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingGrooveTest);

public:
    BlockingGrooveTest() = default;
    ~BlockingGrooveTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingRevolutionState)
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingGrooveState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingRevolutionTest,
    PartDesign::Revolution,
    blockingRevolutionState
)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingGrooveTest,
    PartDesign::Groove,
    blockingGrooveState
)

class testTaskRevolutionParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness(
            {&PartDesign::BlockingRevolutionTest::init, &PartDesign::BlockingGrooveTest::init}
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
            {&PartDesign::BlockingRevolutionTest::init, &PartDesign::BlockingGrooveTest::init}
        ));

        PartDesign::BlockingRevolutionTest::resetBlocker();
        PartDesign::BlockingGrooveTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_revolution_dialog");
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

        revolution = doc->addObject<PartDesign::BlockingRevolutionTest>("BlockingRevolution");
        QVERIFY(revolution != nullptr);
        body->addObject(revolution);
        revolution->Profile.setValue(sketch, {""});
        revolution->ReferenceAxis.setValue(sketch, {"V_Axis"});
        revolution->Type.setValue("Angle");
        revolution->Angle.setValue(180.0);
        revolution->Angle2.setValue(0.0);
        revolution->Midplane.setValue(false);
        revolution->Reversed.setValue(false);

        doc->recompute();

        revolutionView = dynamic_cast<PartDesignGui::ViewProviderRevolution*>(
            guiDoc->getViewProvider(revolution)
        );
        QVERIFY(revolutionView != nullptr);

        grooveBody = doc->addObject<PartDesign::Body>("GrooveBody");
        QVERIFY(grooveBody != nullptr);

        grooveBaseBox = doc->addObject<PartDesign::AdditiveBox>("GrooveBaseBox");
        QVERIFY(grooveBaseBox != nullptr);
        grooveBody->addObject(grooveBaseBox);
        grooveBaseBox->Length.setValue(10.0);
        grooveBaseBox->Width.setValue(10.0);
        grooveBaseBox->Height.setValue(10.0);

        doc->recompute();

        groove = doc->addObject<PartDesign::BlockingGrooveTest>("BlockingGroove");
        QVERIFY(groove != nullptr);
        grooveBody->addObject(groove);
        groove->Profile.setValue(grooveBaseBox, {"Face6"});
        groove->ReferenceAxis.setValue(doc->getObject("X_Axis"), {""});
        groove->Type.setValue("Angle");
        groove->Angle.setValue(180.0);
        groove->Angle2.setValue(0.0);
        groove->Midplane.setValue(false);
        groove->Reversed.setValue(true);

        doc->recompute();

        grooveView = dynamic_cast<PartDesignGui::ViewProviderGroove*>(guiDoc->getViewProvider(groove));
        QVERIFY(grooveView != nullptr);

        guiDoc->openCommand("Edit blocking revolution");
        PartDesign::BlockingRevolutionTest::resetBlocker();
        PartDesign::BlockingGrooveTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingRevolutionTest::releaseBlocker();
        PartDesign::BlockingGrooveTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        revolutionView = nullptr;
        revolution = nullptr;
        grooveView = nullptr;
        groove = nullptr;
        grooveBaseBox = nullptr;
        grooveBody = nullptr;
        sketch = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void revolutionRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgRevolutionParameters(revolutionView);
        QPointer<PartDesignGui::TaskDlgRevolutionParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingRevolutionTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingRevolutionTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingRevolutionTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void revolutionCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgRevolutionParameters(revolutionView);
        QPointer<PartDesignGui::TaskDlgRevolutionParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingRevolutionTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingRevolutionTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingRevolutionTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingRevolutionTest::getExecutionCount(), 1);
    }

    void revolutionAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgRevolutionParameters(revolutionView);
        QPointer<PartDesignGui::TaskDlgRevolutionParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingRevolutionTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingRevolutionTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingRevolutionTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        reversed->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingRevolutionTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingRevolutionTest::getTotalExecutionCount(), 2);
    }

    void grooveRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgGrooveParameters(grooveView);
        QPointer<PartDesignGui::TaskDlgGrooveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingGrooveTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingGrooveTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingGrooveTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void grooveCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgGrooveParameters(grooveView);
        QPointer<PartDesignGui::TaskDlgGrooveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingGrooveTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingGrooveTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingGrooveTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingGrooveTest::getExecutionCount(), 1);
    }

    void grooveAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgGrooveParameters(grooveView);
        QPointer<PartDesignGui::TaskDlgGrooveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskRevolutionParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxReversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingGrooveTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingGrooveTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingGrooveTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        reversed->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingGrooveTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingGrooveTest::getTotalExecutionCount(), 2);
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
    PartDesign::BlockingRevolutionTest* revolution = nullptr;
    PartDesignGui::ViewProviderRevolution* revolutionView = nullptr;
    PartDesign::Body* grooveBody = nullptr;
    PartDesign::AdditiveBox* grooveBaseBox = nullptr;
    PartDesign::BlockingGrooveTest* groove = nullptr;
    PartDesignGui::ViewProviderGroove* grooveView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskRevolutionParametersTest(int argc, char** argv)
{
    testTaskRevolutionParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskRevolutionParametersTest.moc"
