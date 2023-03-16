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
# include <QMessageBox>
#endif

#include "DlgSettingsNotificationArea.h"
#include "ui_DlgSettingsNotificationArea.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsNotificationArea */

DlgSettingsNotificationArea::DlgSettingsNotificationArea(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgSettingsNotificationArea)
{
    ui->setupUi(this);

    connect(ui->NotificationAreaEnabled, &QCheckBox::stateChanged, [this](int state) {
        if(state == Qt::CheckState::Checked) {
            ui->NonIntrusiveNotificationsEnabled->setEnabled(true);
            ui->maxDuration->setEnabled(true);
            ui->maxDuration->setEnabled(true);
            ui->minDuration->setEnabled(true);
            ui->maxNotifications->setEnabled(true);
            ui->maxWidgetMessages->setEnabled(true);
            ui->autoRemoveUserNotifications->setEnabled(true);
            QMessageBox::information(this, tr("Notification Area"),
            tr("Activation of the Notification Area only takes effect after an application restart."));
        }
        else {
            ui->NonIntrusiveNotificationsEnabled->setEnabled(false);
            ui->maxDuration->setEnabled(false);
            ui->maxDuration->setEnabled(false);
            ui->minDuration->setEnabled(false);
            ui->maxNotifications->setEnabled(false);
            ui->maxWidgetMessages->setEnabled(false);
            ui->autoRemoveUserNotifications->setEnabled(false);
        // N.B: Deactivation is handled by the Notification Area itself, as it listens to all its configuration parameters.
        }
    });
}

DlgSettingsNotificationArea::~DlgSettingsNotificationArea()
{
}

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
}

void DlgSettingsNotificationArea::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

#include "moc_DlgSettingsNotificationArea.cpp"
