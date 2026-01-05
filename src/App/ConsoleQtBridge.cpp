// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ConsoleQtBridge.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QMetaObject>
#include <QThread>

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

template<Base::LogStyle category, Base::IntendedRecipient recipientTag>
void notifyConsoleMessageByContentForRecipient(
    const Base::ContentType content,
    const std::string& notifiername,
    const std::string& msg
)
{
    switch (content) {
        case Base::ContentType::Untranslated:
            Base::Console().notify<category, recipientTag, Base::ContentType::Untranslated>(notifiername, msg);
            return;
        case Base::ContentType::Translated:
            Base::Console().notify<category, recipientTag, Base::ContentType::Translated>(notifiername, msg);
            return;
        case Base::ContentType::Untranslatable:
            Base::Console().notify<category, recipientTag, Base::ContentType::Untranslatable>(notifiername, msg);
            return;
    }
}

template<Base::LogStyle category>
void notifyConsoleMessage(
    const Base::IntendedRecipient recipient,
    const Base::ContentType content,
    const std::string& notifiername,
    const std::string& msg
)
{
    switch (recipient) {
        case Base::IntendedRecipient::All:
            notifyConsoleMessageByContentForRecipient<category, Base::IntendedRecipient::All>(
                content,
                notifiername,
                msg
            );
            return;
        case Base::IntendedRecipient::User:
            notifyConsoleMessageByContentForRecipient<category, Base::IntendedRecipient::User>(
                content,
                notifiername,
                msg
            );
            return;
        case Base::IntendedRecipient::Developer:
            notifyConsoleMessageByContentForRecipient<category, Base::IntendedRecipient::Developer>(
                content,
                notifiername,
                msg
            );
            return;
    }
}

void deliverConsoleMessage(
    const Base::ConsoleSingleton::FreeCAD_ConsoleMsgType type,
    const Base::IntendedRecipient recipient,
    const Base::ContentType content,
    const std::string& notifiername,
    const std::string& msg
)
{
    switch (type) {
        case Base::ConsoleSingleton::MsgType_Txt:
            notifyConsoleMessage<Base::LogStyle::Message>(recipient, content, notifiername, msg);
            return;
        case Base::ConsoleSingleton::MsgType_Log:
            notifyConsoleMessage<Base::LogStyle::Log>(recipient, content, notifiername, msg);
            return;
        case Base::ConsoleSingleton::MsgType_Wrn:
            notifyConsoleMessage<Base::LogStyle::Warning>(recipient, content, notifiername, msg);
            return;
        case Base::ConsoleSingleton::MsgType_Err:
            notifyConsoleMessage<Base::LogStyle::Error>(recipient, content, notifiername, msg);
            return;
        case Base::ConsoleSingleton::MsgType_Critical:
            notifyConsoleMessage<Base::LogStyle::Critical>(recipient, content, notifiername, msg);
            return;
        case Base::ConsoleSingleton::MsgType_Notification:
            notifyConsoleMessage<Base::LogStyle::Notification>(recipient, content, notifiername, msg);
            return;
        default:
            return;
    }
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
