// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <QApplication>
#include <QEvent>
#include <QMetaObject>
#include <QThread>
#include <QtTest/QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MainThreadSignal.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>
#include <Gui/Application.h>
#include <Gui/Selection/Selection.h>

#include <src/App/InitApplication.h>

namespace
{
std::atomic<int> g_mainThreadInvokeCount {0};

QThread* g_mainThread = nullptr;
std::unique_ptr<QObject> g_mainThreadInvoker;
std::unique_ptr<Gui::Application> g_guiApplication;

void drainQtEvents()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

bool isMainThread()
{
    return QThread::currentThread() == g_mainThread;
}

bool invokeOnMainThread(App::MainThreadSignalConfig::TaskFn task, void* context)
{
    g_mainThreadInvokeCount.fetch_add(1, std::memory_order_relaxed);

    if (!g_mainThreadInvoker) {
        return false;
    }

    return QMetaObject::invokeMethod(
        g_mainThreadInvoker.get(),
        [task, context] { task(context); },
        Qt::BlockingQueuedConnection
    );
}

bool rejectInvoke(App::MainThreadSignalConfig::TaskFn, void*)
{
    return false;
}

bool acceptWithoutInvoke(App::MainThreadSignalConfig::TaskFn, void*)
{
    return true;
}

QString workerError(const std::exception_ptr& exception)
{
    if (!exception) {
        return {};
    }
    try {
        std::rethrow_exception(exception);
    }
    catch (const std::exception& error) {
        return QString::fromUtf8(error.what());
    }
    catch (...) {
        return QStringLiteral("Worker threw an unknown exception");
    }
}
}  // namespace

