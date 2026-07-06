// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#include <chrono>
#include <condition_variable>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>

#include <App/Application.h>
#include <App/Document.h>
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
#include <Gui/Widgets.h>
#include <Mod/Part/App/FeatureOffset.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PartFeatures.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/Gui/TaskLoft.h>
#include <Mod/Part/Gui/TaskOffset.h>
#include <Mod/Part/Gui/TaskSweep.h>
#include <Mod/Part/Gui/TaskThickness.h>

#include <src/App/InitApplication.h>

namespace Part
{

class BlockingOffsetTest: public Part::Offset
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::BlockingOffsetTest);

public:
    BlockingOffsetTest() = default;
    ~BlockingOffsetTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingThicknessTest: public Part::Thickness
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::BlockingThicknessTest);

public:
    BlockingThicknessTest() = default;
    ~BlockingThicknessTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

class BlockingLineTest: public Part::Line
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::BlockingLineTest);

public:
    BlockingLineTest() = default;
    ~BlockingLineTest() override = default;

    static void resetBlocker();
    static void armBlocker();
    static int getExecutionCount();
    static void releaseBlocker();

    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace Part

namespace
{

using namespace std::chrono_literals;

class TestGuiApplication: public Gui::Application
{
public:
    explicit TestGuiApplication(bool guiEnabled)
        : Gui::Application(guiEnabled)
    {}
};

struct GuiHarness
{
    explicit GuiHarness(std::initializer_list<void (*)()> initFunctions = {})
        : app(true)
    {
        Gui::Application::initApplication();
        Gui::Application::initOpenInventor();
        Base::Interpreter().runString("import FreeCAD as App\nimport FreeCADGui as Gui");
        Base::Interpreter().runString("import Part");
        Base::Interpreter().runString("import PartGui");

        for (auto initFunction : initFunctions) {
            if (initFunction) {
                initFunction();
            }
        }

        mainWindow = new Gui::MainWindow();
        mainWindow->hide();

        if (!Gui::DockWindowManager::instance()->getDockWindow("Tasks")) {
            auto* taskView = new Gui::TaskView::TaskView(Gui::getMainWindow());
            taskView->setWindowTitle(QStringLiteral("Tasks"));
            Gui::DockWindowManager::instance()->addDockWindow("Tasks", taskView, Qt::RightDockWidgetArea);
        }
    }

    TestGuiApplication app;
    Gui::MainWindow* mainWindow = nullptr;
};

GuiHarness& ensureGuiHarness(std::initializer_list<void (*)()> initFunctions = {})
{
    static GuiHarness harness(initFunctions);
    return harness;
}

class BlockingExecutionState
{
public:
    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex);
        armed = false;
        proceed = true;
        executionCount = 0;
    }

    void arm()
    {
        std::lock_guard<std::mutex> lock(mutex);
        armed = true;
        proceed = false;
        executionCount = 0;
    }

    int getExecutionCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return executionCount;
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            proceed = true;
        }
        changed.notify_all();
    }

    template<typename ExecuteBase>
    App::DocumentObjectExecReturn* execute(ExecuteBase&& executeBase)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (armed) {
                armed = false;
                ++executionCount;
                changed.notify_all();
                changed.wait(lock, [this] { return proceed; });
            }
        }

        if (App::currentRecomputeWasCanceled()) {
            throw Base::UserAbortException();
        }

        return std::forward<ExecuteBase>(executeBase)();
    }

private:
    std::mutex mutex;
    std::condition_variable changed;
    bool armed = false;
    bool proceed = true;
    int executionCount = 0;
};

BlockingExecutionState& blockingOffsetState()
{
    static BlockingExecutionState state;
    return state;
}

BlockingExecutionState& blockingThicknessState()
{
    static BlockingExecutionState state;
    return state;
}

BlockingExecutionState& blockingLineState()
{
    static BlockingExecutionState state;
    return state;
}

template<typename TaskWidget>
TaskWidget* findTaskWidget(Gui::TaskView::TaskDialog* dialog)
{
    if (!dialog) {
        return nullptr;
    }

    for (QWidget* widget : dialog->getDialogContent()) {
        if (auto* taskWidget = qobject_cast<TaskWidget*>(widget)) {
            return taskWidget;
        }
        if (auto* taskWidget = widget->findChild<TaskWidget*>()) {
            return taskWidget;
        }
    }

    return nullptr;
}

