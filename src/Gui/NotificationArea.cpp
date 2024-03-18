/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QAction>
#include <QActionEvent>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QStringList>
#include <QTextDocument>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QWidgetAction>
#include <memory>
#include <mutex>
#endif

#include <App/Application.h>
#include <Base/Console.h>

#include "Application.h"
#include "BitmapFactory.h"
#include "MDIView.h"
#include "MainWindow.h"
#include "NotificationBox.h"

#include "NotificationArea.h"

using namespace Gui;

using Connection = boost::signals2::connection;

namespace sp = std::placeholders;

using std::lock_guard;

class NotificationAreaObserver;

namespace Gui
{
/** PImpl idiom structure having all data necessary for the notification area */
struct NotificationAreaP
{
    // Structure holding all variables necessary for the Notification Area.
    // Preference parameters are updated by NotificationArea::ParameterObserver

    //NOLINTBEGIN
    /** @name Non-intrusive notifications parameters */
    //@{
    /// Parameter controlled
    int maxOpenNotifications = 15;
    /// Parameter controlled
    unsigned int notificationExpirationTime = 10000;
    /// minimum time that the notification will remain unclosed
    unsigned int minimumOnScreenTime = 5000;
    /// Parameter controlled
    bool notificationsDisabled = false;
    /// Control of confirmation mechanism (for Critical Messages)
    bool requireConfirmationCriticalMessageDuringRestoring = true;
    /// Width of the non-intrusive notification
    int notificationWidth = 800;
    /// Any open non-intrusive notifications will disappear when another window is activated
    bool hideNonIntrusiveNotificationsWhenWindowDeactivated = true;
    /// Prevent non-intrusive notifications from appearing when the FreeCAD Window is not the active
    /// window
    bool preventNonIntrusiveNotificationsWhenWindowNotActive = true;
    //@}

    /** @name Widget parameters */
    //@{
    /// Parameter controlled - maximum number of message allowed in the notification area widget (0
    /// means no limit)
    int maxWidgetMessages = 1000;
    /// User notifications get automatically removed from the Widget after the non-intrusive
    /// notification expiration time
    bool autoRemoveUserNotifications = false;
    //@}

    /** @name Notification rate control */
    //@{
    /* Control rate of updates of non-intrusive messages (avoids visual artifacts when messages are
     * constantly being received) */
    // Timer to delay notification until a minimum time between two consecutive messages have lapsed
    QTimer inhibitTimer;
    // The time between two consecutive messages forced by the inhibitTimer
    const unsigned int inhibitNotificationTime = 250;
    //@}

    /** @name Data source control */
    //@{
    /* Controls whether debug warnings and errors intended for developers should be processed or not
     */
    bool developerErrorSubscriptionEnabled = false;
    bool developerWarningSubscriptionEnabled = false;
    //@}

    bool missedNotifications = false;

    // Access control
    std::mutex mutexNotification;

    // Pointers to widgets (no ownership)
    QMenu* menu = nullptr;
    QWidgetAction* notificationaction = nullptr;

    /** @name Resources */
    //@{
    /// Base::Console Message observer
    std::unique_ptr<NotificationAreaObserver> observer;
    Connection finishRestoreDocumentConnection;
    /// Preference Parameter observer
    std::unique_ptr<NotificationArea::ParameterObserver> parameterObserver;
    //@}

    //NOLINTEND
};

}// namespace Gui


/******************* Resource Management *****************************************/

/** Simple class to manage Notification Area Resources*/
class ResourceManager
{

private:
    ResourceManager()
    {
        //NOLINTBEGIN
        error = BitmapFactory().pixmapFromSvg(":/icons/edit_Cancel.svg", QSize(16, 16));
        warning = BitmapFactory().pixmapFromSvg(":/icons/Warning.svg", QSize(16, 16));
        critical = BitmapFactory().pixmapFromSvg(":/icons/critical-info.svg", QSize(16, 16));
        info = BitmapFactory().pixmapFromSvg(":/icons/info.svg", QSize(16, 16));
        //NOLINTEND
        notificationArea = QIcon(QStringLiteral(":/icons/InTray.svg"));
        notificationAreaMissedNotifications =
            QIcon(QStringLiteral(":/icons/InTray_missed_notifications.svg"));
    }

    inline static const auto& getResourceManager()
    {
        static ResourceManager manager;

        return manager;
    }

public:
    inline static auto ErrorPixmap()
    {
        auto rm = getResourceManager();
        return rm.error;
    }

    inline static auto WarningPixmap()
    {
        auto rm = getResourceManager();
        return rm.warning;
    }

    inline static auto CriticalPixmap()
    {
        auto rm = getResourceManager();
        return rm.critical;
    }

    inline static auto InfoPixmap()
    {
        auto rm = getResourceManager();
        return rm.info;
    }

    inline static auto NotificationAreaIcon()
    {
        auto rm = getResourceManager();
        return rm.notificationArea;
    }

