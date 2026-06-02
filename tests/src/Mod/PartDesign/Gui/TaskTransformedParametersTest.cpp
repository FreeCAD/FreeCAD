// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QTest>
#include <QTimer>

#include <App/Application.h>
#include <App/Datums.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/PropertyContainer.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/QuantitySpinBox.h>
#include <Gui/SpinBox.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/Gui/PatternParametersWidget.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/FeatureScaled.h>
#include <Mod/PartDesign/Gui/TaskMirroredParameters.h>
#include <Mod/PartDesign/Gui/TaskMultiTransformParameters.h>
#include <Mod/PartDesign/Gui/TaskPatternParameters.h>
#include <Mod/PartDesign/Gui/TaskScaledParameters.h>
#include <Mod/PartDesign/Gui/TaskTransformedParameters.h>
#include <src/App/InitApplication.h>

#include "TaskDialogTestUtils.h"

namespace PartDesign
{

class BlockingLinearPatternTest: public PartDesign::LinearPattern
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingLinearPatternTest);

public:
    BlockingLinearPatternTest() = default;
    ~BlockingLinearPatternTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderLinearPattern";
    }

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingPolarPatternTest: public PartDesign::PolarPattern
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingPolarPatternTest);

public:
    BlockingPolarPatternTest() = default;
    ~BlockingPolarPatternTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderPolarPattern";
    }

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingMirroredTest: public PartDesign::Mirrored
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingMirroredTest);

public:
    BlockingMirroredTest() = default;
    ~BlockingMirroredTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderMirrored";
    }

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingScaledTest: public PartDesign::Scaled
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingScaledTest);

public:
    BlockingScaledTest() = default;
    ~BlockingScaledTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderScaled";
    }

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static int getTotalExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingMultiTransformTest: public PartDesign::MultiTransform
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::BlockingMultiTransformTest);

public:
    BlockingMultiTransformTest() = default;
    ~BlockingMultiTransformTest() override = default;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderMultiTransform";
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
FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(blockingTransformedState)

using tests::partdesigngui::ensureGuiHarness;
using tests::partdesigngui::findCancelPreviewButton;
using tests::partdesigngui::findTaskBox;

void traceTransformedTest(const char* message)
{
    std::fprintf(stderr, "[TaskTransformedParametersTest] %s\n", message);
    std::fflush(stderr);
}

struct CapturedConsoleLog
{
    std::string msg;
    Base::LogStyle level;
};

class CapturingConsoleLogger final: public Base::ILogger
{
public:
    CapturingConsoleLogger(std::vector<CapturedConsoleLog>& output, std::mutex& mutex)
        : output(output)
        , mutex(mutex)
    {
        bNotification = true;
    }

    void sendLog(
        const std::string&,
        const std::string& msg,
        Base::LogStyle level,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        output.push_back({msg, level});
    }

private:
    std::vector<CapturedConsoleLog>& output;
    std::mutex& mutex;
};

class ScopedConsoleObserver
{
public:
    explicit ScopedConsoleObserver(Base::ILogger& logger)
        : logger(logger)
    {
        Base::Console().attachObserver(&logger);
    }

    ~ScopedConsoleObserver()
    {
        Base::Console().detachObserver(&logger);
    }

private:
    Base::ILogger& logger;
};

bool isUserVisiblePreviewAbortLog(const CapturedConsoleLog& log)
{
    const bool userVisibleLevel = log.level == Base::LogStyle::Warning
        || log.level == Base::LogStyle::Error || log.level == Base::LogStyle::Critical
        || log.level == Base::LogStyle::Notification;
    return userVisibleLevel && log.msg.find("User aborted") != std::string::npos;
}

int getBlockingTransformedTotalExecutionCount()
{
    return blockingTransformedState().getTotalExecutionCount();
}

PartGui::PatternParametersWidget* findPrimaryPatternParametersWidget(QWidget* taskBox)
{
    if (!taskBox) {
        return nullptr;
    }

    const auto widgets = taskBox->findChildren<PartGui::PatternParametersWidget*>();
    return widgets.empty() ? nullptr : widgets.front();
}

Gui::UIntSpinBox* findOccurrencesSpinBox(PartGui::PatternParametersWidget* widget)
{
    return widget ? widget->findChild<Gui::UIntSpinBox*>(QStringLiteral("spinOccurrences")) : nullptr;
}

Gui::QuantitySpinBox* findExtentSpinBox(PartGui::PatternParametersWidget* widget)
{
    return widget ? widget->findChild<Gui::QuantitySpinBox*>(QStringLiteral("spinExtent")) : nullptr;
}

QComboBox* findPlaneCombo(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<QComboBox*>(QStringLiteral("comboPlane")) : nullptr;
}

