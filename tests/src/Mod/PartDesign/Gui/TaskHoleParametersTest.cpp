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
#include <QCheckBox>
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
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskHoleParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderHole.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingHoleTest: public PartDesign::Hole
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingHoleTest);

public:
    BlockingHoleTest() = default;
    ~BlockingHoleTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderHole";
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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingHoleState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingHoleTest,
    PartDesign::Hole,
    blockingHoleState
)

class testTaskHoleParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingHoleTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingHoleTest::init}));

        PartDesign::BlockingHoleTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_hole_dialog");
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

        sketch = doc->addObject<Sketcher::SketchObject>("SketchHole");
        QVERIFY(sketch != nullptr);
        sketch->AttachmentSupport.setValue(baseBox, "Face6");
        sketch->MapMode.setValue("FlatFace");
        sketch->MapReversed.setValue(true);
        body->addObject(sketch);

        Part::GeomCircle circle;
        circle.setCenter(Base::Vector3d(2.0, 2.0, 0.0));
        circle.setRadius(1.0);
        sketch->addGeometry(&circle, false);

        hole = doc->addObject<PartDesign::BlockingHoleTest>("BlockingHole");
        QVERIFY(hole != nullptr);
        try {
            body->addObject(hole);
        }
        catch (const Base::Exception& e) {
            QFAIL(e.what());
        }

        try {
            hole->Profile.setValue(sketch, {""});
            hole->Diameter.setValue(4.0);
            hole->Depth.setValue(6.0);
            hole->ThreadType.setValue(0L);
            hole->HoleCutType.setValue(0L);
            hole->DepthType.setValue(0L);
            hole->DrillPoint.setValue(0L);
            hole->Tapered.setValue(false);
        }
        catch (const Base::Exception& e) {
            QFAIL(e.what());
        }

        try {
            doc->recompute();
        }
        catch (const Base::Exception& e) {
            QFAIL(e.what());
        }

        holeView = dynamic_cast<PartDesignGui::ViewProviderHole*>(guiDoc->getViewProvider(hole));
        QVERIFY(holeView != nullptr);

        guiDoc->openCommand("Edit blocking hole");
        PartDesign::BlockingHoleTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingHoleTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        holeView = nullptr;
        hole = nullptr;
        sketch = nullptr;
        baseBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void holeRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgHoleParameters(holeView);
        QPointer<PartDesignGui::TaskDlgHoleParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskHoleParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("Reversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingHoleTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingHoleTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingHoleTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void holeCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgHoleParameters(holeView);
        QPointer<PartDesignGui::TaskDlgHoleParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskHoleParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("Reversed"));
        QVERIFY(reversed != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingHoleTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingHoleTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingHoleTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingHoleTest::getExecutionCount(), 1);
    }

    void holeAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgHoleParameters(holeView);
        QPointer<PartDesignGui::TaskDlgHoleParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskHoleParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* reversed = taskBox->findChild<QCheckBox*>(QStringLiteral("Reversed"));
        QVERIFY(reversed != nullptr);

        PartDesign::BlockingHoleTest::armBlocker();
        reversed->click();
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingHoleTest::getExecutionCount(), 1, 3000);
        QCOMPARE(PartDesign::BlockingHoleTest::getTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        reversed->click();
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingHoleTest::releaseBlocker();
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
        QCOMPARE(PartDesign::BlockingHoleTest::getTotalExecutionCount(), 2);
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
    Sketcher::SketchObject* sketch = nullptr;
    PartDesign::BlockingHoleTest* hole = nullptr;
    PartDesignGui::ViewProviderHole* holeView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskHoleParametersTest(int argc, char** argv)
{
    testTaskHoleParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskHoleParametersTest.moc"
