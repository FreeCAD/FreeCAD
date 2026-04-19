// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <QCoreApplication>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QToolButton>

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
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ShapeBinder.h>
// Drive the task's internal selection handler directly in the headless test harness.
#define private public
#include <Mod/PartDesign/Gui/TaskShapeBinder.h>
#undef private
#include <Mod/PartDesign/Gui/ViewProviderShapeBinder.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingShapeBinderTest: public PartDesign::ShapeBinder
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingShapeBinderTest);

public:
    BlockingShapeBinderTest() = default;
    ~BlockingShapeBinderTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingShapeBinderState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

QLineEdit* findBaseEdit(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<QLineEdit*>(QStringLiteral("baseEdit")) : nullptr;
}

QToolButton* findBaseButton(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<QToolButton*>(QStringLiteral("buttonBase")) : nullptr;
}

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingShapeBinderTest,
    PartDesign::ShapeBinder,
    blockingShapeBinderState
)

class testTaskShapeBinder final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingShapeBinderTest::init}));

        asyncParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Document"
        );
        oldAsyncEnabled = asyncParams->GetBool("EnableAsyncRecompute", true);
        asyncParams->SetBool("EnableAsyncRecompute", true);
    }

    void cleanupTestCase()  // NOLINT
    {
        if (asyncParams) {
            asyncParams->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            asyncParams = nullptr;
        }
    }

    void init()  // NOLINT
    {
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingShapeBinderTest::init}));

        PartDesign::BlockingShapeBinderTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_shapebinder_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);

        externalBox = doc->addObject<Part::Box>("ExternalBox");
        QVERIFY(externalBox != nullptr);
        externalBox->Length.setValue(10.0);
        externalBox->Width.setValue(8.0);
        externalBox->Height.setValue(6.0);

        externalBox2 = doc->addObject<Part::Box>("ExternalBox2");
        QVERIFY(externalBox2 != nullptr);
        externalBox2->Length.setValue(12.0);
        externalBox2->Width.setValue(9.0);
        externalBox2->Height.setValue(5.0);

        externalBox3 = doc->addObject<Part::Box>("ExternalBox3");
        QVERIFY(externalBox3 != nullptr);
        externalBox3->Length.setValue(7.0);
        externalBox3->Width.setValue(11.0);
        externalBox3->Height.setValue(4.0);

        doc->recompute();

        binder = doc->addObject<PartDesign::BlockingShapeBinderTest>("BlockingShapeBinder");
        QVERIFY(binder != nullptr);
        body->addObject(binder);
        binder->Support.setValue(externalBox, {"Face1"});

        doc->recompute();

        binderView = dynamic_cast<PartDesignGui::ViewProviderShapeBinder*>(
            guiDoc->getViewProvider(binder)
        );
        QVERIFY(binderView != nullptr);

        guiDoc->openCommand("Edit blocking shape binder");
        PartDesign::BlockingShapeBinderTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingShapeBinderTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        binderView = nullptr;
        binder = nullptr;
        externalBox3 = nullptr;
        externalBox2 = nullptr;
        externalBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void shapeBinderRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgShapeBinder(binderView);
        QPointer<PartDesignGui::TaskDlgShapeBinder> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskShapeBinder>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* baseEdit = findBaseEdit(taskBox);
        QVERIFY(baseEdit != nullptr);
        QVERIFY(!baseEdit->text().isEmpty());

        PartDesign::BlockingShapeBinderTest::armBlocker();
        baseEdit->clear();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingShapeBinderTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingShapeBinderTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void shapeBinderCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgShapeBinder(binderView);
        QPointer<PartDesignGui::TaskDlgShapeBinder> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskShapeBinder>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* baseEdit = findBaseEdit(taskBox);
        QVERIFY(baseEdit != nullptr);
        QVERIFY(!baseEdit->text().isEmpty());

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingShapeBinderTest::armBlocker();
        baseEdit->clear();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingShapeBinderTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingShapeBinderTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingShapeBinderTest::getExecutionCount(), 1);
    }

    void shapeBinderAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgShapeBinder(binderView);
        QPointer<PartDesignGui::TaskDlgShapeBinder> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskShapeBinder>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* baseEdit = findBaseEdit(taskBox);
        QVERIFY(baseEdit != nullptr);

        auto* baseButton = findBaseButton(taskBox);
        QVERIFY(baseButton != nullptr);

        PartDesign::BlockingShapeBinderTest::armBlocker();
        baseButton->click();
        QCoreApplication::processEvents();
        QVERIFY(baseButton->isChecked());
        taskBox->onSelectionChanged(
            Gui::SelectionChanges(
                Gui::SelectionChanges::AddSelection,
                doc->getName(),
                externalBox2->getNameInDocument()
            )
        );
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingShapeBinderTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingShapeBinderTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(baseEdit->text(), QString::fromStdString(externalBox2->Label.getStrValue()));

        baseButton->click();
        QCoreApplication::processEvents();
        QVERIFY(baseButton->isChecked());
        taskBox->onSelectionChanged(
            Gui::SelectionChanges(
                Gui::SelectionChanges::AddSelection,
                doc->getName(),
                externalBox3->getNameInDocument()
            )
        );
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(baseEdit->text(), QString::fromStdString(externalBox3->Label.getStrValue()));

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingShapeBinderTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingShapeBinderTest::getTotalExecutionCount(), 2);
        QCOMPARE(binder->Support.getValue(), static_cast<App::DocumentObject*>(externalBox3));
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::Body* body = nullptr;
    Part::Box* externalBox = nullptr;
    Part::Box* externalBox2 = nullptr;
    Part::Box* externalBox3 = nullptr;
    PartDesign::BlockingShapeBinderTest* binder = nullptr;
    PartDesignGui::ViewProviderShapeBinder* binderView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskShapeBinderTest(int argc, char** argv)
{
    testTaskShapeBinder test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskShapeBinderTest.moc"
