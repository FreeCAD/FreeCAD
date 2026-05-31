// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QApplication>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QTest>
#include <QTimer>

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
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Gui/ViewProviderSuppressibleExtension.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/Part/Gui/ViewProviderPreviewExtension.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskPrimitiveParameters.h>
#include <Mod/PartDesign/Gui/ViewProvider.h>
#include <Mod/PartDesign/Gui/ViewProviderPrimitive.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingBoxTest: public PartDesign::Box
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingBoxTest);

public:
    BlockingBoxTest() = default;
    ~BlockingBoxTest() override = default;

    static void resetBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace PartDesign

namespace
{
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingBoxState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;

PartDesignGui::TaskBoxPrimitives* findPrimitiveTaskBox(Gui::TaskView::TaskDialog* dialog)
{
    return tests::partdesigngui::findTaskBox<PartDesignGui::TaskBoxPrimitives>(dialog);
}

QDialogButtonBox* findActiveDialogButtonBox()
{
    auto* taskView = Gui::Control().taskPanel();
    return taskView ? taskView->findChild<QDialogButtonBox*>() : nullptr;
}

Gui::QuantitySpinBox* findBoxLengthSpinBox(PartDesignGui::TaskBoxPrimitives* taskBox)
{
    return taskBox ? taskBox->findChild<Gui::QuantitySpinBox*>(QStringLiteral("boxLength")) : nullptr;
}

}  // namespace

PROPERTY_SOURCE(PartDesign::BlockingBoxTest, PartDesign::Box)

void PartDesign::BlockingBoxTest::resetBlocker()
{
    blockingBoxState().arm();
}

int PartDesign::BlockingBoxTest::getExecutionCount()
{
    return blockingBoxState().getExecutionCount();
}

int PartDesign::BlockingBoxTest::getTotalExecutionCount()
{
    return blockingBoxState().getTotalExecutionCount();
}

void PartDesign::BlockingBoxTest::releaseBlocker()
{
    blockingBoxState().release();
}

App::DocumentObjectExecReturn* PartDesign::BlockingBoxTest::execute()
{
    return blockingBoxState().execute([this]() { return PartDesign::Box::execute(); });
}

class testTaskPrimitiveParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingBoxTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingBoxTest::init}));

        PartDesign::BlockingBoxTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_primitive_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        guiDoc->openCommand("Edit blocking primitive");

        box = doc->addObject<PartDesign::BlockingBoxTest>("BlockingBox");
        QVERIFY(box != nullptr);
        box->Length.setValue(10.0);
        box->Width.setValue(8.0);
        box->Height.setValue(6.0);

        primitiveView = dynamic_cast<PartDesignGui::ViewProviderPrimitive*>(
            guiDoc->getViewProvider(box)
        );
        QVERIFY(primitiveView != nullptr);
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingBoxTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        primitiveView = nullptr;
        box = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void rejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPrimitiveParameters(primitiveView);
        QPointer<PartDesignGui::TaskDlgPrimitiveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        auto* taskBox = findPrimitiveTaskBox(dialog);
        QVERIFY(taskBox != nullptr);

        auto* boxLength = findBoxLengthSpinBox(taskBox);
        QVERIFY(boxLength != nullptr);

        boxLength->setValue(boxLength->rawValue() + 1.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBoxTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingBoxTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void cancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPrimitiveParameters(primitiveView);
        QPointer<PartDesignGui::TaskDlgPrimitiveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findPrimitiveTaskBox(dialog);
        QVERIFY(taskBox != nullptr);

        auto* boxLength = findBoxLengthSpinBox(taskBox);
        QVERIFY(boxLength != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        boxLength->setValue(boxLength->rawValue() + 1.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBoxTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingBoxTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingBoxTest::getExecutionCount(), 1);
    }

    void primitiveAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgPrimitiveParameters(primitiveView);
        QPointer<PartDesignGui::TaskDlgPrimitiveParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findPrimitiveTaskBox(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* boxLength = findBoxLengthSpinBox(taskBox);
        QVERIFY(boxLength != nullptr);

        boxLength->setValue(boxLength->rawValue() + 1.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBoxTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingBoxTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        boxLength->setValue(boxLength->rawValue() + 2.0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        int timerTicksWhileAccepting = 0;
        bool sawModalProgressDialog = false;
        bool sawInlineAcceptStatus = false;
        bool sawDisabledDialogButtons = false;
        QTimer responsivenessTimer;
        QPointer<QWidget> taskBoxGuard(taskBox);
        responsivenessTimer.setInterval(10);
        connect(&responsivenessTimer, &QTimer::timeout, [&]() {
            ++timerTicksWhileAccepting;

            for (auto* widget : QApplication::topLevelWidgets()) {
                if (qobject_cast<QProgressDialog*>(widget) && widget->isVisible()) {
                    sawModalProgressDialog = true;
                }
            }

            if (taskBoxGuard) {
                auto* statusLabel = taskBoxGuard->findChild<QLabel*>(
                    QStringLiteral("labelPreviewStatus")
                );
                auto* statusWidget = taskBoxGuard->findChild<QWidget*>(
                    QStringLiteral("previewStatusWidget")
                );
                if (statusLabel && statusWidget && !statusLabel->isHidden()
                    && !statusWidget->isHidden()
                    && statusLabel->text().contains(QStringLiteral("Applying changes"))) {
                    sawInlineAcceptStatus = true;
                }
            }

            auto* activeButtonBox = findActiveDialogButtonBox();
            if (activeButtonBox && !activeButtonBox->isEnabled()) {
                sawDisabledDialogButtons = true;
            }
        });
        responsivenessTimer.start();
        QCoreApplication::processEvents();
        timerTicksWhileAccepting = 0;

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            PartDesign::BlockingBoxTest::releaseBlocker();
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
        responsivenessTimer.stop();
        releaser.join();
        QVERIFY2(acceptError.empty(), acceptError.c_str());

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(PartDesign::BlockingBoxTest::getTotalExecutionCount(), 2);
        QVERIFY2(
            timerTicksWhileAccepting > 0,
            "accept should keep the GUI event loop responsive while the final recompute settles"
        );
        QVERIFY2(!sawModalProgressDialog, "primitive accept should use the inline task-panel status instead of a modal progress dialog");
        QVERIFY2(
            sawInlineAcceptStatus,
            "primitive accept should show inline task-panel feedback while final recompute settles"
        );
        QVERIFY2(
            sawDisabledDialogButtons,
            "primitive accept should disable task dialog buttons until final recompute settles"
        );
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    Base::Reference<ParameterGrp> gizmoParams;
    bool oldAsyncEnabled = true;
    bool oldGizmosEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::BlockingBoxTest* box = nullptr;
    PartDesignGui::ViewProviderPrimitive* primitiveView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskPrimitiveParametersTest(int argc, char** argv)
{
    testTaskPrimitiveParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskPrimitiveParametersTest.moc"
