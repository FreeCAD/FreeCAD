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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <QMessageBox>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>
#include "DlgSettingsNotificationArea.h"
#include "ui_DlgSettingsNotificationArea.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsNotificationArea */

DlgSettingsNotificationArea::DlgSettingsNotificationArea(QWidget* parent)
    : PreferencePage(parent),
      ui(new Ui_DlgSettingsNotificationArea)
{
    ui->setupUi(this);

    connect(ui->NotificationAreaEnabled, &QGroupBox::toggled, [this](int on) {

        bool enabled = on;

        if (enabled) {
            this->requireRestart();
        }
    });
}

DlgSettingsNotificationArea::~DlgSettingsNotificationArea() = default;

void DlgSettingsNotificationArea::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/NotificationArea");

    bool isNotificationAreaEnabled = ui->NotificationAreaEnabled->isChecked();
    hGrp->SetBool("NotificationAreaEnabled", isNotificationAreaEnabled);
    bool isNonIntrusiveNotificationsEnabled = ui->NotificationAreaEnabled->isChecked();
    hGrp->SetBool("NonIntrusiveNotificationsEnabled", isNonIntrusiveNotificationsEnabled);

    ui->maxDuration->onSave();
    ui->minDuration->onSave();
    ui->maxNotifications->onSave();
    ui->maxWidgetMessages->onSave();
    ui->autoRemoveUserNotifications->onSave();
    ui->notificationWidth->onSave();
    ui->hideNonIntrusiveNotificationsWhenWindowDeactivated->onSave();
    ui->preventNonIntrusiveNotificationsWhenWindowNotActive->onSave();
    ui->developerErrorSubscriptionEnabled->onSave();
    ui->developerWarningSubscriptionEnabled->onSave();
}

void DlgSettingsNotificationArea::loadSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/NotificationArea");

    bool isNotificationAreaEnabled = hGrp->GetBool("NotificationAreaEnabled", true);
    ui->NotificationAreaEnabled->setChecked(isNotificationAreaEnabled);

    bool isNonIntrusiveNotificationsEnabled = hGrp->GetBool("NonIntrusiveNotificationsEnabled", true);
    ui->NonIntrusiveNotificationsEnabled->setChecked(isNonIntrusiveNotificationsEnabled);

    ui->maxDuration->onRestore();
    ui->minDuration->onRestore();
    ui->maxNotifications->onRestore();
    ui->maxWidgetMessages->onRestore();
    ui->autoRemoveUserNotifications->onRestore();
    ui->notificationWidth->onRestore();
    ui->hideNonIntrusiveNotificationsWhenWindowDeactivated->onRestore();
    ui->preventNonIntrusiveNotificationsWhenWindowNotActive->onRestore();
    ui->developerErrorSubscriptionEnabled->onRestore();
    ui->developerWarningSubscriptionEnabled->onRestore();
}

void DlgSettingsNotificationArea::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

#include "moc_DlgSettingsNotificationArea.cpp"
