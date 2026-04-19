// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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
#include <Gui/QuantitySpinBox.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskChamferParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderChamfer.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingChamferTest: public PartDesign::Chamfer
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingChamferTest);

public:
    BlockingChamferTest() = default;
    ~BlockingChamferTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingChamferState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

Gui::QuantitySpinBox* findSizeSpinBox(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<Gui::QuantitySpinBox*>(QStringLiteral("chamferSize"))
                   : nullptr;
}

std::vector<std::string> allBoxFaces()
{
    return {"Face1", "Face2", "Face3", "Face4", "Face5", "Face6"};
}

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingChamferTest,
    PartDesign::Chamfer,
    blockingChamferState
)

class testTaskChamferParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingChamferTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingChamferTest::init}));

        PartDesign::BlockingChamferTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_chamfer_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);

        baseBox = doc->addObject<PartDesign::AdditiveBox>("BaseBox");
        QVERIFY(baseBox != nullptr);
        body->addObject(baseBox);
        baseBox->Length.setValue(10.0);
        baseBox->Width.setValue(10.0);
        baseBox->Height.setValue(10.0);

        doc->recompute();

        chamfer = doc->addObject<PartDesign::BlockingChamferTest>("BlockingChamfer");
        QVERIFY(chamfer != nullptr);
        body->addObject(chamfer);
        chamfer->Base.setValue(baseBox, allBoxFaces());
        chamfer->ChamferType.setValue(0L);
        chamfer->Size.setValue(1.0);
        chamfer->UseAllEdges.setValue(false);

        doc->recompute();

        chamferView = dynamic_cast<PartDesignGui::ViewProviderChamfer*>(
            guiDoc->getViewProvider(chamfer)
        );
        QVERIFY(chamferView != nullptr);

        guiDoc->openCommand("Edit blocking chamfer");
        PartDesign::BlockingChamferTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingChamferTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        chamferView = nullptr;
        chamfer = nullptr;
        baseBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void chamferRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgChamferParameters(chamferView);
        QPointer<PartDesignGui::TaskDlgChamferParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskChamferParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* size = findSizeSpinBox(taskBox);
        QVERIFY(size != nullptr);

        PartDesign::BlockingChamferTest::armBlocker();
        size->setValue(size->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingChamferTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingChamferTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void chamferCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgChamferParameters(chamferView);
        QPointer<PartDesignGui::TaskDlgChamferParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskChamferParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* size = findSizeSpinBox(taskBox);
        QVERIFY(size != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingChamferTest::armBlocker();
        size->setValue(size->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingChamferTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingChamferTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingChamferTest::getExecutionCount(), 1);
    }

    void chamferAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgChamferParameters(chamferView);
        QPointer<PartDesignGui::TaskDlgChamferParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskChamferParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* size = findSizeSpinBox(taskBox);
        QVERIFY(size != nullptr);

        PartDesign::BlockingChamferTest::armBlocker();
        size->setValue(size->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingChamferTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingChamferTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        size->setValue(size->rawValue() + 0.5);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingChamferTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingChamferTest::getTotalExecutionCount(), 2);
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
    PartDesign::AdditiveBox* baseBox = nullptr;
    PartDesign::BlockingChamferTest* chamfer = nullptr;
    PartDesignGui::ViewProviderChamfer* chamferView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskChamferParametersTest(int argc, char** argv)
{
    testTaskChamferParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskChamferParametersTest.moc"
