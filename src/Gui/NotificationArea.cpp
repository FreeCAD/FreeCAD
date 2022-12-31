/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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

#ifndef _PreComp_
# include <memory>
# include <mutex>
# include <QApplication>
# include <QAction>
# include <QActionEvent>
# include <QEvent>
# include <QHBoxLayout>
# include <QHeaderView>
# include <QMenu>
# include <QStringList>
# include <QTreeWidget>
# include <QTimer>
# include <QToolTip>
# include <QWidgetAction>
#endif

#include <Base/Console.h>

#include "BitmapFactory.h"

#include "NotificationArea.h"

using namespace Gui;

namespace Gui {
class NotificationItem : public QTreeWidgetItem
{
public:
    NotificationItem(NotificationArea::NotificationType notificationtype, QString notifiername, QString message):
        notificationType(notificationtype), notifierName(std::move(notifiername)){
            message = message.simplified();
            msg = std::move(message);
        }

    QVariant data(int column, int role) const override {
        // property name
        if( role == Qt::DisplayRole ) {
            switch(column) {
                case 1:
                    return notifierName;
                    break;
                case 2:
                    return msg;
                    break;
            }
        }
        else
        if(column == 0 && role == Qt::DecorationRole) {
            if(notificationType == NotificationArea::NotificationType::Error) {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/edit_Cancel.svg",QSize(16, 16)));
            }
            else
            if(notificationType == NotificationArea::NotificationType::Warning) {
                return QVariant::fromValue(BitmapFactory().pixmapFromSvg(":/icons/Warning.svg",QSize(16, 16)));
            }
        }

        return QVariant();
    }

    NotificationArea::NotificationType notificationType;
    QString notifierName;
    QString msg;
};

class NotificationAreaObserver: public Base::ILogger
{
public:
    NotificationAreaObserver(NotificationArea * notificationarea);
    ~NotificationAreaObserver() override;


    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level) override;

    /// name of the observer
    const char *Name() override {return "NotificationAreaObserver";}

private:
    NotificationArea * notificationArea;
};

struct NotificationAreaP
{
    int currentlyNotifyingIndex = 0;
    unsigned int unread = 0;
    std::mutex mutexNotification;
    const unsigned int notificationExpirationTime = 5000;
    QMenu * menu;
    QTreeWidget * table;

    std::unique_ptr<NotificationAreaObserver> observer;
};

} // namespace Gui

NotificationAreaObserver::NotificationAreaObserver(NotificationArea * notificationarea): notificationArea(notificationarea)
{
    Base::Console().AttachObserver(this);
}

NotificationAreaObserver::~NotificationAreaObserver()
{
    Base::Console().DetachObserver(this);
}

void NotificationAreaObserver::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level)
{
    switch(level) {
        case Base::LogStyle::Error:
            notificationArea->pushNotification(NotificationArea::NotificationType::Error, QString::fromLatin1(notifiername.c_str()), QString::fromLatin1(msg.c_str()));
            break;
        case Base::LogStyle::Warning:
            notificationArea->pushNotification(NotificationArea::NotificationType::Warning, QString::fromLatin1(notifiername.c_str()), QString::fromLatin1(msg.c_str()));
            break;
        case Base::LogStyle::Message:
            notificationArea->pushNotification(NotificationArea::NotificationType::Message, QString::fromLatin1(notifiername.c_str()), QString::fromLatin1(msg.c_str()));
            break;
        default:
            break;
    }
}

class NotificationsAction : public QWidgetAction
{
public:
    NotificationsAction(QWidget* parent) : QWidgetAction(parent), parentWidget(parent) {}

    auto getTable(){return tableWidget;}
protected:
    QWidget* createWidget(QWidget* parent) override
    {
        QWidget* notificationsWidget = new QWidget(parent);

        QHBoxLayout* layout = new QHBoxLayout(notificationsWidget);
        notificationsWidget->setLayout(layout);

        tableWidget = new QTreeWidget(parent);
        tableWidget->setColumnCount(3);

        QStringList headers;
        headers << QObject::tr("Type") << QObject::tr("Notifier") << QObject::tr("Message");
        tableWidget->setHeaderLabels(headers);

        layout->addWidget(tableWidget);

        tableWidget->setMaximumSize(1200,600);
        tableWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        tableWidget->header()->setStretchLastSection(false);
        tableWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

        return notificationsWidget;
    }

private:
    QTreeWidget * tableWidget;
    QWidget * parentWidget;
};

NotificationArea::NotificationArea(QWidget *parent):QPushButton(parent)
{
    setText(0);
    setFlat(true);

    d = std::make_unique<NotificationAreaP>();

    d->observer = std::make_unique<NotificationAreaObserver>(this);

    d->menu = new QMenu(parent);
    setMenu(d->menu);

    auto na = new NotificationsAction(d->menu);

    d->menu->addAction(na);

    d->table = na->getTable();

    QObject::connect(d->menu, &QMenu::aboutToHide,
                     [&]() {
                         std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip
                         d->unread = 0;
                         d->table->clearSelection();
                         setText(QString::number(d->unread));
                     });

    QObject::connect(d->menu, &QMenu::aboutToShow,
                     [&]() {
                         std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

                         for (unsigned int i=0; i < d->unread; i++) {
                             d->table->topLevelItem(i)->setSelected(true);
                         }
                     });
}

void NotificationArea::pushNotification(NotificationType notificationtype, const QString & notifiername, const QString & message)
{
    auto * item = new NotificationItem(notificationtype, notifiername, message);

    std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification list and indices while creating the tooltip

    d->table->insertTopLevelItem(0,item);
    d->currentlyNotifyingIndex++;
    d->unread++;
    setText(QString::number(d->unread));

    showInNotificationArea();

    QTimer::singleShot(d->notificationExpirationTime, [this](){
        std::lock_guard<std::mutex> g(d->mutexNotification); // guard to avoid modifying the notification start index while creating the tooltip
        d->currentlyNotifyingIndex--;
    });

}

void NotificationArea::showInNotificationArea()
{
    QString msgw = QString::fromLatin1("<style>p { margin: 0 0 0 0 }</style><p style='white-space:nowrap'>              \
    <table>                                                                                                             \
     <tr>                                                                                                               \
      <th><small>%1</small></th>                                                                                        \
      <th><small>%2</small></th>                                                                                        \
      <th><small>%3</small></th>                                                                                        \
     </tr>")
        .arg(QObject::tr("Type"))
        .arg(QObject::tr("Notifier"))
        .arg(QObject::tr("Message"));

    for(int i = 0 ; i < d->currentlyNotifyingIndex; i++) {
        NotificationItem* item = static_cast<NotificationItem*>(d->table->topLevelItem(i));

        QString iconstr;
        if(item->notificationType == NotificationArea::NotificationType::Error) {
            iconstr = QStringLiteral(":/icons/edit_Cancel.svg");
        }
        else
        if(item->notificationType == NotificationArea::NotificationType::Warning) {
            iconstr = QStringLiteral(":/icons/Warning.svg");
        }

        msgw += QString::fromLatin1("                                                                                   \
        <tr>                                                                                                            \
        <td align='center'><img width=\"16\" height=\"16\" src='%1'></td>                                               \
        <td align='center'>%2</td>                                                                                      \
        <td align='center'>%3</td>                                                                                      \
        </tr>")
            .arg(iconstr)
            .arg(item->notifierName)
            .arg(item->msg);
    }



    msgw += QString::fromLatin1("</table></p>");


    QToolTip::hideText(); // ensure the tooltip is not being shown
    QToolTip::showText( this->mapToGlobal( QPoint( ) ), msgw);
}