    inline static auto notificationAreaMissedNotificationsIcon()
    {
        auto rm = getResourceManager();
        return rm.notificationAreaMissedNotifications;
    }

private:
    QPixmap error;
    QPixmap warning;
    QPixmap critical;
    QPixmap info;
    QIcon notificationArea;
    QIcon notificationAreaMissedNotifications;
};

/******************** Console Messages Observer (Console Interface) ************************/

/** This class listens to all messages sent via the console interface and
   feeds the non-intrusive notification system and the notifications widget */
class NotificationAreaObserver: public Base::ILogger
{
public:
    NotificationAreaObserver(NotificationArea* notificationarea);
    ~NotificationAreaObserver() override;

    NotificationAreaObserver(const NotificationAreaObserver &) = delete;
    NotificationAreaObserver(NotificationAreaObserver &&) = delete;
    NotificationAreaObserver &operator=(const NotificationAreaObserver &) = delete;
    NotificationAreaObserver &operator=(NotificationAreaObserver &&) = delete;

    /// Function that is called by the console interface for this observer with the message
    /// information
    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                 Base::IntendedRecipient recipient, Base::ContentType content) override;

    /// Name of the observer
    const char* Name() override
    {
        return "NotificationAreaObserver";
    }

private:
    NotificationArea* notificationArea;
};

NotificationAreaObserver::NotificationAreaObserver(NotificationArea* notificationarea)
    : notificationArea(notificationarea)
{
    Base::Console().AttachObserver(this);
    bLog = false;        // ignore log messages
    bMsg = false;        // ignore messages
    bNotification = true;// activate user notifications
}

NotificationAreaObserver::~NotificationAreaObserver()
{
    Base::Console().DetachObserver(this);
}

void NotificationAreaObserver::SendLog(const std::string& notifiername, const std::string& msg,
                                       Base::LogStyle level, Base::IntendedRecipient recipient,
                                       Base::ContentType content)
{
    // 1. As notification system is shared with report view and others, the expectation is that any
    // individual error and warning message will end in "\n". This means the string must be stripped
    // of this character for representation in the notification area.
    // 2. However, any message marked with the QT_TRANSLATE_NOOT macro with the "Notifications"
    // context, shall not include
    // "\n", as this generates problems with the translation system. Then the string must be
    // stripped of "\n" before translation.

    bool violatesBasicPolicy = (recipient == Base::IntendedRecipient::Developer
                                || content == Base::ContentType::Untranslatable);

    // We allow derogations for debug purposes according to user preferences
    bool meetsDerogationCriteria = false;

    if (violatesBasicPolicy) {
        meetsDerogationCriteria =
            (level == Base::LogStyle::Warning && notificationArea->areDeveloperWarningsActive())
            || (level == Base::LogStyle::Error && notificationArea->areDeveloperErrorsActive());
    }

    if (violatesBasicPolicy && !meetsDerogationCriteria) {
        return;
    }

    auto simplifiedstring =
        QString::fromStdString(msg)
            .trimmed();// remove any leading and trailing whitespace character ('\n')

    // avoid processing empty strings
    if (simplifiedstring.isEmpty())
        return;

    if (content == Base::ContentType::Translated) {
        notificationArea->pushNotification(
            QString::fromStdString(notifiername), simplifiedstring, level);
    }
    else {
        notificationArea->pushNotification(
            QString::fromStdString(notifiername),
            QCoreApplication::translate("Notifications", simplifiedstring.toUtf8()),
            level);
    }
}


/******************* Notification Widget *******************************************************/

/** Specialised Item class for the Widget notifications/errors/warnings
   It holds all item specific data, including visualisation data and controls how
   the item should appear in the widget.
*/
class NotificationItem: public QTreeWidgetItem
{
public:
    NotificationItem(Base::LogStyle notificationtype, QString notifiername, QString message)
        : notificationType(notificationtype),
          notifierName(std::move(notifiername)),
          msg(std::move(message))
    {}

    QVariant data(int column, int role) const override
    {
        // strings that will be displayed for each column of the widget
        if (role == Qt::DisplayRole) {
            switch (column) {
                case 1:
                    return notifierName;
                    break;
                case 2:
                    return getMessage();
                    break;
            }
        }
        else if (column == 0 && role == Qt::DecorationRole) {
            // Icons to be displayed for the first row
            if (notificationType == Base::LogStyle::Error) {
                return std::move(ResourceManager::ErrorPixmap());
            }
            else if (notificationType == Base::LogStyle::Warning) {
                return std::move(ResourceManager::WarningPixmap());
            }
            else if (notificationType == Base::LogStyle::Critical) {
                return std::move(ResourceManager::CriticalPixmap());
            }
            else {
                return std::move(ResourceManager::InfoPixmap());
            }
        }
        else if (role == Qt::FontRole) {
            // Visualisation control of unread messages
            static QFont font;
            static QFont boldFont(font.family(), font.pointSize(), QFont::Bold);

            if (unread) {
                return boldFont;
            }

            return font;
        }

        return {};
    }

