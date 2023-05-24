/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef GUI_NOTIFICATIONS_H
#define GUI_NOTIFICATIONS_H

#include <QMessageBox>
#include <QCoreApplication>

#include <Base/Console.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Gui/MainWindow.h>

namespace Gui {

/** Methods to seamlessly provide intrusive or non-intrusive notifications of error, warning,
 * messages, translated notifications, or untranslated notifications originating in a given
 * document object.
 *
 * They are intended for easy substitution of currently blocking modal dialogs in which the user
 * may only click 'ok'.
 *
 * It produces a blocking modal notification or a non-intrusive non-modal notification depending on
 * the preference parameter NotificationArea/NonIntrusiveNotificationsEnabled.
 *
 * The notifier field can be a string or an App::DocumentObject * object, then the notifier is taken from the
 * getFullLabel() method of the DocumentObject.
 *
 * Translations:
 *
 * An attempt is made by NotificationArea to translate the message using the "Notifications" context,
 * except for messages which are already translated. Those that are untranslatable can only be send
 * as messages intended for a developer directly using Console..
 *
 * For the former, this may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 *
 * For translated Notification, many modules using blocking notifications have their translations stored
 * in other contexts, and the translations available at the callee function. This kind of notification
 * provides a very low entry point to move existing blocking notifications into non-intrusive respecting
 * the user choice (given by NotificationArea/NonIntrusiveNotificationsEnabled).
 *
 * Example 1:
 *
 * QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
 *                      QObject::tr("Cannot add a constraint between two external geometries."));
 *
 * Can be rewritten as:
 *
 * Gui::TranslatedNotification(obj,
 *               QObject::tr("Wrong selection"),
 *               QObject::tr("Cannot add a constraint between two external geometries."));
 *
 * or
 *
 * Gui::TranslatedUserWarning(obj,
 *               QObject::tr("Wrong selection"),
 *               QObject::tr("Cannot add a constraint between two external geometries."));
 *
 * or
 *
 * Gui::TranslatedUserError(obj,
 *               QObject::tr("Wrong selection"),
 *               QObject::tr("Cannot add a constraint between two external geometries.")); *
 *
 * depending on the severity.
 *
 * Here obj is a DocumentObject and serves to set the Notifier field for the notification.
 * If the user preference is to see non-intrusive notifications, no pop-up will be shown and the
 * notification will be shown in the notifications area. If the user preference is to see intrusive
 * notifications, the pop-up will be shown (and it won't appear on the notification area as as a non-
 * intrusive notification).
 */

///generic function to send any message provided by Base::LogStyle
template <Base::LogStyle, Base::IntendedRecipient = Base::IntendedRecipient::User,
          Base::ContentType = Base::ContentType::Translated,
          typename TNotifier, typename TCaption, typename TMessage>
inline void Notify(TNotifier && notifier, TCaption && caption, TMessage && message);

/** Convenience function to notify warnings
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TNotifier, typename TCaption, typename TMessage>
inline void NotifyWarning(TNotifier && notifier, TCaption && caption, TMessage && message);

template <typename TNotifier, typename TCaption, typename TMessage>
inline void NotifyUserWarning(TNotifier && notifier, TCaption && caption, TMessage && message);

template <typename TNotifier, typename TCaption, typename TMessage>
inline void TranslatedUserWarning(TNotifier && notifier, TCaption && caption, TMessage && message);

/** Convenience function to notify errors
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TNotifier, typename TCaption, typename TMessage>
inline void NotifyError(TNotifier && notifier, TCaption && caption, TMessage && message);

template <typename TNotifier, typename TCaption, typename TMessage>
inline void NotifyUserError(TNotifier && notifier, TCaption && caption, TMessage && message);

template <typename TNotifier, typename TCaption, typename TMessage>
inline void TranslatedUserError(TNotifier && notifier, TCaption && caption, TMessage && message);

/** Convenience function to send already translated user notifications.
 *  No attempt will be made by the NotificationArea to translate them.
 */
template <typename TNotifier, typename TCaption, typename TMessage>
inline void TranslatedNotification(TNotifier && notifier, TCaption && caption, TMessage && message);

/** Convenience function to send untranslated user notifications.
 *  The NotificationArea will attempt to find a translation in the "Notifications" context.
 *  This may be marked using QT_TRANSLATE_NOOP("Notifications","My message")
 */
template <typename TNotifier, typename TCaption, typename TMessage>
inline void Notification(TNotifier && notifier, TCaption && caption, TMessage && message);
} //namespace Gui