QDialogButtonBox* findActiveDialogButtonBox()
{
    auto* taskView = Gui::Control().taskPanel();
    return taskView ? taskView->findChild<QDialogButtonBox*>() : nullptr;
}

Gui::UIntSpinBox* findUIntSpinBox(QWidget* taskBox, const QString& objectName)
{
    return taskBox ? taskBox->findChild<Gui::UIntSpinBox*>(objectName) : nullptr;
}

Gui::QuantitySpinBox* findQuantitySpinBox(QWidget* taskBox, const QString& objectName)
{
    return taskBox ? taskBox->findChild<Gui::QuantitySpinBox*>(objectName) : nullptr;
}

bool activateComboIndex(QComboBox* combo, int index)
{
    if (!combo || index < 0 || index >= combo->count()) {
        return false;
    }

    combo->setCurrentIndex(index);
    return QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, index));
}

QListWidget* findListWidget(QWidget* taskBox, const QString& objectName)
{
    return taskBox ? taskBox->findChild<QListWidget*>(objectName) : nullptr;
}

enum class TransformedKind
{
    Linear,
    Polar,
    Mirrored,
    Scaled,
    MultiTransformScaled,
    MultiTransformPolar
};

}  // namespace

FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingLinearPatternTest,
    PartDesign::LinearPattern,
    blockingTransformedState
)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingPolarPatternTest,
    PartDesign::PolarPattern,
    blockingTransformedState
)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingMirroredTest,
    PartDesign::Mirrored,
    blockingTransformedState
)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingScaledTest,
    PartDesign::Scaled,
    blockingTransformedState
)
FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(
    PartDesign::BlockingMultiTransformTest,
    PartDesign::MultiTransform,
    blockingTransformedState
)