    void addRepetition() {
        unread = true;
        notifying = true;
        shown = false;
        repetitions++;
    }

    bool isRepeated(Base::LogStyle notificationtype, const QString & notifiername, const QString & message ) const {
        return (notificationType == notificationtype && notifierName == notifiername && msg == message);
    }

    bool isType(Base::LogStyle notificationtype) const {
        return notificationType == notificationtype;
    }

    bool isUnread() const {
        return unread;
    }

    bool isNotifying() const {
        return notifying;
    }

    bool isShown() const {
        return shown;
    }

    int getRepetitions() const{
        return repetitions;
    }

    void setNotified() {
        notifying = false;
    }

    void resetNotified() {
        notifying = true;
    }

    void setShown() {
        shown = true;
    }

    void resetShown() {
        shown = false;
    }

    void setRead() {
        unread = false;
    }

    void setUnread() {
        unread = true;
    }

    QString getMessage() const {
        if(repetitions == 0) {
            return msg;
        }
        else {
            return msg + QObject::tr(" (%1 times)").arg(repetitions+1);
        }
    }

    const QString & getNotifier() {
        return notifierName;
    }

private:
    Base::LogStyle notificationType;
    QString notifierName;
    QString msg;

    bool unread = true;   // item is unread in the Notification Area Widget
    bool notifying = true;// item is to be notified or being notified as non-intrusive message
    bool shown = false;   // item is already being notified (it is onScreen)
    int repetitions = 0; // message appears n times in a row.
};

/** Drop menu Action containing the notifications widget.
 *  It stores all the notification item information in the form
 * of NotificationItems (QTreeWidgetItem). This information is used
 * by the Widget and by the non-intrusive messages.
 * It owns the notification resources and is responsible for the release
 * of the memory resources, either directly for the intermediate fast cache
 * or indirectly via QT for the case of the QTreeWidgetItems.
 */
class NotificationsAction: public QWidgetAction
{
public:
    NotificationsAction(QWidget* parent)
        : QWidgetAction(parent)
    {}

    NotificationsAction(const NotificationsAction &) = delete;
    NotificationsAction(NotificationsAction &&) = delete;
    NotificationsAction & operator=(const NotificationsAction &) = delete;
    NotificationsAction & operator=(NotificationsAction &&) = delete;

    ~NotificationsAction() override
    {
        for (auto* item : qAsConst(pushedItems)) {
            if (item) {
                delete item; // NOLINT
            }
        }
    }

public:
    /// deletes only notifications (messages of type Notification)
    void deleteNotifications()
    {
        if (tableWidget) {
            for (int i = tableWidget->topLevelItemCount() - 1; i >= 0; i--) {
                //NOLINTNEXTLINE
                auto* item = static_cast<NotificationItem*>(tableWidget->topLevelItem(i));
                if (item->isType(Base::LogStyle::Notification)) {
                    delete item; //NOLINT
                }
            }
        }
        for (int i = pushedItems.size() - 1; i >= 0; i--) {
            //NOLINTNEXTLINE
            auto* item = static_cast<NotificationItem*>(pushedItems.at(i));
            if (item->isType(Base::LogStyle::Notification)) {
                delete pushedItems.takeAt(i); //NOLINT
            }
        }
    }

    /// deletes all notifications, errors and warnings
    void deleteAll()
    {
        if (tableWidget) {
            tableWidget->clear();// the parent will delete the items.
        }
        while (!pushedItems.isEmpty())
            delete pushedItems.takeFirst();
    }

    /// returns the amount of unread notifications, errors and warnings
    inline int getUnreadCount() const
    {
        return getCurrently([](auto* item) {
            return item->isUnread();
        });
    }

    /// returns the amount of notifications, errors and warnings currently being notified
    inline int getCurrentlyNotifyingCount() const
    {
        return getCurrently([](auto* item) {
            return item->isNotifying();
        });
    }

    /// returns the amount of notifications, errors and warnings currently being shown as
    /// non-intrusive messages (on-screen)
    inline int getShownCount() const
    {
        return getCurrently([](auto* item) {
            return item->isShown();
        });
    }

    /// marks all notifications, errors and warnings as read
    void clearUnreadFlag()
    {
        for (auto i = 0; i < tableWidget->topLevelItemCount();
             i++) {// all messages were read, so clear the unread flag
            //NOLINTNEXTLINE
            auto* item = static_cast<NotificationItem*>(tableWidget->topLevelItem(i));
            item->setRead();
        }
    }

    /// pushes all Notification Items to the Widget, so that they can be shown
    void synchroniseWidget()
    {
        tableWidget->insertTopLevelItems(0, pushedItems);
        pushedItems.clear();
    }

