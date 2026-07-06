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
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/PartDesign/Gui/TaskLoftParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingAdditiveLoftTest: public PartDesign::AdditiveLoft
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingAdditiveLoftTest);

public:
    BlockingAdditiveLoftTest() = default;
    ~BlockingAdditiveLoftTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingLoftState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingAdditiveLoftTest,
    PartDesign::AdditiveLoft,
    blockingLoftState
)

class testTaskLoftParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingAdditiveLoftTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingAdditiveLoftTest::init}));

        PartDesign::BlockingAdditiveLoftTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_loft_dialog");
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

        Part::GeomCircle profileCircle;
        profileCircle.setCenter(Base::Vector3d(2.0, 0.0, 0.0));
        profileCircle.setRadius(1.0);
        profileSketch->addGeometry(&profileCircle, false);

        sectionSketch = doc->addObject<Sketcher::SketchObject>("SectionSketch");
        QVERIFY(sectionSketch != nullptr);
        body->addObject(sectionSketch);
        sectionSketch->AttachmentSupport.setValue(doc->getObject("XZ_Plane"), "");
        sectionSketch->MapMode.setValue("FlatFace");

        Part::GeomCircle sectionCircle;
        sectionCircle.setCenter(Base::Vector3d(0.0, 2.0, 0.0));
        sectionCircle.setRadius(0.75);
        sectionSketch->addGeometry(&sectionCircle, false);

        doc->recompute();

        loft = doc->addObject<PartDesign::BlockingAdditiveLoftTest>("BlockingLoft");
        QVERIFY(loft != nullptr);
        body->addObject(loft);
        loft->Profile.setValue(profileSketch, {""});
        loft->Sections.setSubListValues({{sectionSketch, {""}}});
        loft->Ruled.setValue(false);
        loft->Closed.setValue(false);

        doc->recompute();

        loftView = dynamic_cast<PartDesignGui::ViewProviderLoft*>(guiDoc->getViewProvider(loft));
        QVERIFY(loftView != nullptr);

        guiDoc->openCommand("Edit blocking loft");
        PartDesign::BlockingAdditiveLoftTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingAdditiveLoftTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        loftView = nullptr;
        loft = nullptr;
        sectionSketch = nullptr;
        profileSketch = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void loftRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgLoftParameters(loftView);
        QPointer<PartDesignGui::TaskDlgLoftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskLoftParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* ruled = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxRuled"));
        QVERIFY(ruled != nullptr);

        PartDesign::BlockingAdditiveLoftTest::armBlocker();
        ruled->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditiveLoftTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingAdditiveLoftTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void loftCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgLoftParameters(loftView);
        QPointer<PartDesignGui::TaskDlgLoftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskLoftParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* ruled = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxRuled"));
        QVERIFY(ruled != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingAdditiveLoftTest::armBlocker();
        ruled->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditiveLoftTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingAdditiveLoftTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingAdditiveLoftTest::getExecutionCount(), 1);
    }

    void loftAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgLoftParameters(loftView);
        QPointer<PartDesignGui::TaskDlgLoftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskLoftParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* updateView = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxUpdateView"));
        QVERIFY(updateView != nullptr);
        QVERIFY(updateView->isChecked());

        auto* ruled = taskBox->findChild<QCheckBox*>(QStringLiteral("checkBoxRuled"));
        QVERIFY(ruled != nullptr);

        PartDesign::BlockingAdditiveLoftTest::armBlocker();
        ruled->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingAdditiveLoftTest::getExecutionCount(), 1, 3000);
        QVERIFY2(
            PartDesign::BlockingAdditiveLoftTest::getTotalExecutionCount() <= 2,
            "accept should settle without replaying the queued preview and then rerunning again"
        );
        QVERIFY(taskBox->hasOutstandingRecompute());

        ruled->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingAdditiveLoftTest::releaseBlocker();
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
        QVERIFY2(
            PartDesign::BlockingAdditiveLoftTest::getTotalExecutionCount() <= 2,
            "accept should finish with at most one settled preview and one final recompute"
        );
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
    Sketcher::SketchObject* sectionSketch = nullptr;
    PartDesign::BlockingAdditiveLoftTest* loft = nullptr;
    PartDesignGui::ViewProviderLoft* loftView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskLoftParametersTest(int argc, char** argv)
{
    testTaskLoftParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskLoftParametersTest.moc"
