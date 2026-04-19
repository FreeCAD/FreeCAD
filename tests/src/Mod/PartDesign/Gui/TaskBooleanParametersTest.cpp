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
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskBooleanParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderBoolean.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingBooleanTest: public PartDesign::Boolean
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingBooleanTest);

public:
    BlockingBooleanTest() = default;
    ~BlockingBooleanTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderBoolean";
    }

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingBooleanState)

using tests::partdesigngui::activateComboIndex;
using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingBooleanTest,
    PartDesign::Boolean,
    blockingBooleanState
)

class testTaskBooleanParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingBooleanTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingBooleanTest::init}));

        docName = App::GetApplication().getUniqueDocumentName("blocking_boolean_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);

        bodyA = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(bodyA != nullptr);
        boxA = doc->addObject<PartDesign::AdditiveBox>("Box");
        QVERIFY(boxA != nullptr);
        boxA->Length.setValue(10.0);
        boxA->Width.setValue(10.0);
        boxA->Height.setValue(10.0);
        bodyA->addObject(boxA);
        doc->recompute();

        bodyB = doc->addObject<PartDesign::Body>("Body001");
        QVERIFY(bodyB != nullptr);
        boxB = doc->addObject<PartDesign::AdditiveBox>("Box001");
        QVERIFY(boxB != nullptr);
        boxB->Length.setValue(10.0);
        boxB->Width.setValue(10.0);
        boxB->Height.setValue(10.0);
        boxB->Placement.setValue(Base::Placement(Base::Vector3d(-5.0, 0.0, 0.0), Base::Rotation()));
        bodyB->addObject(boxB);
        doc->recompute();

        booleanFeature = doc->addObject<PartDesign::BlockingBooleanTest>("BooleanFuse");
        QVERIFY(booleanFeature != nullptr);
        bodyB->addObject(booleanFeature);
        booleanFeature->setObjects({bodyA});
        booleanFeature->Type.setValue(0L);
        doc->recompute();

        booleanView = dynamic_cast<PartDesignGui::ViewProviderBoolean*>(
            guiDoc->getViewProvider(booleanFeature)
        );
        QVERIFY(booleanView != nullptr);

        PartDesign::BlockingBooleanTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingBooleanTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        booleanView = nullptr;
        booleanFeature = nullptr;
        boxB = nullptr;
        bodyB = nullptr;
        boxA = nullptr;
        bodyA = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void booleanRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgBooleanParameters(booleanView);
        QPointer<PartDesignGui::TaskDlgBooleanParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskBooleanParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* comboType = taskBox->findChild<QComboBox*>(QStringLiteral("comboType"));
        QVERIFY(comboType != nullptr);

        PartDesign::BlockingBooleanTest::armBlocker();
        QVERIFY(activateComboIndex(comboType, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBooleanTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());

        PartDesign::BlockingBooleanTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
    }

    void booleanCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgBooleanParameters(booleanView);
        QPointer<PartDesignGui::TaskDlgBooleanParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskBooleanParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* comboType = taskBox->findChild<QComboBox*>(QStringLiteral("comboType"));
        QVERIFY(comboType != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingBooleanTest::armBlocker();
        QVERIFY(activateComboIndex(comboType, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBooleanTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingBooleanTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingBooleanTest::getExecutionCount(), 1);
    }

    void booleanAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgBooleanParameters(booleanView);
        QPointer<PartDesignGui::TaskDlgBooleanParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskBooleanParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* comboType = taskBox->findChild<QComboBox*>(QStringLiteral("comboType"));
        QVERIFY(comboType != nullptr);

        PartDesign::BlockingBooleanTest::armBlocker();
        QVERIFY(activateComboIndex(comboType, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingBooleanTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingBooleanTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        QVERIFY(activateComboIndex(comboType, 2));
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingBooleanTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingBooleanTest::getTotalExecutionCount(), 2);
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::Body* bodyA = nullptr;
    PartDesign::AdditiveBox* boxA = nullptr;
    PartDesign::Body* bodyB = nullptr;
    PartDesign::AdditiveBox* boxB = nullptr;
    PartDesign::BlockingBooleanTest* booleanFeature = nullptr;
    PartDesignGui::ViewProviderBoolean* booleanView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskBooleanParametersTest(int argc, char** argv)
{
    testTaskBooleanParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskBooleanParametersTest.moc"