class testTaskTransformedParameters final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        traceTransformedTest("initTestCase: begin");
        traceTransformedTest("initTestCase: before tests::initApplication");
        tests::initApplication();
        traceTransformedTest("initTestCase: after tests::initApplication");
        traceTransformedTest("initTestCase: before Python Part import");
        Base::Interpreter().runString("import Part");
        traceTransformedTest("initTestCase: after Python Part import");
        traceTransformedTest("initTestCase: before Python _PartDesign import");
        Base::Interpreter().runString("import _PartDesign");
        traceTransformedTest("initTestCase: after Python _PartDesign import");
        traceTransformedTest("initTestCase: before ensureGuiHarness");
        Q_UNUSED(ensureGuiHarness(
            {&PartDesign::BlockingLinearPatternTest::init,
             &PartDesign::BlockingPolarPatternTest::init,
             &PartDesign::BlockingMirroredTest::init,
             &PartDesign::BlockingScaledTest::init,
             &PartDesign::BlockingMultiTransformTest::init}
        ));
        traceTransformedTest("initTestCase: after ensureGuiHarness");

        traceTransformedTest("initTestCase: before async parameter lookup");
        asyncParams = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Document"
        );
        oldAsyncEnabled = asyncParams->GetBool("EnableAsyncRecompute", true);
        asyncParams->SetBool("EnableAsyncRecompute", true);
        traceTransformedTest("initTestCase: done");
    }

    void cleanupTestCase()  // NOLINT
    {
        traceTransformedTest("cleanupTestCase: begin");
        if (asyncParams) {
            asyncParams->SetBool("EnableAsyncRecompute", oldAsyncEnabled);
            asyncParams = nullptr;
        }
        traceTransformedTest("cleanupTestCase: done");
    }

    void init()  // NOLINT
    {
        traceTransformedTest("init: begin");
        traceTransformedTest("init: before ensureGuiHarness");
        Q_UNUSED(ensureGuiHarness(
            {&PartDesign::BlockingLinearPatternTest::init,
             &PartDesign::BlockingPolarPatternTest::init,
             &PartDesign::BlockingMirroredTest::init,
             &PartDesign::BlockingScaledTest::init,
             &PartDesign::BlockingMultiTransformTest::init}
        ));
        traceTransformedTest("init: after ensureGuiHarness");

        traceTransformedTest("init: before resetBlocker");
        PartDesign::BlockingLinearPatternTest::resetBlocker();
        traceTransformedTest("init: after resetBlocker");

        traceTransformedTest("init: before unique document name");
        docName = App::GetApplication().getUniqueDocumentName("blocking_pattern_dialog");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        traceTransformedTest("init: before newDocument");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);
        traceTransformedTest("init: after newDocument");

        traceTransformedTest("init: before gui document lookup");
        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);
        traceTransformedTest("init: after gui document lookup");

        traceTransformedTest("init: before body creation");
        body = doc->addObject<PartDesign::Body>("Body");
        QVERIFY(body != nullptr);
        traceTransformedTest("init: after body creation");

        traceTransformedTest("init: before base box creation");
        baseBox = doc->addObject<PartDesign::AdditiveBox>("BaseBox");
        QVERIFY(baseBox != nullptr);
        body->addObject(baseBox);
        traceTransformedTest("init: after base box creation");

        baseBox->Length.setValue(10.0);
        baseBox->Width.setValue(8.0);
        baseBox->Height.setValue(6.0);
        traceTransformedTest("init: after base box dimensions");

        traceTransformedTest("init: before doc recompute");
        doc->recompute();

        traceTransformedTest("init: before origin references");
        bodyXAxis = body->getOrigin()->getX();
        bodyXYPlane = body->getOrigin()->getXY();
        bodyYZPlane = body->getOrigin()->getYZ();
        QVERIFY(bodyXAxis != nullptr);
        QVERIFY(bodyXYPlane != nullptr);
        QVERIFY(bodyYZPlane != nullptr);
        traceTransformedTest("init: after origin references");

        traceTransformedTest("init: done");
    }

    void cleanup()  // NOLINT
    {
        traceTransformedTest("cleanup: begin");
        traceTransformedTest("cleanup: before releaseBlocker");
        PartDesign::BlockingLinearPatternTest::releaseBlocker();
        QCoreApplication::processEvents();
        traceTransformedTest("cleanup: after releaseBlocker/processEvents");

        if (doc && Gui::Control().activeDialog(doc)) {
            traceTransformedTest("cleanup: before closeDialog");
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
            traceTransformedTest("cleanup: after closeDialog");
        }

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            traceTransformedTest("cleanup: before closeDocument");
            App::GetApplication().closeDocument(docName.c_str());
            traceTransformedTest("cleanup: after closeDocument");
        }

        transformedView = nullptr;
        bodyXAxis = nullptr;
        bodyXYPlane = nullptr;
        bodyYZPlane = nullptr;
        baseBox = nullptr;
        body = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
        traceTransformedTest("cleanup: done");
    }

    void linearRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        traceTransformedTest("linearReject: begin");
        traceTransformedTest("linearReject: before prepareTransformedFixture");
        prepareTransformedFixture(TransformedKind::Linear);
        traceTransformedTest("linearReject: after prepareTransformedFixture");

        traceTransformedTest("linearReject: before dialog creation");
        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        traceTransformedTest("linearReject: after dialog creation");
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        traceTransformedTest("linearReject: before showDialog");
        Gui::Control().showDialog(dialog, doc);
        traceTransformedTest("linearReject: after showDialog");
        QCoreApplication::processEvents();
        traceTransformedTest("linearReject: after processEvents");

        traceTransformedTest("linearReject: before findTaskBox");
        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        traceTransformedTest("linearReject: after findTaskBox");

        traceTransformedTest("linearReject: before findPrimaryPatternParametersWidget");
        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);
        traceTransformedTest("linearReject: after findPrimaryPatternParametersWidget");

        traceTransformedTest("linearReject: before findOccurrencesSpinBox");
        auto* occurrences = findOccurrencesSpinBox(widget);
        QVERIFY(occurrences != nullptr);
        traceTransformedTest("linearReject: after findOccurrencesSpinBox");

        traceTransformedTest("linearReject: before armBlocker");
        PartDesign::BlockingLinearPatternTest::armBlocker();
        traceTransformedTest("linearReject: after armBlocker");
        traceTransformedTest("linearReject: before occurrences change");
        occurrences->setValue(occurrences->value() + 1);
        QCoreApplication::processEvents();
        traceTransformedTest("linearReject: after occurrences change/processEvents");

        traceTransformedTest("linearReject: before wait for execution count");
        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingLinearPatternTest::getExecutionCount(), 1, 3000);
        traceTransformedTest("linearReject: after wait for execution count");
        QVERIFY(taskBox->hasOutstandingRecompute());
        traceTransformedTest("linearReject: after outstanding recompute verify");

        traceTransformedTest("linearReject: before reject");
        Gui::Control().reject(doc);
        traceTransformedTest("linearReject: after reject");

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());
        traceTransformedTest("linearReject: after reject state verifies");

        traceTransformedTest("linearReject: before releaseBlocker");
        PartDesign::BlockingLinearPatternTest::releaseBlocker();
        traceTransformedTest("linearReject: after releaseBlocker");

        traceTransformedTest("linearReject: before wait for dialog close");
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        traceTransformedTest("linearReject: after wait for dialog close");
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        traceTransformedTest("linearReject: done");
    }

    void linearCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Linear);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* occurrences = findOccurrencesSpinBox(widget);
        QVERIFY(occurrences != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingLinearPatternTest::armBlocker();
        occurrences->setValue(occurrences->value() + 1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingLinearPatternTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingLinearPatternTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingLinearPatternTest::getExecutionCount(), 1);
    }

    void linearAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Linear);
        auto* pattern = dynamic_cast<PartDesign::BlockingLinearPatternTest*>(
            doc->getObject("BlockingLinearPattern")
        );
        QVERIFY(pattern != nullptr);
        const int baselineExecutionCount = getBlockingTransformedTotalExecutionCount();

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* occurrences = findOccurrencesSpinBox(widget);
        QVERIFY(occurrences != nullptr);

        PartDesign::BlockingLinearPatternTest::armBlocker();
        occurrences->setValue(occurrences->value() + 1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingLinearPatternTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), baselineExecutionCount + 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        const int finalOccurrences = occurrences->value() + 1;
        occurrences->setValue(finalOccurrences);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingLinearPatternTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(pattern->Occurrences.getValue(), finalOccurrences);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), baselineExecutionCount + 2);
    }

    void polarRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingPolarPatternTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void polarCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        std::mutex logMutex;
        std::vector<CapturedConsoleLog> logs;
        CapturingConsoleLogger logger(logs, logMutex);
        ScopedConsoleObserver scopedLogger(logger);
        Base::Console().setConnectionMode(Base::ConsoleSingleton::Direct);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingPolarPatternTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!pattern->isError());
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1);

        {
            std::lock_guard<std::mutex> lock(logMutex);
            const bool reportedAbort = std::ranges::any_of(logs, isUserVisiblePreviewAbortLog);
            QVERIFY2(!reportedAbort, "manual preview cancel must not report User aborted");
        }
    }

    void polarAcceptAfterCanceledPreviewRunsFinalRecompute()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* occurrences = findOccurrencesSpinBox(widget);
        QVERIFY(occurrences != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        const int acceptedOccurrences = occurrences->value() + 1;
        occurrences->setValue(acceptedOccurrences);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());

        PartDesign::BlockingPolarPatternTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());

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

        QVERIFY2(acceptError.empty(), acceptError.c_str());
        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(pattern->Occurrences.getValue(), acceptedOccurrences);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 2);
    }

    void polarAcceptKeepsGuiResponsiveWhileSettlingInFlightPreview()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        const double acceptedAngle = extent->rawValue() - 45.0;
        extent->setValue(acceptedAngle);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
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
            PartDesign::BlockingPolarPatternTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        responsivenessTimer.stop();
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QVERIFY2(
            timerTicksWhileAccepting > 0,
            "accept should keep the GUI event loop responsive while the final recompute settles"
        );
        QVERIFY2(!sawModalProgressDialog, "transformed accept should use the inline task-panel status instead of a modal progress dialog");
        QVERIFY2(sawInlineAcceptStatus, "transformed accept should show inline task-panel feedback while final recompute settles");
        QVERIFY2(
            sawDisabledDialogButtons,
            "transformed accept should disable task dialog buttons until final recompute settles"
        );
        QCOMPARE(pattern->Angle.getValue(), acceptedAngle);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 2);
    }

    void polarAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        const double finalAngle = extent->rawValue() + 15.0;
        extent->setValue(finalAngle);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingPolarPatternTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(pattern->Angle.getValue(), finalAngle);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 2);
    }

    void polarAcceptAfterSettledPreviewRunsFinalRecompute()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        const int startingExecutionCount = getBlockingTransformedTotalExecutionCount();
        const double finalAngle = extent->rawValue() - 45.0;
        extent->setValue(finalAngle);
        QCoreApplication::processEvents();

        QTRY_VERIFY_WITH_TIMEOUT(
            getBlockingTransformedTotalExecutionCount() > startingExecutionCount,
            3000
        );
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        const int settledPreviewExecutionCount = getBlockingTransformedTotalExecutionCount();

        Gui::Control().accept(doc);

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(pattern->Angle.getValue(), finalAngle);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), settledPreviewExecutionCount + 1);
    }

    void polarQueuedPreviewKeepsSingleActiveTaskDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Polar);
        auto* pattern = dynamic_cast<PartDesign::BlockingPolarPatternTest*>(
            doc->getObject("BlockingPolarPattern")
        );
        QVERIFY(pattern != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskPatternParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* buttonBox = findActiveDialogButtonBox();
        QVERIFY(buttonBox != nullptr);
        QVERIFY(buttonBox->isEnabled());

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingPolarPatternTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingPolarPatternTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        const double finalAngle = extent->rawValue() + 15.0;
        extent->setValue(finalAngle);
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(extent->isEnabled());
        QVERIFY(cancelPreview->isEnabled());
        QVERIFY(buttonBox->isEnabled());

        auto* duplicateDialog = new PartDesignGui::TaskDlgLinearPatternParameters(transformedView);
        QPointer<PartDesignGui::TaskDlgLinearPatternParameters> duplicateGuard(duplicateDialog);
        Gui::Control().showDialog(duplicateDialog, doc);
        QCoreApplication::processEvents();

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(!guard.isNull());
        QVERIFY(!duplicateGuard.isNull());

        delete duplicateDialog;
        QVERIFY(duplicateGuard.isNull());

        PartDesign::BlockingPolarPatternTest::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(getBlockingTransformedTotalExecutionCount(), 2, 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(buttonBox->isEnabled());

        Gui::Control().accept(doc);

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(pattern->Angle.getValue(), finalAngle);
    }

    void mirroredRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Mirrored);

        auto* mirroredView = dynamic_cast<PartDesignGui::ViewProviderMirrored*>(transformedView);
        QVERIFY(mirroredView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMirroredParameters(mirroredView);
        QPointer<PartDesignGui::TaskDlgMirroredParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMirroredParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* planeCombo = findPlaneCombo(taskBox);
        QVERIFY(planeCombo != nullptr);
        QVERIFY(planeCombo->count() >= 3);

        PartDesign::BlockingMirroredTest::armBlocker();
        QVERIFY(activateComboIndex(planeCombo, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMirroredTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingMirroredTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void mirroredCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Mirrored);

        auto* mirroredView = dynamic_cast<PartDesignGui::ViewProviderMirrored*>(transformedView);
        QVERIFY(mirroredView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMirroredParameters(mirroredView);
        QPointer<PartDesignGui::TaskDlgMirroredParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMirroredParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* planeCombo = findPlaneCombo(taskBox);
        QVERIFY(planeCombo != nullptr);
        QVERIFY(planeCombo->count() >= 3);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingMirroredTest::armBlocker();
        QVERIFY(activateComboIndex(planeCombo, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMirroredTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingMirroredTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingMirroredTest::getExecutionCount(), 1);
    }

    void mirroredAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Mirrored);

        auto* mirroredView = dynamic_cast<PartDesignGui::ViewProviderMirrored*>(transformedView);
        QVERIFY(mirroredView != nullptr);
        auto* mirrored = mirroredView->getObject<PartDesign::Mirrored>();
        QVERIFY(mirrored != nullptr);
        auto* origin = body->getOrigin();
        QVERIFY(origin != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMirroredParameters(mirroredView);
        QPointer<PartDesignGui::TaskDlgMirroredParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMirroredParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* planeCombo = findPlaneCombo(taskBox);
        QVERIFY(planeCombo != nullptr);
        QVERIFY(planeCombo->count() >= 3);

        PartDesign::BlockingMirroredTest::armBlocker();
        QVERIFY(activateComboIndex(planeCombo, 1));
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMirroredTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        QVERIFY(activateComboIndex(planeCombo, 2));
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingMirroredTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 2);
        QCOMPARE(mirrored->MirrorPlane.getValue(), static_cast<App::DocumentObject*>(origin->getXZ()));
        const auto& mirrorPlaneSubs = mirrored->MirrorPlane.getSubValues();
        QCOMPARE(mirrorPlaneSubs.size(), std::size_t(1));
        QVERIFY(mirrorPlaneSubs.front().empty());
    }

    void scaledRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Scaled);

        auto* scaledView = dynamic_cast<PartDesignGui::ViewProviderScaled*>(transformedView);
        QVERIFY(scaledView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgScaledParameters(scaledView);
        QPointer<PartDesignGui::TaskDlgScaledParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskScaledParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* factor = findQuantitySpinBox(taskBox, QStringLiteral("spinFactor"));
        QVERIFY(factor != nullptr);

        PartDesign::BlockingScaledTest::armBlocker();
        factor->setValue(factor->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingScaledTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingScaledTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void scaledCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Scaled);

        auto* scaledView = dynamic_cast<PartDesignGui::ViewProviderScaled*>(transformedView);
        QVERIFY(scaledView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgScaledParameters(scaledView);
        QPointer<PartDesignGui::TaskDlgScaledParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskScaledParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* occurrences = findUIntSpinBox(taskBox, QStringLiteral("spinOccurrences"));
        QVERIFY(occurrences != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingScaledTest::armBlocker();
        occurrences->setValue(occurrences->value() + 1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingScaledTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingScaledTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingScaledTest::getExecutionCount(), 1);
    }

    void scaledAcceptWaitsForQueuedPreviewWithoutExtraRerun()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::Scaled);

        auto* scaledView = dynamic_cast<PartDesignGui::ViewProviderScaled*>(transformedView);
        QVERIFY(scaledView != nullptr);
        auto* scaled = scaledView->getObject<PartDesign::Scaled>();
        QVERIFY(scaled != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgScaledParameters(scaledView);
        QPointer<PartDesignGui::TaskDlgScaledParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskScaledParameters>(dialog);
        QVERIFY(taskBox != nullptr);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);

        auto* factor = findQuantitySpinBox(taskBox, QStringLiteral("spinFactor"));
        QVERIFY(factor != nullptr);

        const double initialFactor = factor->rawValue();
        const double firstFactor = initialFactor + 0.5;
        const double finalFactor = firstFactor + 0.25;

        PartDesign::BlockingScaledTest::armBlocker();
        factor->setValue(firstFactor);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingScaledTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        factor->setValue(finalFactor);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingScaledTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 2);
        QCOMPARE(scaled->Factor.getValue(), finalFactor);
    }

    void multiTransformScaledRejectDefersCloseUntilAsyncPreviewSettles()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::MultiTransformScaled);

        auto* multiTransformView = dynamic_cast<PartDesignGui::ViewProviderMultiTransform*>(
            transformedView
        );
        QVERIFY(multiTransformView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMultiTransformParameters(multiTransformView);
        QPointer<PartDesignGui::TaskDlgMultiTransformParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMultiTransformParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* list = findListWidget(taskBox, QStringLiteral("listTransformFeatures"));
        QVERIFY(list != nullptr);
        QVERIFY(list->count() >= 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(taskBox, "onTransformEdit", Qt::DirectConnection));
        QCoreApplication::processEvents();

        auto* occurrences = findUIntSpinBox(taskBox, QStringLiteral("spinOccurrences"));
        QVERIFY(occurrences != nullptr);

        PartDesign::BlockingMultiTransformTest::armBlocker();
        occurrences->setValue(occurrences->value() + 1);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());

        Gui::Control().reject(doc);

        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(guiDoc->hasPendingCommand());

        PartDesign::BlockingMultiTransformTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
    }

    void multiTransformScaledCancelPreviewStopsAsyncRunWithoutClosingDialog()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::MultiTransformScaled);

        auto* multiTransformView = dynamic_cast<PartDesignGui::ViewProviderMultiTransform*>(
            transformedView
        );
        QVERIFY(multiTransformView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMultiTransformParameters(multiTransformView);
        QPointer<PartDesignGui::TaskDlgMultiTransformParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMultiTransformParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* list = findListWidget(taskBox, QStringLiteral("listTransformFeatures"));
        QVERIFY(list != nullptr);
        QVERIFY(list->count() >= 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(taskBox, "onTransformEdit", Qt::DirectConnection));
        QCoreApplication::processEvents();

        auto* factor = findQuantitySpinBox(taskBox, QStringLiteral("spinFactor"));
        QVERIFY(factor != nullptr);

        auto* cancelPreview = findCancelPreviewButton(taskBox);
        QVERIFY(cancelPreview != nullptr);

        PartDesign::BlockingMultiTransformTest::armBlocker();
        factor->setValue(factor->rawValue() + 0.5);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1, 3000);
        QVERIFY(taskBox->hasOutstandingRecompute());
        QVERIFY(cancelPreview->isEnabled());

        cancelPreview->click();
        QCoreApplication::processEvents();

        QVERIFY(taskBox->hasOutstandingRecompute());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));

        PartDesign::BlockingMultiTransformTest::releaseBlocker();

        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QCOMPARE(Gui::Control().activeDialog(doc), static_cast<Gui::TaskView::TaskDialog*>(dialog));
        QCOMPARE(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1);
    }

    void multiTransformPolarQueuedPreviewReplaysAfterSettle()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::MultiTransformPolar);

        auto* multiTransformView = dynamic_cast<PartDesignGui::ViewProviderMultiTransform*>(
            transformedView
        );
        QVERIFY(multiTransformView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMultiTransformParameters(multiTransformView);
        QPointer<PartDesignGui::TaskDlgMultiTransformParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMultiTransformParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* list = findListWidget(taskBox, QStringLiteral("listTransformFeatures"));
        QVERIFY(list != nullptr);
        QVERIFY(list->count() >= 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(taskBox, "onTransformEdit", Qt::DirectConnection));
        QCoreApplication::processEvents();

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        PartDesign::BlockingMultiTransformTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        extent->setValue(extent->rawValue() + 15.0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        PartDesign::BlockingMultiTransformTest::releaseBlocker();

        QTRY_COMPARE_WITH_TIMEOUT(getBlockingTransformedTotalExecutionCount(), 2, 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
    }

    void multiTransformPolarSubTaskOkWaitsForQueuedPreview()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::MultiTransformPolar);

        auto* multiTransformView = dynamic_cast<PartDesignGui::ViewProviderMultiTransform*>(
            transformedView
        );
        QVERIFY(multiTransformView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMultiTransformParameters(multiTransformView);
        QPointer<PartDesignGui::TaskDlgMultiTransformParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMultiTransformParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* list = findListWidget(taskBox, QStringLiteral("listTransformFeatures"));
        QVERIFY(list != nullptr);
        QVERIFY(list->count() >= 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(taskBox, "onTransformEdit", Qt::DirectConnection));
        QCoreApplication::processEvents();

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        auto* subTaskOk = taskBox->findChild<QPushButton*>(QStringLiteral("buttonOK"));
        QVERIFY(subTaskOk != nullptr);

        PartDesign::BlockingMultiTransformTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        extent->setValue(extent->rawValue() + 15.0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingMultiTransformTest::releaseBlocker();
        });

        subTaskOk->click();
        releaser.join();

        QTRY_COMPARE_WITH_TIMEOUT(getBlockingTransformedTotalExecutionCount(), 2, 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!taskBox->hasOutstandingRecompute(), 3000);
        QVERIFY(!guard.isNull());
        QVERIFY(taskBox->findChild<PartGui::PatternParametersWidget*>() == nullptr);
    }

    void multiTransformAcceptFlushesOpenSubTaskQueuedPreview()  // NOLINT
    {
        prepareTransformedFixture(TransformedKind::MultiTransformPolar);

        auto* multiTransformView = dynamic_cast<PartDesignGui::ViewProviderMultiTransform*>(
            transformedView
        );
        QVERIFY(multiTransformView != nullptr);

        auto* dialog = new PartDesignGui::TaskDlgMultiTransformParameters(multiTransformView);
        QPointer<PartDesignGui::TaskDlgMultiTransformParameters> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskBox = findTaskBox<PartDesignGui::TaskMultiTransformParameters>(dialog);
        QVERIFY(taskBox != nullptr);

        auto* list = findListWidget(taskBox, QStringLiteral("listTransformFeatures"));
        QVERIFY(list != nullptr);
        QVERIFY(list->count() >= 1);
        list->setCurrentRow(0);
        QVERIFY(QMetaObject::invokeMethod(taskBox, "onTransformEdit", Qt::DirectConnection));
        QCoreApplication::processEvents();

        auto* widget = findPrimaryPatternParametersWidget(taskBox);
        QVERIFY(widget != nullptr);

        auto* extent = findExtentSpinBox(widget);
        QVERIFY(extent != nullptr);

        PartDesign::BlockingMultiTransformTest::armBlocker();
        extent->setValue(extent->rawValue() - 45.0);
        QCoreApplication::processEvents();

        QTRY_COMPARE_WITH_TIMEOUT(PartDesign::BlockingMultiTransformTest::getExecutionCount(), 1, 3000);
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 1);
        QVERIFY(taskBox->hasOutstandingRecompute());

        extent->setValue(extent->rawValue() + 15.0);
        QCoreApplication::processEvents();
        QVERIFY(taskBox->hasOutstandingRecompute());

        std::thread releaser([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            PartDesign::BlockingMultiTransformTest::releaseBlocker();
        });

        Gui::Control().accept(doc);
        releaser.join();

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(getBlockingTransformedTotalExecutionCount(), 3);
    }