    /** pushes all Notification Items to the fast cache (this also prevents all unnecessary
     * signaling from parents) and allows to accelerate insertions and deletions
     */
    void shiftToCache()
    {
        tableWidget->blockSignals(true);
        tableWidget->clearSelection();
        while (tableWidget->topLevelItemCount() > 0) {
            auto* item = tableWidget->takeTopLevelItem(0);
            pushedItems.push_back(item);
        }
        tableWidget->blockSignals(false);
    }

    /// returns if there are no notifications, errors and warnings at all
    bool isEmpty() const
    {
        return tableWidget->topLevelItemCount() == 0 && pushedItems.isEmpty();
    }

    /// returns the total amount of notifications, errors and warnings currently stored
    auto count() const
    {
        return tableWidget->topLevelItemCount() + pushedItems.count();
    }

    /// retrieves a pointer to a given notification from storage.
    auto getItem(int index) const
    {
        if (index < pushedItems.count()) {
            return pushedItems.at(index);
        }
        else {
            return tableWidget->topLevelItem(index - pushedItems.count());
        }
    }

    /// deletes a given notification, errors or warnings by index
    void deleteItem(int index)
    {
        if (index < pushedItems.count()) {
            delete pushedItems.takeAt(index); //NOLINT
        }
        else {
            delete tableWidget->topLevelItem(index - pushedItems.count()); //NOLINT
        }
    }

    /// deletes a given notification, errors or warnings by pointer
    void deleteItem(NotificationItem* item)
    {
        for (int i = 0; i < count(); i++) {
            if (getItem(i) == item) {
                deleteItem(i);
                return;
            }
        }
    }

    /// deletes the last Notification Item
    void deleteLastItem()
    {
        deleteItem(count() - 1);
    }

    /// checks if last notification is the same
    bool isSameNotification(const QString& notifiername, const QString& message,
                            Base::LogStyle level) const {
        if(count() > 0) { // if not empty
            //NOLINTNEXTLINE
            auto item = static_cast<NotificationItem*>(getItem(0));
            return item->isRepeated(level,notifiername,message);
        }

        return false;
    }

    void resetLastNotificationStatus() {
        //NOLINTNEXTLINE
        auto item = static_cast<NotificationItem*>(getItem(0));
        item->addRepetition();
    }

    /// pushes a notification item to the front
    auto push_front(std::unique_ptr<NotificationItem> item)
    {
        auto it = item.release();
        pushedItems.push_front(it);
        return it;
    }

    QSize size()
    {
        return tableWidget->size();
    }

protected:
    /// creates the Notifications Widget
    QWidget* createWidget(QWidget* parent) override
    {
        //NOLINTBEGIN
        QWidget* notificationsWidget = new QWidget(parent);

        QHBoxLayout* layout = new QHBoxLayout(notificationsWidget);
        notificationsWidget->setLayout(layout);

        tableWidget = new QTreeWidget(parent);
        tableWidget->setColumnCount(3);

        QStringList headers;
        headers << QObject::tr("Type") << QObject::tr("Notifier") << QObject::tr("Message");
        tableWidget->setHeaderLabels(headers);

        layout->addWidget(tableWidget);

        tableWidget->setMaximumSize(1200, 600);
        tableWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        tableWidget->header()->setStretchLastSection(false);
        tableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);


        // context menu on any item (row) of the widget
        QObject::connect(
            tableWidget, &QTreeWidget::customContextMenuRequested, [&](const QPoint& pos) {
                auto selectedItems = tableWidget->selectedItems();

                QMenu menu;

                QAction* del = menu.addAction(tr("Delete"), this, [&]() {
                    for (auto it : qAsConst(selectedItems)) {
                        delete it;
                    }
                });

                del->setEnabled(!selectedItems.isEmpty());

                menu.addSeparator();

                QAction* delnotifications =
                    menu.addAction(tr("Delete user notifications"),
                                   this,
                                   &NotificationsAction::deleteNotifications);

                delnotifications->setEnabled(tableWidget->topLevelItemCount() > 0);

                QAction* delall =
                    menu.addAction(tr("Delete All"), this, &NotificationsAction::deleteAll);

                delall->setEnabled(tableWidget->topLevelItemCount() > 0);

                menu.setDefaultAction(del);

                menu.exec(tableWidget->mapToGlobal(pos));
            });
        //NOLINTEND

        return notificationsWidget;
    }

private:
    /// utility function to return the number of Notification Items meeting the functor/lambda
    /// criteria
    int getCurrently(std::function<bool(const NotificationItem*)> F) const
    {
        int instate = 0;
        for (auto i = 0; i < tableWidget->topLevelItemCount(); i++) {
            //NOLINTNEXTLINE
            auto* item = static_cast<NotificationItem*>(tableWidget->topLevelItem(i));
            if (F(item)) {
                instate++;
            }
        }
        for (auto i = 0; i < pushedItems.count(); i++) {
             //NOLINTNEXTLINE
            auto* item = static_cast<NotificationItem*>(pushedItems.at(i));
            if (F(item)) {
                instate++;
            }
        }
        return instate;
    }