template <Base::LogStyle type, Base::IntendedRecipient recipient, Base::ContentType content, typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::Notify(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    static_assert(content != Base::ContentType::Untranslatable,
                  "A Gui notification cannot be provided with an untranslatable message.");
    static_assert(recipient != Base::IntendedRecipient::Developer,
                  "A Gui notification cannot be intended only for the developer, use Console instead.");
    static_assert(!(recipient == Base::IntendedRecipient::All && content == Base::ContentType::Translated),
                  "Information intended for Developers must not be translated. Provide an untranslated message instead.");

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->
    GetGroup("NotificationArea");

    bool nonIntrusive = hGrp->GetBool("NonIntrusiveNotificationsEnabled", true);

    if(!nonIntrusive) {

        // If Developers are also an intended recipient, notify before creating the blocking pop-up
        if constexpr(recipient == Base::IntendedRecipient::All) {
            // Send also to log for developer only
            auto msg = std::string(message).append("\n"); // use untranslated message

            if constexpr( std::is_base_of_v<App::DocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type,
                                     Base::IntendedRecipient::Developer,
                                     Base::ContentType::Untranslated>(notifier->getFullLabel(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<Gui::ViewProviderDocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type,
                                     Base::IntendedRecipient::Developer,
                                     Base::ContentType::Untranslated>(notifier->getObject()->getFullLabel(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<Gui::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type,
                                     Base::IntendedRecipient::Developer,
                                     Base::ContentType::Untranslated>(notifier->getDocument()->Label.getStrValue(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<App::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type,
                                     Base::IntendedRecipient::Developer,
                                     Base::ContentType::Untranslated>(notifier->Label.getStrValue(), msg.c_str());
            }
            else {
                Base::Console().Send<type,
                                     Base::IntendedRecipient::Developer,
                                     Base::ContentType::Untranslated>(notifier, msg.c_str());
            }
        }

        if constexpr(content == Base::ContentType::Untranslated) {
            if constexpr( type == Base::LogStyle::Warning) {
                QMessageBox::warning(Gui::getMainWindow(),
                                    QCoreApplication::translate("Notifications", caption),
                                    QCoreApplication::translate("Notifications", message));
            }
            else
            if constexpr( type == Base::LogStyle::Error) {
                QMessageBox::critical(Gui::getMainWindow(),
                                    QCoreApplication::translate("Notifications", caption),
                                    QCoreApplication::translate("Notifications", message));
            }
            else {
                QMessageBox::information(Gui::getMainWindow(),
                                    QCoreApplication::translate("Notifications", caption),
                                    QCoreApplication::translate("Notifications", message));
            }
        }
        else {
            if constexpr( type == Base::LogStyle::Warning) {
                QMessageBox::warning(Gui::getMainWindow(),
                                    caption,
                                    message);
            }
            else
            if constexpr( type == Base::LogStyle::Error) {
                QMessageBox::critical(Gui::getMainWindow(),
                                    caption,
                                    message);
            }
            else {
                QMessageBox::information(Gui::getMainWindow(),
                                    caption,
                                    message);
            }
        }
    }
    else {

        if constexpr( content == Base::ContentType::Translated) {
            // trailing newline is not necessary as translated messages are not shown in logs
            auto msg = QStringLiteral("%1. %2").arg(caption).arg(message); // QString

            if constexpr( std::is_base_of_v<App::DocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getFullLabel(), msg.toUtf8());
            }
            else if constexpr( std::is_base_of_v<Gui::ViewProviderDocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getObject()->getFullLabel(), msg.toUtf8());
            }
            else if constexpr( std::is_base_of_v<Gui::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getDocument()->Label.getStrValue(), msg.toUtf8());
            }
            else if constexpr( std::is_base_of_v<App::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->Label.getStrValue(), msg.toUtf8());
            }
            else {
                Base::Console().Send<type, recipient, content>(notifier, msg.toUtf8());
            }
        }
        else {
            // trailing newline is necessary as this may be shown too in a console requiring them (depending on the configuration).
            auto msg = std::string(message).append("\n");

            if constexpr( std::is_base_of_v<App::DocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getFullLabel(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<Gui::ViewProviderDocumentObject, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getObject()->getFullLabel(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<Gui::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->getDocument()->Label.getStrValue(), msg.c_str());
            }
            else if constexpr( std::is_base_of_v<App::Document, std::remove_pointer_t<typename std::decay<TNotifier>::type>> ) {
                Base::Console().Send<type, recipient, content>(notifier->Label.getStrValue(), msg.c_str());
            }
            else {
                Base::Console().Send<type, recipient, content>(notifier, msg.c_str());
            }
        }
    }
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::NotifyWarning(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Warning>(std::forward<TNotifier>(notifier),
                                    std::forward<TCaption>(caption),
                                    std::forward<TMessage>(message));
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::NotifyUserWarning(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Warning,
           Base::IntendedRecipient::User,
           Base::ContentType::Untranslated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::TranslatedUserWarning(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Warning,
           Base::IntendedRecipient::User,
           Base::ContentType::Translated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}


template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::NotifyError(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Error,
           Base::IntendedRecipient::All,
           Base::ContentType::Untranslated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::NotifyUserError(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Error,
           Base::IntendedRecipient::User,
           Base::ContentType::Untranslated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::TranslatedUserError(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Error,
           Base::IntendedRecipient::User,
           Base::ContentType::Translated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}


template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::TranslatedNotification(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Notification,
           Base::IntendedRecipient::User,
           Base::ContentType::Translated>(std::forward<TNotifier>(notifier),
                                          std::forward<TCaption>(caption),
                                          std::forward<TMessage>(message));
}

template <typename TNotifier, typename TCaption, typename TMessage>
inline void Gui::Notification(TNotifier && notifier, TCaption && caption, TMessage && message)
{
    Notify<Base::LogStyle::Notification,
           Base::IntendedRecipient::User,
           Base::ContentType::Untranslated>(std::forward<TNotifier>(notifier),
                                            std::forward<TCaption>(caption),
                                            std::forward<TMessage>(message));
}

#endif // GUI_NOTIFICATIONS_H
