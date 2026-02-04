// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ConsoleQtBridge.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QMetaObject>
#include <QThread>

#include <algorithm>
#include <array>
#include <utility>

#include <Base/Console.h>
#include <Base/ServiceProvider.h>

namespace
{

void deliverConsoleMessage(
    Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type,
    Base::IntendedRecipient recipient,
    Base::ContentType content,
    const std::string& notifiername,
    const std::string& msg
);

bool tryMapTypeToLogStyle(Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type, Base::LogStyle& out)
{
    using MsgType = Base::ConsoleSingleton::FreeCAD_ConsoleMsgType;
    using Entry = std::pair<MsgType, Base::LogStyle>;

    static constexpr std::array<Entry, 6> map {
        Entry {MsgType::MsgType_Txt, Base::LogStyle::Message},
        Entry {MsgType::MsgType_Log, Base::LogStyle::Log},
        Entry {MsgType::MsgType_Wrn, Base::LogStyle::Warning},
        Entry {MsgType::MsgType_Err, Base::LogStyle::Error},
        Entry {MsgType::MsgType_Critical, Base::LogStyle::Critical},
        Entry {MsgType::MsgType_Notification, Base::LogStyle::Notification},
    };

    const auto it = std::find_if(map.begin(), map.end(), [type](const Entry& e) { return e.first == type; });
    if (it == map.end()) {
        return false;
    }
    out = it->second;
    return true;
}

class QtConsoleBridge final : public Base::ConsoleSingleton::Bridge
{
public:
    void postEvent(
        Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type,
        Base::IntendedRecipient recipient,
        Base::ContentType content,
        const std::string& notifiername,
        const std::string& msg
    ) const override
    {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            deliverConsoleMessage(type, recipient, content, notifiername, msg);
            return;
        }

        if (QThread::currentThread() == app->thread()) {
            deliverConsoleMessage(type, recipient, content, notifiername, msg);
            return;
        }

        QMetaObject::invokeMethod(
            app,
            [type, recipient, content, notifiername, msg]() {
                deliverConsoleMessage(type, recipient, content, notifiername, msg);
            },
            Qt::QueuedConnection
        );
    }

    void refresh() const override
    {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            return;
        }

        const auto flags = QEventLoop::ExcludeUserInputEvents;
        if (QThread::currentThread() == app->thread()) {
            QCoreApplication::processEvents(flags);
            return;
        }

        // Best-effort: avoid blocking from background threads (can deadlock during shutdown).
        QMetaObject::invokeMethod(
            app,
            []() { QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents); },
            Qt::QueuedConnection
        );
    }
};

void deliverConsoleMessage(
    const Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type,
    const Base::IntendedRecipient recipient,
    const Base::ContentType content,
    const std::string& notifiername,
    const std::string& msg
)
{
    Base::LogStyle style {};
    if (!tryMapTypeToLogStyle(type, style)) {
        return;
    }

    Base::Console().notify(style, recipient, content, notifiername, msg);
}

}  // namespace

namespace App
{

void installConsoleQtBridge()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    static QtConsoleBridge qtConsoleBridge;
    Base::registerServiceImplementation<Base::ConsoleSingleton::Bridge>(&qtConsoleBridge);
    Base::Console().setBridge(Base::provideService<Base::ConsoleSingleton::Bridge>());
}

}  // namespace App