private:
    QTreeWidget* tableWidget = nullptr;
    // Intermediate storage
    // Note: QTreeWidget is helplessly slow to single insertions, QTreeWidget is actually only
    // necessary when showing the widget. A single QList insertion into a QTreeWidget is actually
    // not that slow. The use of this intermediate storage substantially accelerates non-intrusive
    // notifications.
    QList<QTreeWidgetItem*> pushedItems;
};

/************ Parameter Observer (preferences) **************************************/

NotificationArea::ParameterObserver::ParameterObserver(NotificationArea* notificationarea)
    : notificationArea(notificationarea)
{
    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/NotificationArea");

    //NOLINTBEGIN
    parameterMap = {
        {"NotificationAreaEnabled",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), true);
             if (!enabled)
                 notificationArea->deleteLater();
         }},
        {"NonIntrusiveNotificationsEnabled",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), true);
             notificationArea->pImp->notificationsDisabled = !enabled;
         }},
        {"NotificationTime",
         [this](const std::string& string) {
             auto time = hGrp->GetInt(string.c_str(), 20) * 1000;
             if (time < 0)
                 time = 0;
             notificationArea->pImp->notificationExpirationTime = static_cast<unsigned int>(time);
         }},
        {"MinimumOnScreenTime",
         [this](const std::string& string) {
             auto time = hGrp->GetInt(string.c_str(), 5) * 1000;
             if (time < 0)
                 time = 0;
             notificationArea->pImp->minimumOnScreenTime = static_cast<unsigned int>(time);
         }},
        {"MaxOpenNotifications",
         [this](const std::string& string) {
             auto limit = hGrp->GetInt(string.c_str(), 15);
             if (limit < 0)
                 limit = 0;
             notificationArea->pImp->maxOpenNotifications = static_cast<unsigned int>(limit);
         }},
        {"MaxWidgetMessages",
         [this](const std::string& string) {
             auto limit = hGrp->GetInt(string.c_str(), 1000);
             if (limit < 0)
                 limit = 0;
             notificationArea->pImp->maxWidgetMessages = static_cast<unsigned int>(limit);
         }},
        {"AutoRemoveUserNotifications",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), true);
             notificationArea->pImp->autoRemoveUserNotifications = enabled;
         }},
        {"NotificiationWidth",
         [this](const std::string& string) {
             auto width = hGrp->GetInt(string.c_str(), 800);
             if (width < 300)
                 width = 300;
             notificationArea->pImp->notificationWidth = width;
         }},
        {"HideNonIntrusiveNotificationsWhenWindowDeactivated",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), true);
             notificationArea->pImp->hideNonIntrusiveNotificationsWhenWindowDeactivated = enabled;
         }},
        {"PreventNonIntrusiveNotificationsWhenWindowNotActive",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), true);
             notificationArea->pImp->preventNonIntrusiveNotificationsWhenWindowNotActive = enabled;
         }},
        {"DeveloperErrorSubscriptionEnabled",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), false);
             notificationArea->pImp->developerErrorSubscriptionEnabled = enabled;
         }},
        {"DeveloperWarningSubscriptionEnabled",
         [this](const std::string& string) {
             auto enabled = hGrp->GetBool(string.c_str(), false);
             notificationArea->pImp->developerWarningSubscriptionEnabled = enabled;
         }},
    };
    //NOLINTEND

    for (auto& val : parameterMap) {
        auto string = val.first;
        auto update = val.second;

        update(string);
    }

    hGrp->Attach(this);
}

NotificationArea::ParameterObserver::~ParameterObserver()
{
    hGrp->Detach(this);
}

void NotificationArea::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller,
                                                   const char* sReason)
{
    (void)rCaller;

    auto key = parameterMap.find(sReason);

    if (key != parameterMap.end()) {
        auto string = key->first;
        auto update = key->second;

        update(string);
    }
}

/************************* Notification Area *****************************************/

