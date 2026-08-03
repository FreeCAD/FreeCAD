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

#pragma once

#include <QPushButton>
#include <QString>

#include <map>
#include <memory>

#include <Base/Observer.h>
#include <Base/Parameter.h>

namespace App
{
class Document;
}

namespace Gui
{

struct NotificationAreaP;

class NotificationArea: public QPushButton
{
    enum class TrayIcon
    {
        Normal,
        MissedNotifications,
    };

public:
    class ParameterObserver: public ParameterGrp::ObserverType
    {
    public:
        explicit ParameterObserver(NotificationArea* notificationarea);
        ~ParameterObserver() override;

        void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

    private:
        NotificationArea* notificationArea;
        ParameterGrp::handle hGrp;
        std::map<std::string, std::function<void(const std::string& string)>> parameterMap;
    };

    NotificationArea(QWidget* parent = nullptr);
    ~NotificationArea() override;

    void pushNotification(const QString& notifiername, const QString& message, Base::LogStyle level);

    bool areDeveloperWarningsActive() const;
    bool areDeveloperErrorsActive() const;

private:
    void showInNotificationArea();
    bool confirmationRequired(Base::LogStyle level);
    void showConfirmationDialog(const QString& notifiername, const QString& message);
    void slotRestoreFinished(const App::Document&);

    void mousePressEvent(QMouseEvent* e) override;

    void setIcon(TrayIcon trayIcon);

private:
    std::unique_ptr<NotificationAreaP> pImp;
};


}  // namespace Gui
