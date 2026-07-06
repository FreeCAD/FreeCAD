// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

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
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureDraft.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/Gui/TaskDraftParameters.h>
#include <Mod/PartDesign/Gui/ViewProviderDraft.h>

#include <TopAbs_ShapeEnum.hxx>

#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingDraftTest: public PartDesign::Draft
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingDraftTest);

public:
    BlockingDraftTest() = default;
    ~BlockingDraftTest() override = default;

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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingDraftState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

Gui::QuantitySpinBox* findAngleSpinBox(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<Gui::QuantitySpinBox*>(QStringLiteral("draftAngle"))
                   : nullptr;
}

std::string findTopFaceSubname(const PartDesign::AdditiveBox& box)
{
    const auto faces = box.Shape.getShape().getSubTopoShapes(TopAbs_FACE);

    constexpr double HorizontalFaceTolerance = 1e-7;
    double bestZMax = -std::numeric_limits<double>::infinity();
    int bestIndex = -1;

    for (std::size_t i = 0; i < faces.size(); ++i) {
        const auto bbox = faces[i].getBoundBox();
        if (bbox.LengthZ() > HorizontalFaceTolerance) {
            continue;
        }
        if (bbox.MaxZ > bestZMax) {
            bestZMax = bbox.MaxZ;
            bestIndex = static_cast<int>(i);
        }
    }

    if (bestIndex < 0) {
        return "Face1";
    }

    return "Face" + std::to_string(bestIndex + 1);
}

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingDraftTest,
    PartDesign::Draft,
    blockingDraftState
)

class testTaskDraftParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import _PartDesign");
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingDraftTest::init}));

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
        Q_UNUSED(ensureGuiHarness({&PartDesign::BlockingDraftTest::init}));

        PartDesign::BlockingDraftTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("blocking_draft_dialog");
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

        datumPlane = doc->addObject<PartDesign::Plane>("DatumPlane");
        QVERIFY(datumPlane != nullptr);
        body->addObject(datumPlane);
        datumPlane->AttachmentSupport.setValue(doc->getObject("YZ_Plane"), "");
        datumPlane->MapMode.setValue("FlatFace");

        datumLine = doc->addObject<PartDesign::Line>("DatumLine");
        QVERIFY(datumLine != nullptr);
        body->addObject(datumLine);
        datumLine->AttachmentSupport.setValue(doc->getObject("X_Axis"), "");
        datumLine->MapMode.setValue("TwoPointLine");

        doc->recompute();

        draft = doc->addObject<PartDesign::BlockingDraftTest>("BlockingDraft");
        QVERIFY(draft != nullptr);
        body->addObject(draft);
        draft->Base.setValue(baseBox, {findTopFaceSubname(*baseBox)});
        draft->NeutralPlane.setValue(datumPlane, {""});
        draft->PullDirection.setValue(datumLine, {""});
        draft->Angle.setValue(45.0);
        draft->Reversed.setValue(true);

        doc->recompute();

        if (draft->isError()) {
            draft->Reversed.setValue(false);
            doc->recompute();
        }

        QVERIFY(!draft->isError());

        draftView = dynamic_cast<PartDesignGui::ViewProviderDraft*>(guiDoc->getViewProvider(draft));
        QVERIFY(draftView != nullptr);

        guiDoc->openCommand("Edit blocking draft");
        PartDesign::BlockingDraftTest::resetBlocker();
    }

    void cleanup()  // NOLINT
    {
        PartDesign::BlockingDraftTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        draftView = nullptr;
        draft = nullptr;
        datumLine = nullptr;
        datumPlane = nullptr;
        baseBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void draftRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgDraftParameters(draftView);
        QPointer<PartDesignGui::TaskDlgDraftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskDraftParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* angle = findAngleSpinBox(taskBox);
        QVERIFY(angle != nullptr);

        PartDesign::BlockingDraftTest::armBlocker();
        angle->setValue(angle->rawValue() - 5.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingDraftTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingDraftTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void draftCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgDraftParameters(draftView);
        QPointer<PartDesignGui::TaskDlgDraftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskDraftParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* angle = findAngleSpinBox(taskBox);
        QVERIFY(angle != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingDraftTest::armBlocker();
        angle->setValue(angle->rawValue() - 5.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingDraftTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingDraftTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingDraftTest::getExecutionCount(), 1);
    }

    void draftAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        auto* dialog = new PartDesignGui::TaskDlgDraftParameters(draftView);
        QPointer<PartDesignGui::TaskDlgDraftParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskDraftParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* angle = findAngleSpinBox(taskBox);
        QVERIFY(angle != nullptr);

        PartDesign::BlockingDraftTest::armBlocker();
        angle->setValue(angle->rawValue() - 5.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingDraftTest::getExecutionCount(), 1, 3000);
        QVERIFY2(
            PartDesign::BlockingDraftTest::getTotalExecutionCount() <= 2,
            "accept should settle without replaying the queued preview and then rerunning again"
        );
        QVERIFY(taskBox->hasOutstandingRecompute());

        angle->setValue(angle->rawValue() - 5.0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingDraftTest::releaseBlocker();
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
            PartDesign::BlockingDraftTest::getTotalExecutionCount() <= 2,
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
    PartDesign::AdditiveBox* baseBox = nullptr;
    PartDesign::Plane* datumPlane = nullptr;
    PartDesign::Line* datumLine = nullptr;
    PartDesign::BlockingDraftTest* draft = nullptr;
    PartDesignGui::ViewProviderDraft* draftView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskDraftParametersTest(int argc, char** argv)
{
    testTaskDraftParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskDraftParametersTest.moc"