NotificationArea::NotificationArea(QWidget* parent)
    : QPushButton(parent)
{
    // QPushButton appearance
    setText(QString());
    setFlat(true);

    // Initialisation of pImpl structure
    pImp = std::make_unique<NotificationAreaP>();

    pImp->observer = std::make_unique<NotificationAreaObserver>(this);
    pImp->parameterObserver = std::make_unique<NotificationArea::ParameterObserver>(this);

    pImp->menu = new QMenu(parent); //NOLINT
    setMenu(pImp->menu);

    auto na = new NotificationsAction(pImp->menu); //NOLINT

    pImp->menu->addAction(na);

    pImp->notificationaction = na;

    //NOLINTBEGIN
    // Signals for synchronisation of storage before showing/hiding the widget
    QObject::connect(pImp->menu, &QMenu::aboutToHide, [&]() {
        lock_guard<std::mutex> g(pImp->mutexNotification);
        static_cast<NotificationsAction*>(pImp->notificationaction)->clearUnreadFlag();
        static_cast<NotificationsAction*>(pImp->notificationaction)->shiftToCache();
    });

    QObject::connect(pImp->menu, &QMenu::aboutToShow, [this]() {
        // guard to avoid modifying the notification list and indices while creating the tooltip
        lock_guard<std::mutex> g(pImp->mutexNotification);
        setText(QString::number(0)); // no unread notifications
        if (pImp->missedNotifications) {
            setIcon(TrayIcon::Normal);
            pImp->missedNotifications = false;
        }
        static_cast<NotificationsAction*>(pImp->notificationaction)->synchroniseWidget();


        // There is a Qt bug in not respecting a QMenu size when size changes in aboutToShow.
        //
        // https://bugreports.qt.io/browse/QTBUG-54421
        // https://forum.qt.io/topic/68765/how-to-update-geometry-on-qaction-visibility-change-in-qmenu-abouttoshow/3
        //
        // None of this works
        // pImp->menu->updateGeometry();
        // pImp->menu->adjustSize();
        // pImp->menu->ensurePolished();
        // this->updateGeometry();
        // this->adjustSize();
        // this->ensurePolished();

        // This does correct the size
        QSize size = static_cast<NotificationsAction*>(pImp->notificationaction)->size();
        QResizeEvent re(size, size);
        qApp->sendEvent(pImp->menu, &re);

        // This corrects the position of the menu
        QTimer::singleShot(0, [&] {
            QWidget* statusbar = static_cast<QWidget*>(this->parent());
            QPoint statusbar_top_right = statusbar->mapToGlobal(statusbar->rect().topRight());
            QSize menusize = pImp->menu->size();
            QWidget* w = this;
            QPoint button_pos = w->mapToGlobal(w->rect().topLeft());
            QPoint widget_pos;
            if ((statusbar_top_right.x() - menusize.width()) > button_pos.x()) {
                widget_pos = QPoint(button_pos.x(), statusbar_top_right.y() - menusize.height());
            }
            else {
                widget_pos = QPoint(statusbar_top_right.x() - menusize.width(),
                                    statusbar_top_right.y() - menusize.height());
            }
            pImp->menu->move(widget_pos);
        });
    });

    // Connection to the finish restore signal to rearm Critical messages modal mode when action is
    // user initiated
    pImp->finishRestoreDocumentConnection =
        App::GetApplication().signalFinishRestoreDocument.connect(
            std::bind(&Gui::NotificationArea::slotRestoreFinished, this, sp::_1));
    //NOLINTEND

    // Initialisation of the timer to inhibit continuous updates of the notification system in case
    // clusters of messages arrive (so as to delay the actual notification until the whole cluster
    // has arrived)
    pImp->inhibitTimer.setSingleShot(true);

    pImp->inhibitTimer.callOnTimeout([this, na]() {
        setText(QString::number(na->getUnreadCount()));
        showInNotificationArea();
    });

    setIcon(TrayIcon::Normal);
}

NotificationArea::~NotificationArea()
{
    pImp->finishRestoreDocumentConnection.disconnect();
}

void NotificationArea::mousePressEvent(QMouseEvent* e)
{
    // Contextual menu (e.g. to delete Notifications or all messages (Notifications, Errors,
    // Warnings and Critical messages)
    if (e->button() == Qt::RightButton && hitButton(e->pos())) {
        QMenu menu;

        //NOLINTBEGIN
        NotificationsAction* na = static_cast<NotificationsAction*>(pImp->notificationaction);

        QAction* delnotifications = menu.addAction(tr("Delete user notifications"), [&]() {
            // guard to avoid modifying the notification list and indices while creating the tooltip
            lock_guard<std::mutex> g(pImp->mutexNotification);
            na->deleteNotifications();
            setText(QString::number(na->getUnreadCount()));
        });

        delnotifications->setEnabled(!na->isEmpty());

        QAction* delall = menu.addAction(tr("Delete All"), [&]() {
            // guard to avoid modifying the notification list and indices while creating the tooltip
            lock_guard<std::mutex> g(pImp->mutexNotification);
            na->deleteAll();
            setText(QString::number(0));
        });
        //NOLINTEND

        delall->setEnabled(!na->isEmpty());

        menu.setDefaultAction(delall);

        menu.exec(this->mapToGlobal(e->pos()));
    }
    QPushButton::mousePressEvent(e);
}

bool NotificationArea::areDeveloperWarningsActive() const
{
    return pImp->developerWarningSubscriptionEnabled;
}

bool NotificationArea::areDeveloperErrorsActive() const
{
    return pImp->developerErrorSubscriptionEnabled;
}

