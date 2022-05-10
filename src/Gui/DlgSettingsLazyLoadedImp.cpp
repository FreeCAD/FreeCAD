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
#ifndef _PreComp_
#include <QCheckBox>
#include <QPushButton>
#include <sstream>
#endif

#include "DlgSettingsLazyLoadedImp.h"
#include "ui_DlgSettingsLazyLoaded.h"
#include "Application.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


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
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsLazyLoadedImp::~DlgSettingsLazyLoadedImp()
{
}


void DlgSettingsLazyLoadedImp::saveSettings()
{
    std::ostringstream csv;
    for (const auto& checkbox : _autoloadCheckboxes) {
        if (checkbox.second->isChecked()) {
            if (!csv.str().empty())
                csv << ",";
            csv << checkbox.first.toStdString();
        }
    }
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        SetASCII("BackgroundAutoloadModules", csv.str().c_str());
}

void DlgSettingsLazyLoadedImp::loadSettings()
{
    // There are two different "autoload" settings: the first, in FreeCAD since 2004, 
    // controls the module the user sees first when starting FreeCAD, and defaults to the Start workbench
    std::string start = App::Application::Config()["StartWorkbench"];
    _startupModule = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetASCII("AutoloadModule", start.c_str());

    // The second autoload setting does a background autoload of any number of other modules
    std::string autoloadCSV = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetASCII("BackgroundAutoloadModules", "");

    // Tokenize the comma-separated list
    _backgroundAutoloadedModules.clear();
    std::stringstream stream(autoloadCSV);
    std::string workbench;
    while (std::getline(stream, workbench, ','))
        _backgroundAutoloadedModules.push_back(workbench);


    buildUnloadedWorkbenchList();
}

void DlgSettingsLazyLoadedImp::onLoadClicked(const QString &wbName)
{
    Workbench* originalActiveWB = WorkbenchManager::instance()->active();
    Application::Instance->activateWorkbench(wbName.toStdString().c_str());
    Application::Instance->activateWorkbench(originalActiveWB->name().c_str());
    buildUnloadedWorkbenchList();
}


/**
Build the list of unloaded workbenches.
*/
void DlgSettingsLazyLoadedImp::buildUnloadedWorkbenchList()
{
    QStringList workbenches = Application::Instance->workbenches();
    workbenches.sort();
   
    ui->workbenchTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->workbenchTable->setRowCount(0);
    _autoloadCheckboxes.clear(); // setRowCount(0) just invalidated all of these pointers
    ui->workbenchTable->setColumnCount(4);
    ui->workbenchTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeMode::ResizeToContents);
    QStringList columnHeaders;
    columnHeaders << QString() << tr("Workbench") << tr("Autoload") << QString();
    ui->workbenchTable->setHorizontalHeaderLabels(columnHeaders);

    unsigned int rowNumber = 0;
    for (const auto& wbName : workbenches) {
        if (wbName.toStdString() == "NoneWorkbench")
            continue; // Do not list the default empty Workbench

        ui->workbenchTable->insertRow(rowNumber);
        auto wbTooltip = Application::Instance->workbenchToolTip(wbName);

        // Column 1: Workbench Icon
        auto wbIcon = Application::Instance->workbenchIcon(wbName);
        auto iconLabel = new QLabel();
        iconLabel->setPixmap(wbIcon.scaled(QSize(20,20), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        iconLabel->setToolTip(wbTooltip);
        iconLabel->setContentsMargins(5, 3, 3, 3); // Left, top, right, bottom
        ui->workbenchTable->setCellWidget(rowNumber, 0, iconLabel);

        // Column 2: Workbench Display Name
        auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);
        auto textLabel = new QLabel(wbDisplayName);
        textLabel->setToolTip(wbTooltip);
        ui->workbenchTable->setCellWidget(rowNumber, 1, textLabel);
        
        // Column 3: Autoloaded checkbox
        // 
        // To get the checkbox centered, we have to jump through some hoops...
        QWidget* checkWidget = new QWidget(this);
        auto autoloadCheckbox = new QCheckBox(this);
        autoloadCheckbox->setToolTip(tr("If checked") + 
                                     QString::fromUtf8(", ") + wbDisplayName + QString::fromUtf8(" ") + 
                                     tr("will be loaded automatically when FreeCAD starts up"));
        QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
        checkLayout->addWidget(autoloadCheckbox);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->setContentsMargins(0, 0, 0, 0);

        // Figure out whether to check and/or disable this checkbox:
        if (wbName.toStdString() == _startupModule) {
            autoloadCheckbox->setChecked(true);
            autoloadCheckbox->setEnabled(false);
            autoloadCheckbox->setToolTip(tr("This is the current startup module, and must be autoloaded. See Preferences/General/Autoload to change."));
        }
        else if (std::find(_backgroundAutoloadedModules.begin(), _backgroundAutoloadedModules.end(),
                           wbName.toStdString()) != _backgroundAutoloadedModules.end()) {
            autoloadCheckbox->setChecked(true);
            _autoloadCheckboxes.insert(std::make_pair(wbName, autoloadCheckbox));
        }
        else {
            _autoloadCheckboxes.insert(std::make_pair(wbName, autoloadCheckbox));
        }
        ui->workbenchTable->setCellWidget(rowNumber, 2, checkWidget);
        
        // Column 4: Load button/loaded indicator
        if (WorkbenchManager::instance()->getWorkbench(wbName.toStdString())) {
            auto label = new QLabel(tr("Loaded"));
            label->setAlignment(Qt::AlignCenter);
            ui->workbenchTable->setCellWidget(rowNumber, 3, label);
        } 
        else {
            auto button = new QPushButton(tr("Load now"));
            connect(button, &QPushButton::clicked, this, [this,wbName]() { onLoadClicked(wbName); });
            ui->workbenchTable->setCellWidget(rowNumber, 3, button);
        }

        ++rowNumber;
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