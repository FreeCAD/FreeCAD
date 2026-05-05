// SPDX-License-Identifier: LGPL-2.1-or-later

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QtTest/QTest>

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
std::unique_ptr<Gui::Application> g_guiApplication;

bool isMainThread()
{
    return QThread::currentThread() == g_mainThread;
}

void invokeOnMainThread(std::function<void()>&& fn, bool blocking)
{
    g_mainThreadInvokeCount.fetch_add(1, std::memory_order_relaxed);
    QMetaObject::invokeMethod(
        qApp,
        [fn = std::move(fn)]() mutable { fn(); },
        blocking ? Qt::BlockingQueuedConnection : Qt::QueuedConnection
    );
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
        g_guiApplication = std::make_unique<Gui::Application>(true);
        App::MainThreadSignalConfig::setHooks(&isMainThread, &invokeOnMainThread);
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

    void dynamicPropertyBridgeMarshalsRenameToMainThread()
    {
        const std::string docName = App::GetApplication().getUniqueDocumentName("thread_affinity");
        App::DocumentInitFlags initFlags;
        initFlags.createView = false;
        App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser", initFlags);
        QVERIFY(doc != nullptr);

        auto* varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet"));
        QVERIFY(varSet != nullptr);

        auto* prop = freecad_cast<App::PropertyInteger*>(
            varSet->addDynamicProperty("App::PropertyInteger", "BridgePropRenamed", "Variables")
        );
        QVERIFY(prop != nullptr);

        QThread* callbackThread = nullptr;
        std::optional<Gui::DynamicPropertyChangeEvent> observed;
        int matchingSignals = 0;

        auto connection = Gui::Application::Instance->signalDynamicPropertyChanged.connect(
            [&](const Gui::DynamicPropertyChangeEvent& event) {
                if (event.kind != Gui::DynamicPropertyChangeEvent::Kind::Rename
                    || event.property != prop) {
                    return;
                }

                callbackThread = QThread::currentThread();
                observed = event;
                ++matchingSignals;
            }
        );

        std::atomic<bool> done {false};
        std::thread worker([&done, prop] {
            std::string oldName = "BridgeProp";
            App::GetApplication().signalRenameDynamicProperty(*prop, oldName.c_str());
            oldName = "mutated-after-emit";
            done.store(true, std::memory_order_release);
        });

        QTRY_VERIFY(done.load(std::memory_order_acquire));
        worker.join();
        QTRY_VERIFY(observed.has_value());

        QCOMPARE(matchingSignals, 1);
        QCOMPARE(callbackThread, g_mainThread);
        QVERIFY(observed->kind == Gui::DynamicPropertyChangeEvent::Kind::Rename);
        QCOMPARE(observed->container, static_cast<const App::PropertyContainer*>(varSet));
        QCOMPARE(observed->property, static_cast<const App::Property*>(prop));
        QCOMPARE(QString::fromStdString(observed->name), QStringLiteral("BridgePropRenamed"));
        QCOMPARE(QString::fromStdString(observed->oldName), QStringLiteral("BridgeProp"));

        connection.disconnect();
        App::GetApplication().closeDocument(docName.c_str());
    }
};

QTEST_MAIN(ThreadAffinity)

#include "ThreadAffinity.moc"