private:
    void prepareTransformedFixture(TransformedKind kind)
    {
        traceTransformedTest("prepareTransformedFixture: begin");
        switch (kind) {
            case TransformedKind::Linear: {
                traceTransformedTest("prepareTransformedFixture: Linear begin");
                traceTransformedTest("prepareTransformedFixture: Linear before blocking addObject");
                auto* pattern = doc->addObject<PartDesign::BlockingLinearPatternTest>(
                    "BlockingLinearPattern"
                );
                traceTransformedTest("prepareTransformedFixture: Linear after blocking addObject");
                QVERIFY(pattern != nullptr);
                traceTransformedTest(
                    "prepareTransformedFixture: Linear before blocking body addObject"
                );
                body->addObject(pattern);
                traceTransformedTest("prepareTransformedFixture: Linear after blocking body addObject");
                traceTransformedTest(
                    "prepareTransformedFixture: Linear before blocking property setup"
                );
                pattern->Originals.setValues({baseBox});
                pattern->Direction.setValue(baseBox, {"Edge1"});
                pattern->Length.setValue(20.0);
                pattern->Occurrences.setValue(2);
                // Transformed also declares Refine; refineShapeIfActive() reads the base property.
                static_cast<PartDesign::FeatureRefine*>(pattern)->Refine.setValue(false);
                traceTransformedTest("prepareTransformedFixture: Linear after blocking property setup");
                traceTransformedTest("prepareTransformedFixture: Linear before blocking doc recompute");
                doc->recompute();
                traceTransformedTest("prepareTransformedFixture: Linear after blocking doc recompute");
                traceTransformedTest("prepareTransformedFixture: Linear before getViewProvider");
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(pattern)
                );
                traceTransformedTest("prepareTransformedFixture: Linear after getViewProvider");
                traceTransformedTest("prepareTransformedFixture: Linear done");
                break;
            }
            case TransformedKind::Polar: {
                auto* pattern = doc->addObject<PartDesign::BlockingPolarPatternTest>(
                    "BlockingPolarPattern"
                );
                QVERIFY(pattern != nullptr);
                body->addObject(pattern);
                pattern->Originals.setValues({baseBox});
                pattern->Axis.setValue(bodyXAxis, {""});
                pattern->Angle.setValue(270.0);
                pattern->Occurrences.setValue(3);
                doc->recompute();
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(pattern)
                );
                break;
            }
            case TransformedKind::Mirrored: {
                auto* mirrored = doc->addObject<PartDesign::BlockingMirroredTest>("BlockingMirrored");
                QVERIFY(mirrored != nullptr);
                body->addObject(mirrored);
                mirrored->Originals.setValues({baseBox});
                mirrored->MirrorPlane.setValue(bodyXYPlane, {""});
                doc->recompute();
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(mirrored)
                );
                break;
            }
            case TransformedKind::Scaled: {
                auto* scaled = doc->addObject<PartDesign::BlockingScaledTest>("BlockingScaled");
                QVERIFY(scaled != nullptr);
                body->addObject(scaled);
                scaled->Originals.setValues({baseBox});
                scaled->Factor.setValue(2.0);
                scaled->Occurrences.setValue(2);
                doc->recompute();
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(scaled)
                );
                break;
            }
            case TransformedKind::MultiTransformScaled: {
                auto* scaled = doc->addObject<PartDesign::Scaled>("MultiTransformScaled");
                QVERIFY(scaled != nullptr);
                body->addObject(scaled);
                scaled->Originals.setValues({baseBox});
                scaled->Factor.setValue(2.0);
                scaled->Occurrences.setValue(2);
                doc->recompute();

                auto* multiTransform = doc->addObject<PartDesign::BlockingMultiTransformTest>(
                    "BlockingMultiTransform"
                );
                QVERIFY(multiTransform != nullptr);
                body->addObject(multiTransform);
                multiTransform->Originals.setValues({baseBox});
                multiTransform->Transformations.setValues({scaled});
                scaled->Originals.setValues({});
                doc->recompute();
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(multiTransform)
                );
                break;
            }
            case TransformedKind::MultiTransformPolar: {
                auto* polar = doc->addObject<PartDesign::PolarPattern>("MultiTransformPolar");
                QVERIFY(polar != nullptr);
                body->addObject(polar);
                polar->Originals.setValues({baseBox});
                polar->Axis.setValue(bodyXAxis, {""});
                polar->Angle.setValue(270.0);
                polar->Occurrences.setValue(3);
                doc->recompute();

                auto* multiTransform = doc->addObject<PartDesign::BlockingMultiTransformTest>(
                    "BlockingMultiTransform"
                );
                QVERIFY(multiTransform != nullptr);
                body->addObject(multiTransform);
                multiTransform->Originals.setValues({baseBox});
                multiTransform->Transformations.setValues({polar});
                polar->Originals.setValues({});
                doc->recompute();
                transformedView = dynamic_cast<PartDesignGui::ViewProviderTransformed*>(
                    guiDoc->getViewProvider(multiTransform)
                );
                break;
            }
        }

        traceTransformedTest("prepareTransformedFixture: after switch");
        QVERIFY(transformedView != nullptr);
        traceTransformedTest("prepareTransformedFixture: transformedView verified");
        traceTransformedTest("prepareTransformedFixture: before openCommand");
        guiDoc->openCommand("Edit blocking transformed");
        traceTransformedTest("prepareTransformedFixture: done");
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    PartDesign::Body* body = nullptr;
    PartDesign::AdditiveBox* baseBox = nullptr;
    App::Line* bodyXAxis = nullptr;
    App::Plane* bodyXYPlane = nullptr;
    App::Plane* bodyYZPlane = nullptr;
    PartDesignGui::ViewProviderTransformed* transformedView = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskTransformedParametersTest(int argc, char** argv)
{
    testTaskTransformedParameters test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskTransformedParametersTest.moc"