void NotificationArea::pushNotification(const QString& notifiername, const QString& message,
                                        Base::LogStyle level)
{
    auto confirmation = confirmationRequired(level);

    if (confirmation) {
        showConfirmationDialog(notifiername, message);
    }
    // guard to avoid modifying the notification list and indices while creating the tooltip
    lock_guard<std::mutex> g(pImp->mutexNotification);

    //NOLINTNEXTLINE
    NotificationsAction* na = static_cast<NotificationsAction*>(pImp->notificationaction);

    // Limit the maximum number of messages stored in the widget (0 means no limit)
    if (pImp->maxWidgetMessages != 0 && na->count() > pImp->maxWidgetMessages) {
        na->deleteLastItem();
    }

    auto repeated = na->isSameNotification(notifiername, message, level);

    if(!repeated) {
        auto itemptr = std::make_unique<NotificationItem>(level, notifiername, message);

        auto item = na->push_front(std::move(itemptr));

        // If the non-intrusive notifications are disabled then stop here (messages added to the widget
        // only)
        if (pImp->notificationsDisabled) {
            item->setNotified(); // avoid mass of old notifications if feature is activated afterwards
            //NOLINTBEGIN
            setText(QString::number(
                static_cast<NotificationsAction*>(pImp->notificationaction)->getUnreadCount()));
            return;
            //NOLINTEND
        }
    }
    else {
        na->resetLastNotificationStatus();
    }

    // start or restart rate control (the timer is rearmed if not yet expired, expiration triggers
    // showing of the notification messages)
    //
    // NOTE: The inhibition timer is created in the main thread and cannot be restarted from another
    // QThread. A QTimer can be moved to another QThread (from the QThread in which it is). However,
    // it can only be create in a QThread, not in any other thread.
    //
    // For this reason, the timer is only triggered if this QThread is the QTimer thread.
    //
    // But I want my message from my thread to appear in the notification area. Fine, then configure
    // Console not to use the direct connection mode, but the Queued one:
    // Base::Console().SetConnectionMode(ConnectionMode::Queued);

    auto timer_thread = pImp->inhibitTimer.thread();
    auto current_thread = QThread::currentThread();

    if (timer_thread == current_thread) {
        pImp->inhibitTimer.start(static_cast<int>(pImp->inhibitNotificationTime));
    }
}

bool NotificationArea::confirmationRequired(Base::LogStyle level)
{
    auto userInitiatedRestore =
        Application::Instance->testStatus(Gui::Application::UserInitiatedOpenDocument);

    return (level == Base::LogStyle::Critical && userInitiatedRestore
            && pImp->requireConfirmationCriticalMessageDuringRestoring);
}

void NotificationArea::showConfirmationDialog(const QString& notifiername, const QString& message)
{
    auto confirmMsg = QObject::tr("Notifier:") + QStringLiteral(" ") + notifiername + QStringLiteral("\n\n") + message
        + QStringLiteral("\n\n")
        + QObject::tr("Do you want to skip confirmation of further critical message notifications "
                      "while loading the file?");

    auto button = QMessageBox::critical(getMainWindow()->activeWindow(),
                                        QObject::tr("Critical Message"),
                                        confirmMsg,
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);

    if (button == QMessageBox::Yes)
        pImp->requireConfirmationCriticalMessageDuringRestoring = false;
}

