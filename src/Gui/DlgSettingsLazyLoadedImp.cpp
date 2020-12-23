/***************************************************************************
 *   Copyright (c) 2020 Chris Hennes (chennes@pioneerlibrarysystem.org)    *
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

#include "DlgSettingsLazyLoadedImp.h"
#include "ui_DlgSettingsLazyLoaded.h"
#include "PrefWidgets.h"
#include "AutoSaver.h"

#include "Application.h"
#include "WorkbenchManager.h"
#include "Workbench.h"

using namespace Gui::Dialog;

const uint DlgSettingsLazyLoadedImp::WorkbenchNameRole = Qt::UserRole;

/* TRANSLATOR Gui::Dialog::DlgSettingsLazyLoadedImp */

/**
 *  Constructs a DlgSettingsLazyLoadedImp 
 */
DlgSettingsLazyLoadedImp::DlgSettingsLazyLoadedImp( QWidget* parent )
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsLazyLoaded)
{
    ui->setupUi(this);
    buildUnloadedWorkbenchList();
    connect(ui->loadButton, &QPushButton::clicked, this, &DlgSettingsLazyLoadedImp::onLoadClicked);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsLazyLoadedImp::~DlgSettingsLazyLoadedImp()
{
}


void DlgSettingsLazyLoadedImp::saveSettings()
{
    
}

void DlgSettingsLazyLoadedImp::loadSettings()
{
   
}

void DlgSettingsLazyLoadedImp::onLoadClicked()
{
    Workbench* originalActiveWB = WorkbenchManager::instance()->active();
    auto selection = ui->workbenchList->selectedItems();
    for (const auto& item : selection) {
        auto name = item->data(WorkbenchNameRole).toString().toStdString();
        Application::Instance->activateWorkbench(name.c_str());
    }
    Application::Instance->activateWorkbench(originalActiveWB->name().c_str());
    buildUnloadedWorkbenchList();
}


/**
Build the list of unloaded workbenches.
*/
void DlgSettingsLazyLoadedImp::buildUnloadedWorkbenchList()
{
    ui->workbenchList->clear();
    QStringList workbenches = Application::Instance->workbenches();
    for (const auto& wbName : workbenches) {
        const auto& wb = WorkbenchManager::instance()->getWorkbench(wbName.toStdString());
        if (!wb) {
            auto wbIcon = Application::Instance->workbenchIcon(wbName);
            auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);
            auto wbTooltip = Application::Instance->workbenchToolTip(wbName);
            QListWidgetItem *wbRow = new QListWidgetItem(wbIcon, wbDisplayName);
            wbRow->setData(WorkbenchNameRole, QVariant(wbName)); // Store the actual internal name for easier loading
            wbRow->setToolTip(wbTooltip);
            ui->workbenchList->addItem(wbRow); // Transfers ownership to the QListWidget
        }
    }
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsLazyLoadedImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsLazyLoadedImp.cpp"