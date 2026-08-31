// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <algorithm>
#include <chrono>
#include <string>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>

#include <src/App/InitApplication.h>

namespace GuiTest
{

/**
 * Initialize the application and ensure that a GUI application singleton
 * exists for a Qt GUI test.
 *
 * The default keeps the application in the same mode as the existing GUI
 * tests. Pass true when the test explicitly needs GUI-enabled application
 * state.
 */
inline void ensureGuiApplication(bool guiEnabled = false)
{
    tests::initApplication();
    if (!Gui::Application::Instance) {
        new Gui::Application(guiEnabled);
    }
}

/**
 * Poll a condition while processing Qt events.
 *
 * The predicate is checked immediately and once more at the timeout boundary.
 * This should be preferred to a fixed QTest::qWait when the test can express
 * the state that proves an asynchronous GUI operation completed.
 */
template<typename Predicate>
bool waitUntil(
    Predicate&& predicate,
    std::chrono::milliseconds timeout = std::chrono::seconds(1),
    std::chrono::milliseconds step = std::chrono::milliseconds(10)
)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    step = std::max(step, std::chrono::milliseconds(1));

    while (true) {
        if (predicate()) {
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return static_cast<bool>(predicate());
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        const auto wait = std::min(step, std::max(remaining, std::chrono::milliseconds(1)));
        QEventLoop loop;
        QTimer::singleShot(static_cast<int>(wait.count()), &loop, [&loop] { loop.quit(); });
        loop.exec(QEventLoop::AllEvents);
    }
}

/** Find a descendant widget by its Qt object name. */
template<typename Widget>
Widget* find(QWidget& parent, const char* objectName)
{
    return parent.findChild<Widget*>(QString::fromLatin1(objectName));
}

/** Own one application document and close it when the guard is destroyed. */
class DocumentGuard
{
public:
    /** Create a uniquely named test document. */
    explicit DocumentGuard(const char* baseName = "gui_test")
    {
        ensureGuiApplication();
        _name = App::GetApplication().getUniqueDocumentName(baseName);
        _document = App::GetApplication().newDocument(_name.c_str(), "testUser");
    }

    DocumentGuard(const DocumentGuard&) = delete;
    DocumentGuard& operator=(const DocumentGuard&) = delete;

    /** Close the owned document, if it is still open. */
    ~DocumentGuard()
    {
        close();
    }

    App::Document* get() const
    {
        return _document;
    }

    /** Return the owned document, or nullptr after close(). */
    App::Document* operator->() const
    {
        return _document;
    }

    /** Close the owned document and make this guard empty. */
    void close()
    {
        if (!_document) {
            return;
        }

        if (App::GetApplication().getDocument(_name.c_str())) {
            App::GetApplication().closeDocument(_name.c_str());
        }
        _document = nullptr;
    }

private:
    std::string _name;
    App::Document* _document = nullptr;
};

}  // namespace GuiTest
