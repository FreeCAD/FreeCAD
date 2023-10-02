/***************************************************************************
 *   Copyright (c) 2022 Chris Hennes <chennes@pioneerlibrarysystem.org>    *
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
# include <QDateTime>
# include <QPushButton>
#endif

#include "DlgRevertToBackupConfigImp.h"
#include "ui_DlgRevertToBackupConfig.h"
#include "Application.h"
#include "PreferencePackManager.h"


using namespace Gui;
using namespace Gui::Dialog;
namespace fs = boost::filesystem;

/* TRANSLATOR Gui::Dialog::DlgRevertToBackupConfigImp */

DlgRevertToBackupConfigImp::DlgRevertToBackupConfigImp(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_DlgRevertToBackupConfig)
{
    ui->setupUi(this);
    connect(ui->listWidget, &QListWidget::itemSelectionChanged, this, &DlgRevertToBackupConfigImp::onItemSelectionChanged);
}

DlgRevertToBackupConfigImp::~DlgRevertToBackupConfigImp() = default;

void Gui::Dialog::DlgRevertToBackupConfigImp::onItemSelectionChanged()
{
    auto items = ui->listWidget->selectedItems();
    if (items.count() == 1)
        ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(true);
    else
        ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);

}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgRevertToBackupConfigImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgRevertToBackupConfigImp::showEvent(QShowEvent* event)
{
    ui->listWidget->clear();
    const auto& backups = Application::Instance->prefPackManager()->configBackups();
    for (const auto& backup : backups) {
        auto filename = backup.filename().string();
        auto modification_date = QDateTime::fromSecsSinceEpoch(fs::last_write_time(backup));
        auto item = new QListWidgetItem(QLocale().toString(modification_date));
        item->setData(Qt::UserRole, QString::fromStdString(backup.string()));
        ui->listWidget->addItem(item);
    }
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
    QDialog::showEvent(event);
}

void DlgRevertToBackupConfigImp::accept()
{
    auto items = ui->listWidget->selectedItems();
    if (items.count() != 1) {
        Base::Console().Error(tr("No selection in dialog, cannot load backup file").toStdString().c_str());
        return;
    }
    auto item = items[0];
    auto path = item->data(Qt::UserRole).toString().toStdString();
    if (fs::exists(path)) {
        auto newParameters = ParameterManager::Create();
        newParameters->LoadDocument(path.c_str());
        auto baseAppGroup = App::GetApplication().GetUserParameter().GetGroup("BaseApp");
        newParameters->GetGroup("BaseApp")->copyTo(baseAppGroup);
    }
    else {
        Base::Console().Error("Preference Pack Internal Error: Invalid backup file location");
    }

    QDialog::accept();
}

#include "moc_DlgRevertToBackupConfigImp.cpp"
