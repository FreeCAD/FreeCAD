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

#include "DlgSettingsNotificationArea.h"
#include "ui_DlgSettingsNotificationArea.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsNotificationArea */

DlgSettingsNotificationArea::DlgSettingsNotificationArea(QWidget* parent)
    : PreferencePage(parent),
      ui(new Ui_DlgSettingsNotificationArea)
{
    ui->setupUi(this);

    adaptUiToAreaEnabledState(ui->NotificationAreaEnabled->isChecked());
    connect(ui->NotificationAreaEnabled, &QCheckBox::stateChanged, [this](int state) {
        bool enabled = state == Qt::CheckState::Checked;
        this->adaptUiToAreaEnabledState(enabled);

        if (enabled) {
            this->requireRestart();
        }
    });
}

DlgSettingsNotificationArea::~DlgSettingsNotificationArea() = default;

void DlgSettingsNotificationArea::saveSettings()
{
    ui->NotificationAreaEnabled->onSave();
    ui->NonIntrusiveNotificationsEnabled->onSave();
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
    ui->NotificationAreaEnabled->onRestore();
    ui->NonIntrusiveNotificationsEnabled->onRestore();
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

void DlgSettingsNotificationArea::adaptUiToAreaEnabledState(bool enabled)
{
    ui->NonIntrusiveNotificationsEnabled->setEnabled(enabled);
    ui->maxDuration->setEnabled(enabled);
    ui->maxDuration->setEnabled(enabled);
    ui->minDuration->setEnabled(enabled);
    ui->maxNotifications->setEnabled(enabled);
    ui->maxWidgetMessages->setEnabled(enabled);
    ui->autoRemoveUserNotifications->setEnabled(enabled);
    ui->hideNonIntrusiveNotificationsWhenWindowDeactivated->setEnabled(enabled);
    ui->preventNonIntrusiveNotificationsWhenWindowNotActive->setEnabled(enabled);
    ui->developerErrorSubscriptionEnabled->setEnabled(enabled);
    ui->developerWarningSubscriptionEnabled->setEnabled(enabled);
}

void DlgSettingsNotificationArea::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

#include "moc_DlgSettingsNotificationArea.cpp"