QDialogButtonBox* findActiveDialogButtonBox()
{
    auto* taskView = Gui::Control().taskPanel();
    return taskView ? taskView->findChild<QDialogButtonBox*>() : nullptr;
}

Gui::QuantitySpinBox* findOffsetSpinBox(QWidget* taskWidget)
{
    return taskWidget ? taskWidget->findChild<Gui::QuantitySpinBox*>(QStringLiteral("spinOffset"))
                      : nullptr;
}

QCheckBox* findUpdateViewCheckBox(QWidget* taskWidget)
{
    return taskWidget ? taskWidget->findChild<QCheckBox*>(QStringLiteral("updateView")) : nullptr;
}

void disablePreviewUpdates(QWidget* taskWidget)
{
    auto* updateView = findUpdateViewCheckBox(taskWidget);
    QVERIFY(updateView != nullptr);
    updateView->setChecked(false);
    QCoreApplication::processEvents();
}

void acceptWithInlineProgress(
    App::Document* document,
    QWidget* taskWidget,
    const QString& expectedStatus,
    std::function<void()> releaseBlocker
)
{
    int timerTicksWhileAccepting = 0;
    bool sawModalProgressDialog = false;
    bool sawInlineAcceptStatus = false;
    bool sawDisabledDialogButtons = false;
    QString modalErrorText;

    QPointer<QWidget> taskWidgetGuard(taskWidget);
    QTimer responsivenessTimer;
    responsivenessTimer.setInterval(10);
    QObject::connect(&responsivenessTimer, &QTimer::timeout, [&]() {
        ++timerTicksWhileAccepting;

        for (auto* widget : QApplication::topLevelWidgets()) {
            if (qobject_cast<QProgressDialog*>(widget) && widget->isVisible()) {
                sawModalProgressDialog = true;
            }
            if (auto* messageBox = qobject_cast<QMessageBox*>(widget);
                messageBox && messageBox->isVisible()) {
                modalErrorText = messageBox->text();
                if (!messageBox->informativeText().isEmpty()) {
                    modalErrorText += QStringLiteral(": ");
                    modalErrorText += messageBox->informativeText();
                }
                QTimer::singleShot(0, messageBox, &QMessageBox::reject);
            }
        }

        if (taskWidgetGuard) {
            auto* statusLabel = taskWidgetGuard->findChild<QLabel*>(
                QStringLiteral("labelPreviewStatus")
            );
            auto* statusWidget = taskWidgetGuard->findChild<QWidget*>(
                QStringLiteral("previewStatusWidget")
            );
            if (statusLabel && statusWidget && !statusLabel->isHidden() && !statusWidget->isHidden()
                && statusLabel->text().contains(expectedStatus)) {
                sawInlineAcceptStatus = true;
            }

            auto* acceptStatusLabel = taskWidgetGuard->findChild<QLabel*>(
                QStringLiteral("fcAsyncAcceptProgressLabel")
            );
            auto* acceptStatusWidget = taskWidgetGuard->findChild<QWidget*>(
                QStringLiteral("fcAsyncAcceptProgress")
            );
            if (acceptStatusLabel && acceptStatusWidget && !acceptStatusLabel->isHidden()
                && !acceptStatusWidget->isHidden()
                && acceptStatusLabel->text().contains(expectedStatus)) {
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

    std::thread releaser([releaseBlocker = std::move(releaseBlocker)]() {
        std::this_thread::sleep_for(150ms);
        releaseBlocker();
    });

    std::string acceptError;
    try {
        Gui::Control().accept(document);
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
    QVERIFY2(modalErrorText.isEmpty(), qPrintable(modalErrorText));
    QVERIFY2(
        timerTicksWhileAccepting > 0,
        "accept should keep the GUI event loop responsive while final recompute settles"
    );
    QVERIFY2(
        !sawModalProgressDialog,
        "Part task accept should use inline task-panel status instead of a modal progress dialog"
    );
    QVERIFY2(
        sawInlineAcceptStatus,
        "Part task accept should show inline task-panel feedback while final recompute settles"
    );
    QVERIFY2(
        sawDisabledDialogButtons,
        "Part task accept should disable task dialog buttons until final recompute settles"
    );
}

Gui::ActionSelector* findActionSelector(QWidget* taskWidget)
{
    return taskWidget ? taskWidget->findChild<Gui::ActionSelector*>(QStringLiteral("selector"))
                      : nullptr;
}

void moveAvailableObjectToSelected(QWidget* taskWidget, const char* objectName)
{
    auto* selector = findActionSelector(taskWidget);
    QVERIFY(selector != nullptr);

    auto* available = selector->availableTreeWidget();
    auto* selected = selector->selectedTreeWidget();
    QVERIFY(available != nullptr);
    QVERIFY(selected != nullptr);

    const QString expectedName = QString::fromLatin1(objectName);
    for (int i = 0; i < available->topLevelItemCount(); ++i) {
        auto* item = available->topLevelItem(i);
        if (item && item->data(0, Qt::UserRole).toString() == expectedName) {
            item = available->takeTopLevelItem(i);
            selected->addTopLevelItem(item);
            selected->setCurrentItem(item);
            return;
        }
    }

    QFAIL(qPrintable(
        QStringLiteral("Object %1 was not available in the task selector").arg(expectedName)
    ));
}

void setCreateSolid(QWidget* taskWidget, bool createSolid)
{
    auto* checkSolid = taskWidget ? taskWidget->findChild<QCheckBox*>(QStringLiteral("checkSolid"))
                                  : nullptr;
    QVERIFY(checkSolid != nullptr);
    checkSolid->setChecked(createSolid);
    QCoreApplication::processEvents();
}

void setFrenet(QWidget* taskWidget, bool frenet)
{
    auto* checkFrenet = taskWidget ? taskWidget->findChild<QCheckBox*>(QStringLiteral("checkFrenet"))
                                   : nullptr;
    QVERIFY(checkFrenet != nullptr);
    checkFrenet->setChecked(frenet);
    QCoreApplication::processEvents();
}

void setLine(Part::Line* line, double x1, double y1, double z1, double x2, double y2, double z2)
{
    QVERIFY(line != nullptr);
    line->X1.setValue(x1);
    line->Y1.setValue(y1);
    line->Z1.setValue(z1);
    line->X2.setValue(x2);
    line->Y2.setValue(y2);
    line->Z2.setValue(z2);
}

}  // namespace

PROPERTY_SOURCE(Part::BlockingOffsetTest, Part::Offset)

void Part::BlockingOffsetTest::resetBlocker()
{
    blockingOffsetState().reset();
}

void Part::BlockingOffsetTest::armBlocker()
{
    blockingOffsetState().arm();
}

int Part::BlockingOffsetTest::getExecutionCount()
{
    return blockingOffsetState().getExecutionCount();
}

void Part::BlockingOffsetTest::releaseBlocker()
{
    blockingOffsetState().release();
}

App::DocumentObjectExecReturn* Part::BlockingOffsetTest::execute()
{
    return blockingOffsetState().execute([this]() {
        auto* source = Source.getValue();
        if (!source) {
            return new App::DocumentObjectExecReturn("No source shape linked.");
        }

        auto shape = Part::Feature::getTopoShape(
            source,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn("Invalid source link");
        }

        Shape.setValue(shape);
        return App::DocumentObject::StdReturn;
    });
}

PROPERTY_SOURCE(Part::BlockingThicknessTest, Part::Thickness)

void Part::BlockingThicknessTest::resetBlocker()
{
    blockingThicknessState().reset();
}

void Part::BlockingThicknessTest::armBlocker()
{
    blockingThicknessState().arm();
}

int Part::BlockingThicknessTest::getExecutionCount()
{
    return blockingThicknessState().getExecutionCount();
}

void Part::BlockingThicknessTest::releaseBlocker()
{
    blockingThicknessState().release();
}

App::DocumentObjectExecReturn* Part::BlockingThicknessTest::execute()
{
    return blockingThicknessState().execute([this]() {
        auto* source = Faces.getValue();
        if (!source) {
            return new App::DocumentObjectExecReturn("No source shape linked.");
        }

        auto shape = Part::Feature::getTopoShape(
            source,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn("Invalid source link");
        }

        Shape.setValue(shape);
        return App::DocumentObject::StdReturn;
    });
}

PROPERTY_SOURCE(Part::BlockingLineTest, Part::Line)

void Part::BlockingLineTest::resetBlocker()
{
    blockingLineState().reset();
}

void Part::BlockingLineTest::armBlocker()
{
    blockingLineState().arm();
}

int Part::BlockingLineTest::getExecutionCount()
{
    return blockingLineState().getExecutionCount();
}

void Part::BlockingLineTest::releaseBlocker()
{
    blockingLineState().release();
}

App::DocumentObjectExecReturn* Part::BlockingLineTest::execute()
{
    return blockingLineState().execute([this]() { return Part::Line::execute(); });
}

class testTaskInlineAcceptProgress final: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        tests::initApplication();
        Base::Interpreter().runString("import Part");
        Q_UNUSED(ensureGuiHarness(
            {&Part::BlockingOffsetTest::init,
             &Part::BlockingThicknessTest::init,
             &Part::BlockingLineTest::init}
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
            {&Part::BlockingOffsetTest::init,
             &Part::BlockingThicknessTest::init,
             &Part::BlockingLineTest::init}
        ));

        Part::BlockingOffsetTest::resetBlocker();
        Part::BlockingThicknessTest::resetBlocker();
        Part::BlockingLineTest::resetBlocker();

        docName = App::GetApplication().getUniqueDocumentName("part_inline_accept_progress");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        guiDoc = Gui::Application::Instance->getDocument(doc);
        QVERIFY(guiDoc != nullptr);
        App::GetApplication().setActiveDocument(doc);
        Gui::Application::Instance->setActiveDocument(guiDoc);

        sourceBox = doc->addObject<Part::Box>("SourceBox");
        QVERIFY(sourceBox != nullptr);
        sourceBox->Length.setValue(10.0);
        sourceBox->Width.setValue(10.0);
        sourceBox->Height.setValue(10.0);
        doc->recompute();
    }

    void cleanup()  // NOLINT
    {
        Part::BlockingOffsetTest::releaseBlocker();
        Part::BlockingThicknessTest::releaseBlocker();
        Part::BlockingLineTest::releaseBlocker();
        QCoreApplication::processEvents();

        if (doc && Gui::Control().activeDialog(doc)) {
            Gui::Control().closeDialog(doc);
            QCoreApplication::processEvents();
        }

        Gui::Selection().rmvSelectionGate();
        Gui::Selection().clearSelection();
        QCoreApplication::processEvents();

        if (!docName.empty() && App::GetApplication().getDocument(docName.c_str())) {
            App::GetApplication().closeDocument(docName.c_str());
        }

        thickness = nullptr;
        offset = nullptr;
        sourceBox = nullptr;
        guiDoc = nullptr;
        doc = nullptr;
        docName.clear();
    }

    void offsetAcceptUsesInlineProgressInsteadOfModalDialog()  // NOLINT
    {
        offset = doc->addObject<Part::BlockingOffsetTest>("BlockingOffset");
        QVERIFY(offset != nullptr);
        offset->Source.setValue(sourceBox);
        offset->Value.setValue(0.25);
        doc->recompute();

        guiDoc->openCommand("Edit blocking offset");
        auto* dialog = new PartGui::TaskOffset(offset);
        QPointer<PartGui::TaskOffset> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskWidget = findTaskWidget<PartGui::OffsetWidget>(dialog);
        QVERIFY(taskWidget != nullptr);
        disablePreviewUpdates(taskWidget);

        auto* offsetSpin = findOffsetSpinBox(taskWidget);
        QVERIFY(offsetSpin != nullptr);

        Part::BlockingOffsetTest::armBlocker();
        offsetSpin->setValue(offsetSpin->rawValue() + 0.1);
        QCoreApplication::processEvents();

        acceptWithInlineProgress(doc, taskWidget, QStringLiteral("Computing offset"), []() {
            Part::BlockingOffsetTest::releaseBlocker();
        });

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(Part::BlockingOffsetTest::getExecutionCount(), 1);
    }

    void thicknessAcceptUsesInlineProgressInsteadOfModalDialog()  // NOLINT
    {
        thickness = doc->addObject<Part::BlockingThicknessTest>("BlockingThickness");
        QVERIFY(thickness != nullptr);
        thickness->Faces.setValue(sourceBox, std::vector<std::string> {"Face1"});
        thickness->Value.setValue(0.25);
        doc->recompute();

        guiDoc->openCommand("Edit blocking thickness");
        auto* dialog = new PartGui::TaskThickness(thickness);
        QPointer<PartGui::TaskThickness> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskWidget = findTaskWidget<PartGui::ThicknessWidget>(dialog);
        QVERIFY(taskWidget != nullptr);
        disablePreviewUpdates(taskWidget);

        auto* offsetSpin = findOffsetSpinBox(taskWidget);
        QVERIFY(offsetSpin != nullptr);

        Part::BlockingThicknessTest::armBlocker();
        offsetSpin->setValue(offsetSpin->rawValue() + 0.1);
        QCoreApplication::processEvents();

        acceptWithInlineProgress(doc, taskWidget, QStringLiteral("Computing thickness"), []() {
            Part::BlockingThicknessTest::releaseBlocker();
        });

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(Part::BlockingThicknessTest::getExecutionCount(), 1);
    }

    void loftAcceptUsesInlineProgressInsteadOfModalDialog()  // NOLINT
    {
        auto* blockingProfile = doc->addObject<Part::BlockingLineTest>("BlockingLoftProfile");
        QVERIFY(blockingProfile != nullptr);
        setLine(blockingProfile, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0);

        auto* secondProfile = doc->addObject<Part::Line>("LoftProfile");
        QVERIFY(secondProfile != nullptr);
        setLine(secondProfile, 0.0, 0.0, 5.0, 10.0, 0.0, 5.0);
        doc->recompute();

        auto* dialog = new PartGui::TaskLoft();
        QPointer<PartGui::TaskLoft> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskWidget = findTaskWidget<PartGui::LoftWidget>(dialog);
        QVERIFY(taskWidget != nullptr);
        setCreateSolid(taskWidget, false);
        moveAvailableObjectToSelected(taskWidget, blockingProfile->getNameInDocument());
        moveAvailableObjectToSelected(taskWidget, secondProfile->getNameInDocument());

        Part::BlockingLineTest::armBlocker();
        blockingProfile->enforceRecompute();

        acceptWithInlineProgress(doc, taskWidget, QStringLiteral("Computing loft"), []() {
            Part::BlockingLineTest::releaseBlocker();
        });

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(Part::BlockingLineTest::getExecutionCount(), 1);
        QVERIFY(doc->getObject("Loft") != nullptr);
    }

    void sweepAcceptUsesInlineProgressInsteadOfModalDialog()  // NOLINT
    {
        auto* blockingProfile = doc->addObject<Part::BlockingLineTest>("BlockingSweepProfile");
        QVERIFY(blockingProfile != nullptr);
        setLine(blockingProfile, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0);

        auto* sweepPath = doc->addObject<Part::Line>("SweepPath");
        QVERIFY(sweepPath != nullptr);
        setLine(sweepPath, 0.0, 0.0, 0.0, 5.0, 0.0, 0.0);
        doc->recompute();

        auto* dialog = new PartGui::TaskSweep();
        QPointer<PartGui::TaskSweep> guard(dialog);
        Gui::Control().showDialog(dialog, doc);
        QCoreApplication::processEvents();

        auto* taskWidget = findTaskWidget<PartGui::SweepWidget>(dialog);
        QVERIFY(taskWidget != nullptr);
        setCreateSolid(taskWidget, false);
        setFrenet(taskWidget, false);
        moveAvailableObjectToSelected(taskWidget, blockingProfile->getNameInDocument());

        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(docName.c_str(), sweepPath->getNameInDocument());
        QCoreApplication::processEvents();

        Part::BlockingLineTest::armBlocker();
        blockingProfile->enforceRecompute();

        acceptWithInlineProgress(doc, taskWidget, QStringLiteral("Computing sweep"), []() {
            Part::BlockingLineTest::releaseBlocker();
        });

        QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 3000);
        QCOMPARE(Gui::Control().activeDialog(doc), nullptr);
        QVERIFY(!guiDoc->hasPendingCommand());
        QCOMPARE(Part::BlockingLineTest::getExecutionCount(), 1);
        QVERIFY(doc->getObject("Sweep") != nullptr);
    }

private:
    Base::Reference<ParameterGrp> asyncParams;
    bool oldAsyncEnabled = true;
    Base::Reference<ParameterGrp> gizmoParams;
    bool oldGizmosEnabled = true;
    std::string docName;
    App::Document* doc = nullptr;
    Gui::Document* guiDoc = nullptr;
    Part::Box* sourceBox = nullptr;
    Part::BlockingOffsetTest* offset = nullptr;
    Part::BlockingThicknessTest* thickness = nullptr;
};

extern "C" Q_DECL_EXPORT int runTaskInlineAcceptProgressTest(int argc, char** argv)
{
    testTaskInlineAcceptProgress test;
    return QTest::qExec(&test, argc, argv);
}

#include "TaskInlineAcceptProgressTest.moc"