void NotificationArea::showInNotificationArea()
{
    // guard to avoid modifying the notification list and indices while creating the tooltip
    lock_guard<std::mutex> g(pImp->mutexNotification);

    //NOLINTNEXTLINE
    NotificationsAction* na = static_cast<NotificationsAction*>(pImp->notificationaction);

    if (!NotificationBox::isVisible()) {
        // The Notification Box may have been closed (by the user by popping it out or by left mouse
        // button) ensure that old notifications are not shown again, even if the timer has not
        // lapsed
        int i = 0;
        //NOLINTNEXTLINE
        while (i < na->count() && static_cast<NotificationItem*>(na->getItem(i))->isNotifying()) {
            //NOLINTNEXTLINE
            NotificationItem* item = static_cast<NotificationItem*>(na->getItem(i));

            if (item->isShown()) {
                item->setNotified();
                item->resetShown();
            }

            i++;
        }
    }

    auto currentlyshown = na->getShownCount();

    // If we cannot show more messages, we do no need to update the non-intrusive notification
    if (currentlyshown < pImp->maxOpenNotifications) {
        // There is space for at least one more notification
        // We update the message with the most recent up to maxOpenNotifications

        QString msgw =
            QString::fromLatin1(
                "<style>p { margin: 0 0 0 0 } td { padding: 0 15px }</style>                     \
        <p style='white-space:normal'>                                                                                      \
        <table>                                                                                                             \
        <tr>                                                                                                               \
        <th><small>%1</small></th>                                                                                        \
        <th><small>%2</small></th>                                                                                        \
        <th><small>%3</small></th>                                                                                        \
        </tr>")
                .arg(QObject::tr("Type"), QObject::tr("Notifier"), QObject::tr("Message"));

        auto currentlynotifying = na->getCurrentlyNotifyingCount();

        if (currentlynotifying > pImp->maxOpenNotifications) {
            msgw +=
                QString::fromLatin1(
                    "                                                                                   \
            <tr>                                                                                                            \
            <td align='left'><img width=\"16\" height=\"16\" src=':/icons/Warning.svg'></td>                                \
            <td align='left'>FreeCAD</td>                                                                                   \
            <td align='left'>%1</td>                                                                                        \
            </tr>")
                    .arg(QObject::tr("Too many opened non-intrusive notifications. Notifications "
                                     "are being omitted!"));
        }

        int i = 0;

        //NOLINTNEXTLINE
        while (i < na->count() && static_cast<NotificationItem*>(na->getItem(i))->isNotifying()) {

            if (i < pImp->maxOpenNotifications) {// show the first up to maxOpenNotifications
                //NOLINTNEXTLINE
                NotificationItem* item = static_cast<NotificationItem*>(na->getItem(i));

                QString iconstr;
                if (item->isType(Base::LogStyle::Error)) {
                    iconstr = QStringLiteral(":/icons/edit_Cancel.svg");
                }
                else if (item->isType(Base::LogStyle::Warning)) {
                    iconstr = QStringLiteral(":/icons/Warning.svg");
                }
                else if (item->isType(Base::LogStyle::Critical)) {
                    iconstr = QStringLiteral(":/icons/critical-info.svg");
                }
                else {
                    iconstr = QStringLiteral(":/icons/info.svg");
                }

                QString tmpmessage =
                    convertFromPlainText(item->getMessage(), Qt::WhiteSpaceMode::WhiteSpaceNormal);

                msgw +=
                    QString::fromLatin1(
                        "                                                                                   \
                <tr>                                                                                                            \
                <td align='left'><img width=\"16\" height=\"16\" src='%1'></td>                                                 \
                <td align='left'>%2</td>                                                                                        \
                <td align='left'>%3</td>                                                                                        \
                </tr>")
                        .arg(iconstr, item->getNotifier(), tmpmessage);

                // start a timer for each of these notifications that was not previously shown
                if (!item->isShown()) {
                    QTimer::singleShot(pImp->notificationExpirationTime, [this, item, repetitions = item->getRepetitions()]() {
                        // guard to avoid modifying the notification
                        // start index while creating the tooltip
                        lock_guard<std::mutex> g(pImp->mutexNotification);

                        // if the item exists and the number of repetitions has not changed in the
                        // meantime
                        if (item && item->getRepetitions() == repetitions) {
                            item->resetShown();
                            item->setNotified();

                            if (pImp->autoRemoveUserNotifications) {
                                if (item->isType(Base::LogStyle::Notification)) {
                                    //NOLINTNEXTLINE
                                    static_cast<NotificationsAction*>(pImp->notificationaction)
                                        ->deleteItem(item);
                                }
                            }
                        }
                    });
                }

                // We update the status to shown
                item->setShown();
            }
            else {// We do not have more space and older notifications will be too old
                //NOLINTBEGIN
                static_cast<NotificationItem*>(na->getItem(i))->setNotified();
                static_cast<NotificationItem*>(na->getItem(i))->resetShown();
                //NOLINTEND
            }

            i++;
        }

        msgw += QString::fromLatin1("</table></p>");

        NotificationBox::Options options = NotificationBox::Options::RestrictAreaToReference;

        if (pImp->preventNonIntrusiveNotificationsWhenWindowNotActive) {
            options = options | NotificationBox::Options::OnlyIfReferenceActive;
        }

        if (pImp->hideNonIntrusiveNotificationsWhenWindowDeactivated) {
            options = options | NotificationBox::Options::HideIfReferenceWidgetDeactivated;
        }

        bool isshown = NotificationBox::showText(this->mapToGlobal(QPoint()),
                                                 msgw,
                                                 getMainWindow(),
                                                 static_cast<int>(pImp->notificationExpirationTime),
                                                 pImp->minimumOnScreenTime,
                                                 options,
                                                 pImp->notificationWidth);

        if (!isshown && !pImp->missedNotifications) {
            pImp->missedNotifications = true;
            setIcon(TrayIcon::MissedNotifications);
        }
    }
}

void NotificationArea::slotRestoreFinished(const App::Document&)
{
    // Re-arm on restore critical message modal notifications if another document is loaded
    pImp->requireConfirmationCriticalMessageDuringRestoring = true;
}

void NotificationArea::setIcon(TrayIcon trayIcon)
{
    if (trayIcon == TrayIcon::Normal) {
        QPushButton::setIcon(ResourceManager::NotificationAreaIcon());
    }
    else if (trayIcon == TrayIcon::MissedNotifications) {
        QPushButton::setIcon(ResourceManager::notificationAreaMissedNotificationsIcon());
    }
}