class ThreadAffinity: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();

        g_mainThread = QThread::currentThread();
        g_mainThreadInvoker = std::make_unique<QObject>();

        g_guiApplication = std::make_unique<Gui::Application>(true);

        App::MainThreadSignalConfig::installHooks(&isMainThread, &invokeOnMainThread);
        QVERIFY(App::MainThreadSignalConfig::hasHooks());
    }

    void cleanupTestCase()
    {
        drainQtEvents();

        // Keep the dispatcher valid for the entire Gui::Application lifetime.
        g_guiApplication.reset();

        // Destruction can post deferred work that still needs the dispatcher.
        drainQtEvents();

        App::MainThreadSignalConfig::clearHooks();
        g_mainThreadInvoker.reset();
    }

    void documentSignalUsesGuiHooksAcrossDllBoundary()
    {
        const std::string docName = App::GetApplication().getUniqueDocumentName("thread_affinity_doc");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        auto* varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet"));
        QVERIFY(varSet != nullptr);
        auto* prop = freecad_cast<App::PropertyInteger*>(
            varSet->addDynamicProperty("App::PropertyInteger", "Value", "Variables")
        );
        QVERIFY(prop != nullptr);

        QThread* callbackThread = nullptr;
        int matchingSignals = 0;
        auto connection = doc->signalChangedObject.connect([&](const App::DocumentObject& object,
                                                               const App::Property& changed) {
            if (&object == varSet && &changed == prop) {
                callbackThread = QThread::currentThread();
                ++matchingSignals;
            }
        });

        std::atomic<bool> done {false};
        std::exception_ptr workerException;
        std::thread worker([&] {
            try {
                prop->setValue(42);
            }
            catch (...) {
                workerException = std::current_exception();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        worker.join();
        QVERIFY2(!workerException, qPrintable(workerError(workerException)));

        QCOMPARE(matchingSignals, 1);
        QCOMPARE(callbackThread, g_mainThread);

        connection.disconnect();

        App::GetApplication().closeDocument(docName.c_str());

        QVERIFY(App::GetApplication().getDocument(docName.c_str()) == nullptr);

        // Property changes can leave queued GUI or DeferredDelete work.
        drainQtEvents();
    }

    void dynamicPropertySignalsUseGuiHooksAcrossDllBoundary()
    {
        const std::string docName = App::GetApplication().getUniqueDocumentName(
            "thread_affinity_dynamic"
        );
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        auto* varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet"));
        QVERIFY(varSet != nullptr);
        QThread* appendThread = nullptr;
        const App::Property* appendedProperty = nullptr;
        auto appendConnection = App::GetApplication().signalAppendDynamicProperty.connect(
            [&](const App::Property& appended) {
                if (appended.getName() && std::string(appended.getName()) == "BridgeProp") {
                    appendThread = QThread::currentThread();
                    appendedProperty = &appended;
                }
            }
        );

        App::PropertyInteger* prop = nullptr;
        std::atomic<bool> done {false};
        std::exception_ptr workerException;
        std::thread appendWorker([&] {
            try {
                prop = freecad_cast<App::PropertyInteger*>(
                    varSet->addDynamicProperty("App::PropertyInteger", "BridgeProp", "Variables")
                );
            }
            catch (...) {
                workerException = std::current_exception();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        appendWorker.join();
        QVERIFY2(!workerException, qPrintable(workerError(workerException)));
        QVERIFY(prop != nullptr);
        QVERIFY(appendedProperty == prop);
        QCOMPARE(appendThread, g_mainThread);
        appendConnection.disconnect();

        QThread* renameThread = nullptr;
        std::string oldName;
        std::string newName;
        auto renameConnection = App::GetApplication().signalRenameDynamicProperty.connect(
            [&](const App::Property& changed, const char* previousName) {
                if (&changed == prop) {
                    renameThread = QThread::currentThread();
                    oldName = previousName ? previousName : "";
                    newName = changed.getName() ? changed.getName() : "";
                }
            }
        );

        done.store(false, std::memory_order_release);
        workerException = nullptr;
        bool renamed = false;
        std::thread renameWorker([&] {
            try {
                renamed = varSet->renameDynamicProperty(prop, "BridgePropRenamed");
            }
            catch (...) {
                workerException = std::current_exception();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        renameWorker.join();
        QVERIFY2(!workerException, qPrintable(workerError(workerException)));

        QVERIFY(renamed);
        QCOMPARE(renameThread, g_mainThread);
        QCOMPARE(QString::fromStdString(oldName), QStringLiteral("BridgeProp"));
        QCOMPARE(QString::fromStdString(newName), QStringLiteral("BridgePropRenamed"));
        renameConnection.disconnect();

        QThread* removeThread = nullptr;
        std::string removedName;
        auto removeConnection = App::GetApplication().signalRemoveDynamicProperty.connect(
            [&](const App::Property& removed) {
                if (&removed == prop) {
                    removeThread = QThread::currentThread();
                    removedName = removed.getName() ? removed.getName() : "";
                }
            }
        );

        done.store(false, std::memory_order_release);
        workerException = nullptr;
        bool removed = false;
        std::thread removeWorker([&] {
            try {
                removed = varSet->removeDynamicProperty("BridgePropRenamed");
            }
            catch (...) {
                workerException = std::current_exception();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        removeWorker.join();
        QVERIFY2(!workerException, qPrintable(workerError(workerException)));

        QVERIFY(removed);
        QCOMPARE(removeThread, g_mainThread);
        QCOMPARE(QString::fromStdString(removedName), QStringLiteral("BridgePropRenamed"));

        removeConnection.disconnect();
        App::GetApplication().closeDocument(docName.c_str());
    }

    void selectionMutatorsMarshalToMainThread()
    {
        g_mainThreadInvokeCount.store(0, std::memory_order_relaxed);

        std::atomic<bool> done {false};
        std::thread worker([&done] {
            Gui::Selection().rmvPreselect();
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        worker.join();

        QCOMPARE(g_mainThreadInvokeCount.load(std::memory_order_relaxed), 1);
    }

    void mainThreadExceptionsReturnToWorker()
    {
        std::atomic<bool> done {false};
        std::string error;

        std::thread worker([&] {
            try {
                App::MainThreadSignalConfig::callOnMainThreadSync([] {
                    throw std::runtime_error("main-thread failure");
                });
            }
            catch (const std::runtime_error& exception) {
                error = exception.what();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        worker.join();

        QCOMPARE(QString::fromStdString(error), QStringLiteral("main-thread failure"));
    }

    void mainThreadSyncSupportsResultTypes()
    {
        const int constValue = 43;
        int valueResult = 0;
        const int* referenceResult = nullptr;
        const int* constReferenceResult = nullptr;
        int moveOnlyResult = 0;
        int immovableLvalueResult = 0;
        std::atomic<bool> done {false};
        std::exception_ptr workerException;

        struct ImmovableCallable
        {
            ImmovableCallable() = default;
            ImmovableCallable(const ImmovableCallable&) = delete;
            ImmovableCallable(ImmovableCallable&&) = delete;

            int operator()() const
            {
                return 45;
            }
        };

        std::thread worker([&] {
            try {
                valueResult = App::MainThreadSignalConfig::callOnMainThreadSync([] { return 42; });

                int& reference = App::MainThreadSignalConfig::callOnMainThreadSync(
                    [&valueResult]() -> int& { return valueResult; }
                );
                referenceResult = &reference;

                const int& constReference = App::MainThreadSignalConfig::callOnMainThreadSync(
                    [&constValue]() -> const int& { return constValue; }
                );
                constReferenceResult = &constReference;

                moveOnlyResult = App::MainThreadSignalConfig::callOnMainThreadSync(
                    [payload = std::make_unique<int>(44)] { return *payload; }
                );

                ImmovableCallable callable;
                immovableLvalueResult = App::MainThreadSignalConfig::callOnMainThreadSync(callable);
            }
            catch (...) {
                workerException = std::current_exception();
            }
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        worker.join();
        QVERIFY2(!workerException, qPrintable(workerError(workerException)));

        QCOMPARE(valueResult, 42);
        QCOMPARE(referenceResult, &valueResult);
        QCOMPARE(constReferenceResult, &constValue);
        QCOMPARE(moveOnlyResult, 44);
        QCOMPARE(immovableLvalueResult, 45);
    }

    void mainThreadSyncHandlesFallbackAndHookFailures()
    {
        App::MainThreadSignalConfig::clearHooks();

        QThread* inlineThread = nullptr;
        int inlineResult = 0;
        std::atomic<bool> inlineDone {false};
        std::exception_ptr inlineException;
        std::thread inlineWorker([&] {
            try {
                inlineResult = App::MainThreadSignalConfig::callOnMainThreadSync([&] {
                    inlineThread = QThread::currentThread();
                    return 51;
                });
            }
            catch (...) {
                inlineException = std::current_exception();
            }
            inlineDone.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(inlineDone.load(std::memory_order_acquire));
        inlineWorker.join();
        QVERIFY2(!inlineException, qPrintable(workerError(inlineException)));
        QVERIFY(!App::MainThreadSignalConfig::hasHooks());
        QCOMPARE(inlineResult, 51);
        QVERIFY(inlineThread != g_mainThread);

        App::MainThreadSignalConfig::installHooks(&isMainThread, &invokeOnMainThread);

        bool rejectedPartialInstall = false;
        try {
            App::MainThreadSignalConfig::installHooks(&isMainThread, nullptr);
        }
        catch (const std::invalid_argument&) {
            rejectedPartialInstall = true;
        }
        QVERIFY(rejectedPartialInstall);
        QVERIFY(App::MainThreadSignalConfig::hasHooks());

        App::MainThreadSignalConfig::installHooks(&isMainThread, &rejectInvoke);
        std::atomic<bool> rejectedDone {false};
        std::string rejection;
        std::thread rejectedWorker([&] {
            try {
                static_cast<void>(App::MainThreadSignalConfig::callOnMainThreadSync([] {
                    return 52;
                }));
            }
            catch (const std::runtime_error& exception) {
                rejection = exception.what();
            }
            rejectedDone.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(rejectedDone.load(std::memory_order_acquire));
        rejectedWorker.join();
        QCOMPARE(
            QString::fromStdString(rejection),
            QStringLiteral("Failed to invoke callable on the main thread")
        );

        App::MainThreadSignalConfig::installHooks(&isMainThread, &acceptWithoutInvoke);
        std::atomic<bool> incompleteDone {false};
        std::string incompleteError;
        std::thread incompleteWorker([&] {
            try {
                static_cast<void>(App::MainThreadSignalConfig::callOnMainThreadSync([] {
                    return 53;
                }));
            }
            catch (const std::logic_error& exception) {
                incompleteError = exception.what();
            }
            incompleteDone.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(incompleteDone.load(std::memory_order_acquire));
        incompleteWorker.join();
        QCOMPARE(
            QString::fromStdString(incompleteError),
            QStringLiteral("Main-thread hook returned before the callable completed")
        );

        std::atomic<bool> incompleteVoidDone {false};
        std::string incompleteVoidError;
        std::thread incompleteVoidWorker([&] {
            try {
                App::MainThreadSignalConfig::callOnMainThreadSync([] {});
            }
            catch (const std::logic_error& exception) {
                incompleteVoidError = exception.what();
            }
            incompleteVoidDone.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(incompleteVoidDone.load(std::memory_order_acquire));
        incompleteVoidWorker.join();
        QCOMPARE(
            QString::fromStdString(incompleteVoidError),
            QStringLiteral("Main-thread hook returned before the callable completed")
        );

        App::MainThreadSignalConfig::installHooks(&isMainThread, &invokeOnMainThread);
        g_mainThreadInvokeCount.store(0, std::memory_order_relaxed);
        QCOMPARE(App::MainThreadSignalConfig::callOnMainThreadSync([] { return 54; }), 54);
        QCOMPARE(g_mainThreadInvokeCount.load(std::memory_order_relaxed), 0);
    }
};

QTEST_MAIN(ThreadAffinity)

#include "ThreadAffinity.moc"
