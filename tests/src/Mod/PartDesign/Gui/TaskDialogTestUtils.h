// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <utility>

#include <QComboBox>
#include <QPushButton>

#include <App/Application.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace tests
{
namespace partdesigngui
{

using GuiInitFunction = void (*)();

class TestGuiApplication: public Gui::Application
{
public:
    explicit TestGuiApplication(bool guiEnabled)
        : Gui::Application(guiEnabled)
    {}
};

struct GuiHarness
{
    explicit GuiHarness(std::initializer_list<GuiInitFunction> initFunctions = {})
        : app(true)
    {
        Gui::Application::initApplication();
        Gui::Application::initOpenInventor();
        Base::Interpreter().runString("import FreeCAD as App\nimport FreeCADGui as Gui");
        Base::Interpreter().runString("import PartDesignGui");

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

inline GuiHarness& ensureGuiHarness(std::initializer_list<GuiInitFunction> initFunctions = {})
{
    static GuiHarness harness(initFunctions);
    return harness;
}

template<typename TaskBoxType>
TaskBoxType* findTaskBox(Gui::TaskView::TaskDialog* dialog)
{
    for (QWidget* widget : dialog->getDialogContent()) {
        if (auto* taskBox = qobject_cast<TaskBoxType*>(widget)) {
            return taskBox;
        }
    }

    return nullptr;
}

inline QPushButton* findCancelPreviewButton(QWidget* taskBox)
{
    return taskBox ? taskBox->findChild<QPushButton*>(QStringLiteral("buttonCancelPreview"))
                   : nullptr;
}

inline bool activateComboIndex(QComboBox* combo, int index)
{
    if (!combo || index < 0 || index >= combo->count()) {
        return false;
    }

    combo->setCurrentIndex(index);
    return true;
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
        totalExecutionCount = 0;
    }

    void arm()
    {
        std::lock_guard<std::mutex> lock(mutex);
        armed = true;
        proceed = false;
        executionCount = 0;
        totalExecutionCount = 0;
    }

    int getExecutionCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return executionCount;
    }

    int getTotalExecutionCount()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return totalExecutionCount;
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
            ++totalExecutionCount;
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
    int totalExecutionCount = 0;
};

}  // namespace partdesigngui
}  // namespace tests

#define FC_TEST_DECLARE_BLOCKING_EXECUTION_STATE(Name) \
    tests::partdesigngui::BlockingExecutionState& Name() \
    { \
        static tests::partdesigngui::BlockingExecutionState state; \
        return state; \
    }

#define FC_TEST_DEFINE_BLOCKING_EXECUTION_TEST_METHODS(TestClass, BaseClass, StateAccessor) \
    PROPERTY_SOURCE(TestClass, BaseClass) \
\
    void TestClass::resetBlocker() \
    { \
        StateAccessor().reset(); \
    } \
\
    void TestClass::armBlocker() \
    { \
        StateAccessor().arm(); \
    } \
\
    int TestClass::getExecutionCount() \
    { \
        return StateAccessor().getExecutionCount(); \
    } \
\
    int TestClass::getTotalExecutionCount() \
    { \
        return StateAccessor().getTotalExecutionCount(); \
    } \
\
    void TestClass::releaseBlocker() \
    { \
        StateAccessor().release(); \
    } \
\
    App::DocumentObjectExecReturn* TestClass::execute() \
    { \
        return StateAccessor().execute([this]() { return BaseClass::execute(); }); \
    }
